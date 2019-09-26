/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "syncer_p.h"
#include "accountauthenticator_p.h"
#include "webdavrequestgenerator_p.h"
#include "replyparser_p.h"

#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtCore/QFile>
#include <QtCore/QByteArray>
#include <QtCore/QStandardPaths>

#include <Accounts/Manager>
#include <Accounts/Account>

#include <SyncProfile.h>
#include <LogMacros.h>

static const int HTTP_UNAUTHORIZED_ACCESS = 401;
static const QString NEXTCLOUD_USERID = QStringLiteral("nextcloud");

Syncer::Syncer(QObject *parent, Buteo::SyncProfile *syncProfile)
    : QObject(parent)
    , m_syncProfile(syncProfile)
{
}

Syncer::~Syncer()
{
    delete m_auth;
    delete m_requestGenerator;
    delete m_replyParser;
}

void Syncer::abortSync()
{
    m_syncAborted = true;
}

void Syncer::startSync(int accountId)
{
    Q_ASSERT(accountId != 0);
    m_accountId = accountId;
    delete m_auth;
    m_auth = new AccountAuthenticator("sync", "nextcloud-images", this);
    connect(m_auth, &AccountAuthenticator::signInCompleted,
            this, &Syncer::sync);
    connect(m_auth, &AccountAuthenticator::signInError,
            this, &Syncer::signInError);
    LOG_DEBUG(Q_FUNC_INFO << "starting Nextcloud images sync with account" << m_accountId);
    m_auth->signIn(accountId);
}

void Syncer::signInError()
{
    emit syncFailed();
}

void Syncer::sync(const QString &serverUrl, const QString &webdavPath, const QString &username, const QString &password, const QString &accessToken, bool ignoreSslErrors)
{
    m_serverUrl = serverUrl;
    m_webdavPath = webdavPath.isEmpty() ? QStringLiteral("/remote.php/webdav/") : webdavPath;
    m_username = username;
    m_password = password;
    m_accessToken = accessToken;
    m_ignoreSslErrors = ignoreSslErrors;

    delete m_requestGenerator;
    m_requestGenerator = accessToken.isEmpty()
                       ? new WebDavRequestGenerator(&m_qnam, username, password)
                       : new WebDavRequestGenerator(&m_qnam, accessToken);

    delete m_replyParser;
    m_replyParser = new ReplyParser(this, m_accountId, NEXTCLOUD_USERID, serverUrl, m_webdavPath);

    // Generate request for top-level Photos directory.
    // Then, for every Album entry, generate a request for that directory.
    // Parse the replies, calculate delta (additions/modifications/removals) and apply it locally.
    const QString photosPath = QString::fromUtf8(
            QByteArray::fromPercentEncoding(
                    QStringLiteral("%1%2").arg(m_webdavPath, QStringLiteral("Photos/")).toUtf8()));
    if (!performAlbumContentMetadataRequest(serverUrl, photosPath, QString())) {
        LOG_WARNING("handleAlbumContentMetaDataReply: failed to trigger initial album request!");
        webDavError();
    }
}

bool Syncer::performAlbumContentMetadataRequest(const QString &serverUrl, const QString &albumPath, const QString &parentAlbumPath)
{
    QNetworkReply *reply = m_requestGenerator->dirListing(serverUrl, albumPath);
    if (reply) {
        m_requestQueue.append(reply);
        reply->setProperty("albumPath", albumPath);
        reply->setProperty("parentAlbumPath", parentAlbumPath);
        connect(reply, &QNetworkReply::finished,
                this, &Syncer::handleAlbumContentMetaDataReply);
        return true;
    }

    return false;
}

void Syncer::handleAlbumContentMetaDataReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    m_requestQueue.removeAll(reply);
    reply->deleteLater();
    const QByteArray replyData = reply->readAll();
    const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QString albumPath = reply->property("albumPath").toString();
    const QString parentAlbumPath = reply->property("parentAlbumPath").toString();

    if (reply->error() == QNetworkReply::NoError) {
        ReplyParser::ContentMetadata metadata = m_replyParser->parseAlbumContentMetadata(
                replyData, albumPath, parentAlbumPath);

        m_albums.insert(metadata.album.albumId, metadata.album);
        Q_FOREACH (const SyncCache::Album &album, metadata.albums) {
            if (album.albumId != albumPath && !performAlbumContentMetadataRequest(m_serverUrl, album.albumId, metadata.album.albumId)) {
                LOG_WARNING("handleAlbumContentMetaDataReply: failed to trigger request for album:" << album.albumId);
                webDavError();
                return;
            }
        }
        Q_FOREACH (const SyncCache::Photo &photo, metadata.photos) {
            m_photos.insert(photo.photoId, photo);
        }
    } else {
        LOG_WARNING("handleAlbumContentMetaDataReply: error:" << reply->error() << reply->errorString());
        webDavError(httpCode);
        return;
    }

    if (m_requestQueue.isEmpty()) {
        // Finished all requests.  Now determine changes and apply locally.
        LOG_DEBUG("handleAlbumContentMetaDataReply: all replies handled.");
        calculateAndApplyDelta();
    }
}

