/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "syncer_p.h"

#include "networkrequestgenerator_p.h"
#include "networkreplyparser_p.h"
#include "replyparser_p.h"

#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QByteArray>
#include <QtCore/QStandardPaths>

// buteo
#include <SyncProfile.h>
#include <LogMacros.h>

// libaccounts-qt5
#include <Accounts/Account>
#include <Accounts/Service>


Syncer::Syncer(QObject *parent, Buteo::SyncProfile *syncProfile)
    : WebDavSyncer(parent, syncProfile, QStringLiteral("nextcloud-images"))
{
}

Syncer::~Syncer()
{
    delete m_manager;
}

void Syncer::beginSync()
{
    QNetworkReply *reply = m_requestGenerator->userInfo(NetworkRequestGenerator::JsonContentType);
    if (reply) {
        connect(reply, &QNetworkReply::finished,
                this, &Syncer::handleUserInfoReply);
    } else {
        finishWithError(QStringLiteral("Failed to start user info request"));
    }
}

void Syncer::handleUserInfoReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    reply->deleteLater();
    const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() != QNetworkReply::NoError) {
        finishWithHttpError("User info request failed", httpCode);
        return;
    }

    NetworkReplyParser::User user = JsonReplyParser::parseUserResponse(reply->readAll());
    if (user.userId.isEmpty()) {
        finishWithError("No user id found in server response");
        return;
    }

    // Store the user.
    SyncCache::ImageDatabase db;
    SyncCache::DatabaseError error;
    db.openDatabase(
            QStringLiteral("%1/system/privileged/Images/nextcloud.db").arg(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)),
            &error);
    if (error.errorCode != SyncCache::DatabaseError::NoError) {
        LOG_WARNING("Failed to open database to store user for account:" << m_accountId
                    << ":" << error.errorMessage);
        return;
    }
    SyncCache::User currentUser;
    currentUser.accountId = m_accountId;
    currentUser.userId = user.userId;
    currentUser.displayName = user.displayName;
    db.storeUser(currentUser, &error);
    if (error.errorCode != SyncCache::DatabaseError::NoError) {
        LOG_WARNING("Failed to store user:" << currentUser.userId
                    << error.errorCode << error.errorMessage);
    }

    if (!m_manager) {
        m_manager = new Accounts::Manager;
    }
    Accounts::Account *account = m_manager->account(m_accountId);
    if (account) {
        const Accounts::ServiceList services = account->services();
        for (const Accounts::Service &service : services) {
            if (service.name() == m_serviceName) {
                account->selectService(service);
                m_dirListingRootPath = account->value(QStringLiteral("images_path")).toString().trimmed();
                break;
            }
        }
    }
    account->selectService(Accounts::Service());

    m_userId = user.userId;
    if (m_dirListingRootPath.isEmpty()) {
        m_dirListingRootPath = QString("/remote.php/dav/files/%1/Photos/").arg(user.userId);
    }
    m_dirListingResults.photos.clear();
    m_dirListingResults.albums.clear();

    if (!performDirListingRequest(m_dirListingRootPath)) {
        WebDavSyncer::finishWithError("Directory list request failed");
    }
}

bool Syncer::performDirListingRequest(const QString &remoteDirPath)
{
    LOG_DEBUG("Fetching directory listing for" << remoteDirPath);

    QNetworkReply *reply = m_requestGenerator->dirListing(remoteDirPath);
    if (reply) {
        reply->setProperty("remoteDirPath", remoteDirPath);
        connect(reply, &QNetworkReply::finished,
                this, &Syncer::handleDirListingReply);
        return true;
    }

    return false;
}

void Syncer::handleDirListingReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    reply->deleteLater();
    const QByteArray replyData = reply->readAll();
    const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QString remoteDirPath = reply->property("remoteDirPath").toString();

    if (httpCode == 404) {
        // if directory doesn't exist, that's okay, there are no photos.
        WebDavSyncer::finishWithSuccess();
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        WebDavSyncer::finishWithHttpError("Remote directory listing failed", httpCode);
        return;
    }

    const QList<NetworkReplyParser::Resource> resourceList = XmlReplyParser::parsePropFindResponse(
            replyData, remoteDirPath);
    const ReplyParser::GalleryMetadata metadata =
            ReplyParser::galleryMetadataFromResources(this, m_dirListingRootPath, remoteDirPath, resourceList);
    m_dirListingResults.albums += metadata.albums;
    m_dirListingResults.photos += metadata.photos;

    for (const NetworkReplyParser::Resource &resource : resourceList) {
        if (resource.isCollection && resource.href != remoteDirPath) {
            m_pendingAlbumListings.append(resource.href);
        }
    }

    if (!m_pendingAlbumListings.isEmpty()) {
        performDirListingRequest(m_pendingAlbumListings.takeLast());
    } else {
        QHash<QString, SyncCache::Album> albums;
        for (const SyncCache::Album &album : m_dirListingResults.albums) {
            albums.insert(album.albumId, album);
        }

        QHash<QString, SyncCache::Photo> photos;
        for (const SyncCache::Photo &photo : m_dirListingResults.photos) {
            photos.insert(photo.photoId, photo);
        }
        m_dirListingResults.photos.clear();
        m_dirListingResults.albums.clear();

        calculateAndApplyDelta(albums, photos, metadata.photos.count() > 0 ? metadata.photos.first().photoId : QString());

        WebDavSyncer::finishWithSuccess();
    }
}