void Syncer::calculateAndApplyDelta()
{
    LOG_DEBUG("calculateAndApplyDelta: entry");
    SyncCache::ImageDatabase db;
    SyncCache::DatabaseError error;
    db.openDatabase(
            QStringLiteral("%1/system/privileged/Images/nextcloud.db").arg(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)),
            &error);

    if (error.errorCode != SyncCache::DatabaseError::NoError) {
        LOG_WARNING("calculateAndApplyDelta: failed to open database:" << error.errorCode << error.errorMessage);
        emit syncFailed();
        return;
    }

    if (!db.beginTransaction(&error)) {
        LOG_WARNING("calculateAndApplyDelta: failed to begin transaction:" << error.errorCode << error.errorMessage);
        emit syncFailed();
        return;
    }

    // read all albums and photos
    QHash<QString, SyncCache::Album> currentAlbums;
    QHash<QString, SyncCache::Photo> currentPhotos;
    const QVector<SyncCache::Album> albums = db.albums(m_accountId, NEXTCLOUD_USERID, &error);
    if (error.errorCode != SyncCache::DatabaseError::NoError) {
        LOG_WARNING("calculateAndApplyDelta: failed to read albums:" << error.errorCode << error.errorMessage);
    } else {
        Q_FOREACH (const SyncCache::Album &album, albums) {
            currentAlbums.insert(album.albumId, album);
            const QVector<SyncCache::Photo> photos = db.photos(m_accountId, NEXTCLOUD_USERID, album.albumId, &error);
            if (error.errorCode == SyncCache::DatabaseError::NoError) {
                Q_FOREACH (const SyncCache::Photo &photo, photos) {
                    currentPhotos.insert(photo.photoId, photo);
                }
            } else {
                LOG_WARNING("calculateAndApplyDelta: failed to read photos:" << error.errorCode << error.errorMessage);
                break;
            }
        }
    }

    // determine and apply delta.
    int addedAlbums = 0, modifiedAlbums = 0, removedAlbums = 0;
    int addedPhotos = 0, modifiedPhotos = 0, removedPhotos = 0;

    // create the default user if none exists.
    if (error.errorCode == SyncCache::DatabaseError::NoError) {
        SyncCache::User currentUser = db.user(m_accountId, NEXTCLOUD_USERID, &error);
        if (error.errorCode != SyncCache::DatabaseError::NoError) {
            LOG_WARNING("calculateAndApplyDelta: failed to read user:" << error.errorCode << error.errorMessage);
        } else if (currentUser.userId.isEmpty()) {
            // need to store the default user.
            currentUser.accountId = m_accountId;
            currentUser.userId = NEXTCLOUD_USERID;
            db.storeUser(currentUser, &error);
            if (error.errorCode != SyncCache::DatabaseError::NoError) {
                LOG_WARNING("calculateAndApplyDelta: failed to store user:" << error.errorCode << error.errorMessage);
            }
        }
    }

    // remove deleted albums.
    if (error.errorCode == SyncCache::DatabaseError::NoError) {
        Q_FOREACH (const SyncCache::Album &album, currentAlbums) {
            if (!m_albums.contains(album.albumId)) {
                db.deleteAlbum(album, &error);
                if (error.errorCode != SyncCache::DatabaseError::NoError) {
                    LOG_WARNING("calculateAndApplyDelta: failed to delete album:" << error.errorCode << error.errorMessage);
                    break;
                }
                currentAlbums.remove(album.albumId);
                ++removedAlbums;
            }
        }
    }

    // add new albums, update modified albums.
    if (error.errorCode == SyncCache::DatabaseError::NoError) {
        Q_FOREACH (const SyncCache::Album &album, m_albums) {
            if (currentAlbums.contains(album.albumId)) {
                if (currentAlbums[album.albumId].photoCount != album.photoCount) {
                    SyncCache::Album currAlbum = currentAlbums[album.albumId];
                    currAlbum.photoCount = album.photoCount;
                    db.storeAlbum(currAlbum, &error);
                    if (error.errorCode != SyncCache::DatabaseError::NoError) {
                        LOG_WARNING("calculateAndApplyDelta: failed to store album update:" << error.errorCode << error.errorMessage);
                        break;
                    }
                    ++modifiedAlbums;
                }
            } else {
                currentAlbums.insert(album.albumId, album);
                db.storeAlbum(album, &error);
                if (error.errorCode != SyncCache::DatabaseError::NoError) {
                    LOG_WARNING("calculateAndApplyDelta: failed to store album addition:" << error.errorCode << error.errorMessage);
                    break;
                }
                ++addedAlbums;
            }
        }
    }

    // remove deleted photos.
    if (error.errorCode == SyncCache::DatabaseError::NoError) {
        Q_FOREACH (const SyncCache::Photo &photo, currentPhotos) {
            if (!m_photos.contains(photo.photoId)) {
                db.deletePhoto(photo, &error);
                if (error.errorCode != SyncCache::DatabaseError::NoError) {
                    LOG_WARNING("calculateAndApplyDelta: failed to delete photo:" << error.errorCode << error.errorMessage);
                    break;
                }
                currentPhotos.remove(photo.photoId);
                ++removedPhotos;
            }
        }
    }

    // add new photos, updated modified photos.
    if (error.errorCode == SyncCache::DatabaseError::NoError) {
        Q_FOREACH (const SyncCache::Photo &photo, m_photos) {
            if (currentPhotos.contains(photo.photoId)) {
                bool changed = false;
                SyncCache::Photo currPhoto = currentPhotos[photo.photoId];
                if (currPhoto.updatedTimestamp != photo.updatedTimestamp
                        || currPhoto.imageUrl != photo.imageUrl) {
                    // the image has changed, we need to delete the old image file.
                    currPhoto.thumbnailPath = QUrl();
                    currPhoto.thumbnailUrl = photo.thumbnailUrl;
                    currPhoto.imagePath = QUrl();
                    currPhoto.imageUrl = photo.imageUrl;
                    currPhoto.updatedTimestamp = photo.updatedTimestamp;
                    changed = true;
                }
                if (currPhoto.albumId != photo.albumId) {
                    // the image has moved to different album.
                    currPhoto.albumId = photo.albumId;
                    changed = true;
                }
                if (changed) {
                    db.storePhoto(currPhoto, &error);
                    if (error.errorCode != SyncCache::DatabaseError::NoError) {
                        LOG_WARNING("calculateAndApplyDelta: failed to store photo update:" << error.errorCode << error.errorMessage);
                        break;
                    }
                    ++modifiedPhotos;
                }
            } else {
                currentPhotos.insert(photo.photoId, photo);
                db.storePhoto(photo, &error);
                if (error.errorCode != SyncCache::DatabaseError::NoError) {
                    LOG_WARNING("calculateAndApplyDelta: failed to store photo addition:" << error.errorCode << error.errorMessage);
                    break;
                }
                ++addedPhotos;
            }
        }
    }

    if (error.errorCode != SyncCache::DatabaseError::NoError) {
        db.rollbackTransaction(&error);
        emit syncFailed();
        return;
    } else if (!db.commitTransation(&error)) {
        LOG_WARNING("calculateAndApplyDelta: failed to commit transaction:" << error.errorCode << error.errorMessage);
        db.rollbackTransaction(&error);
        emit syncFailed();
        return;
    }

    // success.
    LOG_DEBUG(Q_FUNC_INFO << "Nextcloud images albums A/M/R:" << addedAlbums << "/" << modifiedAlbums << "/" << removedAlbums);
    LOG_DEBUG(Q_FUNC_INFO << "Nextcloud images photos A/M/R:" << addedPhotos << "/" << modifiedPhotos << "/" << removedPhotos);
    LOG_DEBUG(Q_FUNC_INFO << "Nextcloud images sync with account" << m_accountId << "finished successfully!");
    emit syncSucceeded();
}

void Syncer::webDavError(int errorCode)
{
    LOG_WARNING("Nextcloud images sync finished with error:" << errorCode <<
                "for account:" << m_accountId);
    m_syncError = true;
    if (errorCode == HTTP_UNAUTHORIZED_ACCESS) {
        m_auth->setCredentialsNeedUpdate(m_accountId);
    }
    emit syncFailed();
}

void Syncer::purgeAccount(int accountId)
{
    SyncCache::ImageDatabase db;
    SyncCache::DatabaseError error;
    db.openDatabase(
            QStringLiteral("%1/system/privileged/Images/nextcloud.db").arg(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)),
            &error);

    if (error.errorCode != SyncCache::DatabaseError::NoError) {
        LOG_WARNING("Failed to open database in order to purge Nextcloud images for account:" << accountId
                    << ":" << error.errorMessage);
        return;
    }

    SyncCache::User user;
    user.accountId = accountId;
    user.userId = NEXTCLOUD_USERID;
    db.deleteUser(user, &error);

    if (error.errorCode != SyncCache::DatabaseError::NoError) {
        LOG_WARNING("Failed to purge Nextcloud images for account:" << accountId
                    << ":" << error.errorMessage);
    }
}