void Syncer::calculateAndApplyDelta(
        const QHash<QString, SyncCache::Album> &serverAlbums,
        const QHash<QString, SyncCache::Photo> &serverPhotos,
        const QString &firstPhotoId)
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
    const QVector<SyncCache::Album> albums = db.albums(m_accountId, m_userId, &error);
    if (error.errorCode != SyncCache::DatabaseError::NoError) {
        LOG_WARNING("calculateAndApplyDelta: failed to read albums:" << error.errorCode << error.errorMessage);
    } else {
        Q_FOREACH (const SyncCache::Album &album, albums) {
            currentAlbums.insert(album.albumId, album);
            const QVector<SyncCache::Photo> photos = db.photos(m_accountId, m_userId, album.albumId, &error);
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

    // remove deleted albums.
    if (error.errorCode == SyncCache::DatabaseError::NoError) {
        Q_FOREACH (const SyncCache::Album &album, currentAlbums) {
            if (!serverAlbums.contains(album.albumId)) {
                db.deleteAlbum(album, &error);
                if (error.errorCode != SyncCache::DatabaseError::NoError) {
                    LOG_WARNING("calculateAndApplyDelta: failed to delete album:"
                                << album.albumId
                                << error.errorCode << error.errorMessage);
                    break;
                }
                currentAlbums.remove(album.albumId);
                ++removedAlbums;
            }
        }
    }

    // add new albums, update modified albums.
    if (error.errorCode == SyncCache::DatabaseError::NoError) {
        Q_FOREACH (const SyncCache::Album &album, serverAlbums) {
            auto it = currentAlbums.find(album.albumId);
            if (it != currentAlbums.end()) {
                SyncCache::Album currAlbum = *it;
                if (currAlbum.photoCount != album.photoCount
                        || currAlbum.thumbnailUrl != album.thumbnailUrl) {
                    currAlbum.photoCount = album.photoCount;
                    currAlbum.thumbnailUrl = album.thumbnailUrl;
                    db.storeAlbum(currAlbum, &error);
                    if (error.errorCode != SyncCache::DatabaseError::NoError) {
                        LOG_WARNING("calculateAndApplyDelta: failed to update album:"
                                    << currAlbum.albumId
                                    << error.errorCode << error.errorMessage);
                        break;
                    }
                    ++modifiedAlbums;
                }
            } else {
                currentAlbums.insert(album.albumId, album);
                db.storeAlbum(album, &error);
                if (error.errorCode != SyncCache::DatabaseError::NoError) {
                    LOG_WARNING("calculateAndApplyDelta: failed to add album:"
                                << album.albumId
                                << error.errorCode << error.errorMessage);
                    break;
                }
                ++addedAlbums;
            }
        }
    }

    // remove deleted photos.
    if (error.errorCode == SyncCache::DatabaseError::NoError) {
        Q_FOREACH (const SyncCache::Photo &photo, currentPhotos) {
            if (!serverPhotos.contains(photo.photoId)) {
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
        Q_FOREACH (const SyncCache::Photo &photo, serverPhotos) {
            auto it = currentPhotos.find(photo.photoId);
            if (it != currentPhotos.end()) {
                bool changed = false;
                SyncCache::Photo currPhoto = *it;
                if (currPhoto.updatedTimestamp != photo.updatedTimestamp
                        || currPhoto.imageUrl != photo.imageUrl) {
                    // the image has changed, we need to delete the old image file.
                    currPhoto.thumbnailPath = QUrl();
                    currPhoto.thumbnailUrl = photo.thumbnailUrl;
                    currPhoto.imagePath = QUrl();
                    currPhoto.imageUrl = photo.imageUrl;
                    currPhoto.updatedTimestamp = photo.updatedTimestamp;
                    currPhoto.fileSize = photo.fileSize;
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
                        LOG_WARNING("calculateAndApplyDelta: failed to update photo:"
                                    << currPhoto.photoId
                                    << "in album" << photo.albumId << photo.albumPath
                                    << error.errorCode << error.errorMessage);
                        break;
                    }
                    ++modifiedPhotos;
                }
            } else {
                currentPhotos.insert(photo.photoId, photo);
                db.storePhoto(photo, &error);
                if (error.errorCode != SyncCache::DatabaseError::NoError) {
                    LOG_WARNING("calculateAndApplyDelta: failed to add photo:"
                                << photo.photoId
                                << "in album" << photo.albumId << photo.albumPath
                                << error.errorCode << error.errorMessage);
                    break;
                }
                ++addedPhotos;
            }
        }
    }

    // Set the user thumbnail using the first photo
    if (!firstPhotoId.isEmpty()) {
        SyncCache::Photo firstPhoto = serverPhotos.value(firstPhotoId);
        if (firstPhoto.accountId > 0) {
            SyncCache::User currentUser = db.user(m_accountId, &error);
            if (error.errorCode != SyncCache::DatabaseError::NoError) {
                LOG_WARNING("Failed to find user:" << m_userId << "for account:" << m_accountId
                            << error.errorCode << error.errorMessage);
            } else if (currentUser.thumbnailUrl.isEmpty()
                       || currentUser.thumbnailUrl != firstPhoto.thumbnailUrl) {
                currentUser.thumbnailUrl = firstPhoto.thumbnailUrl;
                currentUser.thumbnailPath = firstPhoto.thumbnailPath;
                currentUser.thumbnailFileName = firstPhoto.fileName;
                db.storeUser(currentUser, &error);
                if (error.errorCode != SyncCache::DatabaseError::NoError) {
                    LOG_WARNING("Failed to store user:" << currentUser.userId
                                << error.errorCode << error.errorMessage);
                }
            }
        }
    }

    if (error.errorCode != SyncCache::DatabaseError::NoError) {
        db.rollbackTransaction(&error);
        emit syncFailed();
        return;
    } else if (!db.commitTransaction(&error)) {
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
    } else {
        SyncCache::User user = db.user(accountId, &error);
        if (error.errorCode == SyncCache::DatabaseError::NoError) {
            if (user.userId.isEmpty()) {
                LOG_WARNING("Failed to find Nextcloud user ID for account:" << accountId
                            << ":" << error.errorMessage);
            } else {
                db.deleteUser(user, &error);
                if (error.errorCode != SyncCache::DatabaseError::NoError) {
                    LOG_WARNING("Failed to purge Nextcloud images for account:" << accountId
                                << ":" << error.errorMessage);
                }
            }
        } else {
            LOG_WARNING("Failed to find Nextcloud user for account:" << accountId
                        << ":" << error.errorMessage);
        }
    }

    QDir dir(SyncCache::ImageCache::imageCacheDir(accountId));
    if (dir.exists() && !dir.removeRecursively()) {
        LOG_WARNING("Failed to purge Nextcloud image cache for account:" << accountId
                    << "in dir:" << dir.absolutePath());
    }

    LOG_DEBUG("Purged Nextcloud images for account:" << accountId);
}
