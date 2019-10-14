/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "imagecache.h"
#include "imagecache_p.h"

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QUuid>

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>

using namespace SyncCache;

namespace {
    QString constructAlbumIdentifier(int accountId, const QString &userId, const QString &albumId) {
        return QStringLiteral("%1|%2|%3").arg(accountId).arg(userId, albumId);
    }
}

ImageDatabasePrivate::ImageDatabasePrivate(ImageDatabase *parent)
    : DatabasePrivate(parent), m_imageDbParent(parent)
{
}

int ImageDatabasePrivate::currentSchemaVersion() const
{
    return 1;
}

QVector<const char *> ImageDatabasePrivate::createStatements() const
{
    static const char *createUsersTable =
            "\n CREATE TABLE Users ("
            "\n accountId INTEGER,"
            "\n userId TEXT,"
            "\n thumbnailUrl TEXT,"
            "\n thumbnailPath TEXT,"
            "\n PRIMARY KEY (accountId, userId));";

    static const char *createAlbumsTable =
            "\n CREATE TABLE Albums ("
            "\n accountId INTEGER,"
            "\n userId TEXT,"
            "\n albumId TEXT,"
            "\n photoCount INTEGER,"
            "\n thumbnailUrl TEXT,"
            "\n thumbnailPath TEXT,"
            "\n parentAlbumId TEXT,"
            "\n albumName TEXT,"
            "\n PRIMARY KEY (accountId, userId, albumId),"
            "\n FOREIGN KEY (accountId, userId) REFERENCES Users (accountId, userId) ON DELETE CASCADE);";

    static const char *createPhotosTable =
            "\n CREATE TABLE Photos ("
            "\n accountId INTEGER,"
            "\n userId TEXT,"
            "\n albumId TEXT,"
            "\n photoId TEXT,"
            "\n fileName TEXT,"
            "\n albumPath TEXT,"
            "\n description TEXT,"
            "\n createdTimestamp TEXT,"
            "\n updatedTimestamp TEXT,"
            "\n thumbnailUrl TEXT,"
            "\n thumbnailPath TEXT,"
            "\n imageUrl TEXT,"
            "\n imagePath TEXT,"
            "\n imageWidth INTEGER,"
            "\n imageHeight INTEGER,"
            "\n PRIMARY KEY (accountId, userId, albumId, photoId),"
            "\n FOREIGN KEY (accountId, userId, albumId) REFERENCES Albums (accountId, userId, albumId) ON DELETE CASCADE);";

    static QVector<const char *> retn { createUsersTable, createAlbumsTable, createPhotosTable };
    return retn;
}

QVector<UpgradeOperation> ImageDatabasePrivate::upgradeVersions() const
{
    static const char *upgradeVersion0to1[] = {
        "PRAGMA user_version=1",
        0 // NULL-terminated
    };

    // example for future: upgrading the schema can be done with upgrade version elements as follows:
    //static const char *anotherUpgradeStatement = "\n UPDATE Photos ... etc etc";
    //static const char *upgradeVersion1to2[] = {
    //     anotherUpgradeStatement,
    //     "PRAGMA user_version=2",
    //     0 // NULL-terminated
    //};
    //static bool twiddleSomeDataViaCpp(QSqlDatabase &database) { /* do queries on database... */ return true; }

    static QVector<UpgradeOperation> retn {
        { 0, upgradeVersion0to1 },
    //  { twiddleSomeDataViaCpp, upgradeVersion1to2 },
    };

    return retn;
}

void ImageDatabasePrivate::preTransactionCommit()
{
    // Fixup album thumbnails if required.
    if (!m_deletedPhotos.isEmpty()
            || !m_storedPhotos.isEmpty()) {
        // potentially need to update album thumbnails.
        QSet<QString> doomedAlbums;
        Q_FOREACH (const SyncCache::Album &doomed, m_deletedAlbums) {
            doomedAlbums.insert(constructAlbumIdentifier(doomed.accountId, doomed.userId, doomed.albumId));
        }
        SyncCache::DatabaseError thumbnailError;
        QHash<QString, SyncCache::Album> thumbnailAlbums;
        Q_FOREACH (const SyncCache::Photo &photo, m_deletedPhotos) {
            const QString albumIdentifier = constructAlbumIdentifier(photo.accountId, photo.userId, photo.albumId);
            if (!thumbnailAlbums.contains(albumIdentifier) && !doomedAlbums.contains(albumIdentifier)) {
                SyncCache::Album album = m_imageDbParent->album(photo.accountId, photo.userId, photo.albumId, &thumbnailError);
                if (thumbnailError.errorCode == SyncCache::DatabaseError::NoError && !album.albumId.isEmpty()) {
                    thumbnailAlbums.insert(albumIdentifier, album);
                }
            }
        }
        thumbnailError.errorCode = SyncCache::DatabaseError::NoError;
        Q_FOREACH (const SyncCache::Photo &photo, m_storedPhotos) {
            const QString albumIdentifier = constructAlbumIdentifier(photo.accountId, photo.userId, photo.albumId);
            if (!thumbnailAlbums.contains(albumIdentifier)) {
                SyncCache::Album album = m_imageDbParent->album(photo.accountId, photo.userId, photo.albumId, &thumbnailError);
                if (thumbnailError.errorCode == SyncCache::DatabaseError::NoError && !album.albumId.isEmpty()) {
                    thumbnailAlbums.insert(albumIdentifier, album);
                }
            }
        }
        thumbnailError.errorCode = SyncCache::DatabaseError::NoError;
        Q_FOREACH (const SyncCache::Album &album, thumbnailAlbums) {
            // if the album uses a photo image as its thumbnail
            // and if that photo image is set to be deleted,
            // use another photo's image.
            if (album.thumbnailUrl.isEmpty()
                    && (album.thumbnailPath.isEmpty()
                          || m_filesToDelete.contains(album.thumbnailPath.toString()))) {
                QVector<SyncCache::Photo> photos = m_imageDbParent->photos(album.accountId, album.userId, album.albumId, &thumbnailError);
                Q_FOREACH (const SyncCache::Photo &photo, photos) {
                    if (!photo.imagePath.isEmpty()) {
                        SyncCache::Album updateAlbum = album;
                        updateAlbum.thumbnailPath = photo.imagePath;
                        m_imageDbParent->storeAlbum(updateAlbum, &thumbnailError);
                        break;
                    }
                }
            }
        }
    }
}

void ImageDatabasePrivate::transactionCommittedPreUnlock()
{
    m_tempFilesToDelete = m_filesToDelete;
    m_tempDeletedUsers = m_deletedUsers;
    m_tempDeletedAlbums = m_deletedAlbums;
    m_tempDeletedPhotos = m_deletedPhotos;
    m_tempStoredUsers = m_storedUsers;
    m_tempStoredAlbums = m_storedAlbums;
    m_tempStoredPhotos = m_storedPhotos;

    m_filesToDelete.clear();
    m_deletedUsers.clear();
    m_deletedAlbums.clear();
    m_deletedPhotos.clear();
    m_storedUsers.clear();
    m_storedAlbums.clear();
    m_storedPhotos.clear();
}

void ImageDatabasePrivate::transactionCommittedPostUnlock()
{
    for (const QString &doomed : m_tempFilesToDelete) {
        if (QFile::exists(doomed)) {
            QFile::remove(doomed);
        }
    }

    if (!m_tempDeletedUsers.isEmpty()) {
        emit m_imageDbParent->usersDeleted(m_tempDeletedUsers);
    }
    if (!m_tempDeletedAlbums.isEmpty()) {
        emit m_imageDbParent->albumsDeleted(m_tempDeletedAlbums);
    }
    if (!m_tempDeletedPhotos.isEmpty()) {
        emit m_imageDbParent->photosDeleted(m_tempDeletedPhotos);
    }

    if (!m_tempStoredUsers.isEmpty()) {
        emit m_imageDbParent->usersStored(m_tempStoredUsers);
    }
    if (!m_tempStoredAlbums.isEmpty()) {
        emit m_imageDbParent->albumsStored(m_tempStoredAlbums);
    }
    if (!m_tempStoredPhotos.isEmpty()) {
        emit m_imageDbParent->photosStored(m_tempStoredPhotos);
    }
}

void ImageDatabasePrivate::transactionRolledBackPreUnlocked()
{
    m_filesToDelete.clear();
    m_deletedUsers.clear();
    m_deletedAlbums.clear();
    m_deletedPhotos.clear();
    m_storedUsers.clear();
    m_storedAlbums.clear();
    m_storedPhotos.clear();
}

//-----------------------------------------------------------------------------

ImageDatabase::ImageDatabase(QObject *parent)
    : Database(new ImageDatabasePrivate(this), parent)
{
    qRegisterMetaType<SyncCache::User>();
    qRegisterMetaType<SyncCache::Album>();
    qRegisterMetaType<SyncCache::Photo>();
    qRegisterMetaType<QVector<SyncCache::User> >();
    qRegisterMetaType<QVector<SyncCache::Album> >();
    qRegisterMetaType<QVector<SyncCache::Photo> >();
}

QVector<SyncCache::User> ImageDatabase::users(DatabaseError *error) const
{
    SYNCCACHE_DB_D(const ImageDatabase);

    const QString queryString = QStringLiteral("SELECT accountId, userId, thumbnailUrl, thumbnailPath FROM USERS"
                                               " ORDER BY accountId ASC, userId ASC");

    const QList<QPair<QString, QVariant> > bindValues;

    auto resultHandler = [](DatabaseQuery &selectQuery) -> SyncCache::User {
        User currUser;
        currUser.accountId = selectQuery.value(0).toInt();
        currUser.userId = selectQuery.value(1).toString();
        currUser.thumbnailUrl = QUrl(selectQuery.value(2).toString());
        currUser.thumbnailPath = QUrl(selectQuery.value(3).toString());
        return currUser;
    };

    return DatabaseImpl::fetchMultiple<SyncCache::User>(
            d,
            queryString,
            bindValues,
            resultHandler,
            QStringLiteral("users"),
            error);
}

QVector<SyncCache::Album> ImageDatabase::albums(int accountId, const QString &userId, DatabaseError *error) const
{
    SYNCCACHE_DB_D(const ImageDatabase);

    const QString queryString = QStringLiteral("SELECT albumId, photoCount, thumbnailUrl, thumbnailPath, parentAlbumId, albumName FROM ALBUMS"
                                               " WHERE accountId = :accountId AND userId = :userId"
                                               " ORDER BY accountId ASC, userId ASC, albumId ASC");

    const QList<QPair<QString, QVariant> > bindValues {
        qMakePair<QString, QVariant>(QStringLiteral(":accountId"), accountId),
        qMakePair<QString, QVariant>(QStringLiteral(":userId"), userId)
    };

    auto resultHandler = [accountId, userId](DatabaseQuery &selectQuery) -> SyncCache::Album {
        int whichValue = 0;
        Album currAlbum;
        currAlbum.accountId = accountId;
        currAlbum.userId = userId;
        currAlbum.albumId = selectQuery.value(whichValue++).toString();
        currAlbum.photoCount = selectQuery.value(whichValue++).toInt();
        currAlbum.thumbnailUrl = QUrl(selectQuery.value(whichValue++).toString());
        currAlbum.thumbnailPath = QUrl(selectQuery.value(whichValue++).toString());
        currAlbum.parentAlbumId = selectQuery.value(whichValue++).toString();
        currAlbum.albumName = selectQuery.value(whichValue++).toString();
        return currAlbum;
    };

    return DatabaseImpl::fetchMultiple<SyncCache::Album>(
            d,
            queryString,
            bindValues,
            resultHandler,
            QStringLiteral("albums"),
            error);
}

QVector<SyncCache::Photo> ImageDatabase::photos(int accountId, const QString &userId, const QString &albumId, DatabaseError *error) const
{
    SYNCCACHE_DB_D(const ImageDatabase);

    const QString queryString = albumId.isEmpty()
                              ? QStringLiteral("SELECT albumId, photoId, createdTimestamp, updatedTimestamp, fileName, albumPath, description,"
                                                     " thumbnailUrl, thumbnailPath, imageUrl, imagePath, imageWidth, imageHeight FROM Photos"
                                               " WHERE accountId = :accountId AND userId = :userId"
                                               " ORDER BY accountId ASC, userId ASC, albumId ASC, createdTimestamp DESC")
                              : QStringLiteral("SELECT photoId, createdTimestamp, updatedTimestamp, fileName, albumPath, description,"
                                                     " thumbnailUrl, thumbnailPath, imageUrl, imagePath, imageWidth, imageHeight FROM Photos"
                                               " WHERE accountId = :accountId AND userId = :userId AND albumId = :albumId"
                                               " ORDER BY accountId ASC, userId ASC, albumId ASC, createdTimestamp DESC");

    const QList<QPair<QString, QVariant> > bindValuesA {
        qMakePair<QString, QVariant>(QStringLiteral(":accountId"), accountId),
        qMakePair<QString, QVariant>(QStringLiteral(":userId"), userId)
    };
    const QList<QPair<QString, QVariant> > bindValuesB {
        qMakePair<QString, QVariant>(QStringLiteral(":accountId"), accountId),
        qMakePair<QString, QVariant>(QStringLiteral(":userId"), userId),
        qMakePair<QString, QVariant>(QStringLiteral(":albumId"), albumId)
    };

    auto resultHandler = [accountId, userId, albumId](DatabaseQuery &selectQuery) -> SyncCache::Photo {
        int whichValue = 0;
        Photo currPhoto;
        currPhoto.accountId = accountId;
        currPhoto.userId = userId;
        currPhoto.albumId = albumId.isEmpty() ? selectQuery.value(whichValue++).toString() : albumId;
        currPhoto.photoId = selectQuery.value(whichValue++).toString();
        currPhoto.createdTimestamp = selectQuery.value(whichValue++).toString();
        currPhoto.updatedTimestamp = selectQuery.value(whichValue++).toString();
        currPhoto.fileName = selectQuery.value(whichValue++).toString();
        currPhoto.albumPath = selectQuery.value(whichValue++).toString();
        currPhoto.description = selectQuery.value(whichValue++).toString();
        currPhoto.thumbnailUrl = QUrl(selectQuery.value(whichValue++).toString());
        currPhoto.thumbnailPath = QUrl(selectQuery.value(whichValue++).toString());
        currPhoto.imageUrl = QUrl(selectQuery.value(whichValue++).toString());
        currPhoto.imagePath = QUrl(selectQuery.value(whichValue++).toString());
        return currPhoto;
    };

    return DatabaseImpl::fetchMultiple<SyncCache::Photo>(
            d,
            queryString,
            albumId.isEmpty() ? bindValuesA : bindValuesB,
            resultHandler,
            QStringLiteral("photos"),
            error);
}

User ImageDatabase::user(int accountId, const QString &userId, DatabaseError *error) const
{
    SYNCCACHE_DB_D(const ImageDatabase);

    const QString queryString = QStringLiteral("SELECT thumbnailUrl, thumbnailPath FROM USERS"
                                               " WHERE accountId = :accountId AND userId = :userId");

    const QList<QPair<QString, QVariant> > bindValues {
        qMakePair<QString, QVariant>(QStringLiteral(":accountId"), accountId),
        qMakePair<QString, QVariant>(QStringLiteral(":userId"), userId)
    };

    auto resultHandler = [accountId, userId](DatabaseQuery &selectQuery) -> SyncCache::User {
        User currUser;
        currUser.accountId = accountId;
        currUser.userId = userId;
        currUser.thumbnailUrl = QUrl(selectQuery.value(0).toString());
        currUser.thumbnailPath = QUrl(selectQuery.value(1).toString());
        return currUser;
    };

    return DatabaseImpl::fetch<SyncCache::User>(
            d,
            queryString,
            bindValues,
            resultHandler,
            QStringLiteral("user"),
            error);
}

Album ImageDatabase::album(int accountId, const QString &userId, const QString &albumId, DatabaseError *error) const
{
    SYNCCACHE_DB_D(const ImageDatabase);

    const QString queryString = QStringLiteral("SELECT photoCount, thumbnailUrl, thumbnailPath, parentAlbumId, albumName FROM ALBUMS"
                                               " WHERE accountId = :accountId AND userId = :userId AND albumId = :albumId");

    const QList<QPair<QString, QVariant> > bindValues {
        qMakePair<QString, QVariant>(QStringLiteral(":accountId"), accountId),
        qMakePair<QString, QVariant>(QStringLiteral(":userId"), userId),
        qMakePair<QString, QVariant>(QStringLiteral(":albumId"), albumId)
    };

    auto resultHandler = [accountId, userId, albumId](DatabaseQuery &selectQuery) -> SyncCache::Album {
        int whichValue = 0;
        Album currAlbum;
        currAlbum.accountId = accountId;
        currAlbum.userId = userId;
        currAlbum.albumId = albumId;
        currAlbum.photoCount = selectQuery.value(whichValue++).toInt();
        currAlbum.thumbnailUrl = QUrl(selectQuery.value(whichValue++).toString());
        currAlbum.thumbnailPath = QUrl(selectQuery.value(whichValue++).toString());
        currAlbum.parentAlbumId = selectQuery.value(whichValue++).toString();
        currAlbum.albumName = selectQuery.value(whichValue++).toString();
        return currAlbum;
    };

    return DatabaseImpl::fetch<SyncCache::Album>(
            d,
            queryString,
            bindValues,
            resultHandler,
            QStringLiteral("album"),
            error);
}

Photo ImageDatabase::photo(int accountId, const QString &userId, const QString &albumId, const QString &photoId, DatabaseError *error) const
{
    SYNCCACHE_DB_D(const ImageDatabase);

    const QString queryString = QStringLiteral("SELECT createdTimestamp, updatedTimestamp, fileName, albumPath, description,"
                                               " thumbnailUrl, thumbnailPath, imageUrl, imagePath, imageWidth, imageHeight FROM Photos"
                                               " WHERE accountId = :accountId AND userId = :userId AND albumId = :albumId AND photoId = :photoId");

    const QList<QPair<QString, QVariant> > bindValues {
        qMakePair<QString, QVariant>(QStringLiteral(":accountId"), accountId),
        qMakePair<QString, QVariant>(QStringLiteral(":userId"), userId),
        qMakePair<QString, QVariant>(QStringLiteral(":albumId"), albumId),
        qMakePair<QString, QVariant>(QStringLiteral(":photoId"), photoId)
    };

    auto resultHandler = [accountId, userId, albumId, photoId](DatabaseQuery &selectQuery) -> SyncCache::Photo {
        int whichValue = 0;
        Photo currPhoto;
        currPhoto.accountId = accountId;
        currPhoto.userId = userId;
        currPhoto.albumId = albumId;
        currPhoto.photoId = photoId;
        currPhoto.createdTimestamp = selectQuery.value(whichValue++).toString();
        currPhoto.updatedTimestamp = selectQuery.value(whichValue++).toString();
        currPhoto.fileName = selectQuery.value(whichValue++).toString();
        currPhoto.albumPath = selectQuery.value(whichValue++).toString();
        currPhoto.description = selectQuery.value(whichValue++).toString();
        currPhoto.thumbnailUrl = QUrl(selectQuery.value(whichValue++).toString());
        currPhoto.thumbnailPath = QUrl(selectQuery.value(whichValue++).toString());
        currPhoto.imageUrl = QUrl(selectQuery.value(whichValue++).toString());
        currPhoto.imagePath = QUrl(selectQuery.value(whichValue++).toString());
        return currPhoto;
    };

    return DatabaseImpl::fetch<SyncCache::Photo>(
            d,
            queryString,
            bindValues,
            resultHandler,
            QStringLiteral("photo"),
            error);
}

void ImageDatabase::storeUser(const User &user, DatabaseError *error)
{
    SYNCCACHE_DB_D(ImageDatabase);

    DatabaseError err;
    const User existingUser = this->user(user.accountId, user.userId, &err);
    if (err.errorCode != DatabaseError::NoError) {
        setDatabaseError(error, err.errorCode,
                         QStringLiteral("Error while querying existing user %1 for store: %2")
                                   .arg(user.userId, err.errorMessage));
        return;
    }

    const QString insertString = QStringLiteral("INSERT INTO Users (accountId, userId, thumbnailUrl, thumbnailPath)"
                                                " VALUES(:accountId, :userId, :thumbnailUrl, :thumbnailPath)");
    const QString updateString = QStringLiteral("UPDATE Users SET thumbnailUrl = :thumbnailUrl, thumbnailPath = :thumbnailPath"
                                                " WHERE accountId = :accountId AND userId = :userId");

    const bool insert = existingUser.userId.isEmpty();
    const QString queryString = insert ? insertString : updateString;

    const QList<QPair<QString, QVariant> > bindValues {
        qMakePair<QString, QVariant>(QStringLiteral(":accountId"), user.accountId),
        qMakePair<QString, QVariant>(QStringLiteral(":userId"), user.userId),
        qMakePair<QString, QVariant>(QStringLiteral(":thumbnailUrl"), user.thumbnailUrl),
        qMakePair<QString, QVariant>(QStringLiteral(":thumbnailPath"), user.thumbnailPath)
    };

    auto storeResultHandler = [d, user]() -> void {
        d->m_storedUsers.append(user);
    };

    DatabaseImpl::store<SyncCache::User>(
            d,
            queryString,
            bindValues,
            storeResultHandler,
            QStringLiteral("user"),
            error);
}

void ImageDatabase::storeAlbum(const Album &album, DatabaseError *error)
{
    SYNCCACHE_DB_D(ImageDatabase);

    DatabaseError err;
    const Album existingAlbum = this->album(album.accountId, album.userId, album.albumId, &err);
    if (err.errorCode != DatabaseError::NoError) {
        setDatabaseError(error, err.errorCode,
                         QStringLiteral("Error while querying existing album %1 for store: %2")
                                   .arg(album.albumId, err.errorMessage));
        return;
    }

    const QString insertString = QStringLiteral("INSERT INTO Albums (accountId, userId, albumId, photoCount, thumbnailUrl, thumbnailPath, parentAlbumId, albumName)"
                                                " VALUES(:accountId, :userId, :albumId, :photoCount, :thumbnailUrl, :thumbnailPath, :parentAlbumId, :albumName)");
    const QString updateString = QStringLiteral("UPDATE Albums SET photoCount = :photoCount, thumbnailUrl = :thumbnailUrl, thumbnailPath = :thumbnailPath, "
                                                                  "parentAlbumId = :parentAlbumId, albumName = :albumName"
                                                " WHERE accountId = :accountId AND userId = :userId AND albumId = :albumId");

    const bool insert = existingAlbum.albumId.isEmpty();
    const QString queryString = insert ? insertString : updateString;

    const QList<QPair<QString, QVariant> > bindValues {
        qMakePair<QString, QVariant>(QStringLiteral(":accountId"), album.accountId),
        qMakePair<QString, QVariant>(QStringLiteral(":userId"), album.userId),
        qMakePair<QString, QVariant>(QStringLiteral(":albumId"), album.albumId),
        qMakePair<QString, QVariant>(QStringLiteral(":photoCount"), album.photoCount),
        qMakePair<QString, QVariant>(QStringLiteral(":thumbnailUrl"), album.thumbnailUrl),
        qMakePair<QString, QVariant>(QStringLiteral(":thumbnailPath"), album.thumbnailPath),
        qMakePair<QString, QVariant>(QStringLiteral(":parentAlbumId"), album.parentAlbumId),
        qMakePair<QString, QVariant>(QStringLiteral(":albumName"), album.albumName)
    };

    auto storeResultHandler = [d, album]() -> void {
        d->m_storedAlbums.append(album);
    };

    DatabaseImpl::store<SyncCache::Album>(
            d,
            queryString,
            bindValues,
            storeResultHandler,
            QStringLiteral("album"),
            error);
}

void ImageDatabase::storePhoto(const Photo &photo, DatabaseError *error)
{
    SYNCCACHE_DB_D(ImageDatabase);

    DatabaseError err;
    const Photo existingPhoto = this->photo(photo.accountId, photo.userId, photo.albumId, photo.photoId, &err);
    if (err.errorCode != DatabaseError::NoError) {
        setDatabaseError(error, err.errorCode,
                         QStringLiteral("Error while querying existing photo %1 for store: %2")
                                   .arg(photo.photoId, err.errorMessage));
        return;
    }

    const QString insertString = QStringLiteral("INSERT INTO Photos (accountId, userId, albumId, photoId, createdTimestamp, updatedTimestamp, "
                                                                    "fileName, albumPath, description, thumbnailUrl, thumbnailPath, "
                                                                    "imageUrl, imagePath, imageWidth, imageHeight)"
                                                " VALUES(:accountId, :userId, :albumId, :photoId, :createdTimestamp, :updatedTimestamp, "
                                                        ":fileName, :albumPath, :description, :thumbnailUrl, :thumbnailPath, :imageUrl, :imagePath, :imageWidth, :imageHeight)");
    const QString updateString = QStringLiteral("UPDATE Photos SET createdTimestamp = :createdTimestamp, updatedTimestamp = :updatedTimestamp, "
                                                                  "fileName = :fileName, albumPath = :albumPath, description = :description, "
                                                                  "thumbnailUrl = :thumbnailUrl, thumbnailPath = :thumbnailPath, "
                                                                  "imageUrl = :imageUrl, imagePath = :imagePath, imageWidth = :imageWidth, imageHeight = :imageHeight"
                                                " WHERE accountId = :accountId AND userId = :userId AND albumId = :albumId AND photoId = :photoId");

    const bool insert = existingPhoto.photoId.isEmpty();
    const QString queryString = insert ? insertString : updateString;

    const QList<QPair<QString, QVariant> > bindValues {
        qMakePair<QString, QVariant>(QStringLiteral(":accountId"), photo.accountId),
        qMakePair<QString, QVariant>(QStringLiteral(":userId"), photo.userId),
        qMakePair<QString, QVariant>(QStringLiteral(":albumId"), photo.albumId),
        qMakePair<QString, QVariant>(QStringLiteral(":photoId"), photo.photoId),
        qMakePair<QString, QVariant>(QStringLiteral(":createdTimestamp"), photo.createdTimestamp),
        qMakePair<QString, QVariant>(QStringLiteral(":updatedTimestamp"), photo.updatedTimestamp),
        qMakePair<QString, QVariant>(QStringLiteral(":fileName"), photo.fileName),
        qMakePair<QString, QVariant>(QStringLiteral(":albumPath"), photo.albumPath),
        qMakePair<QString, QVariant>(QStringLiteral(":description"), photo.description),
        qMakePair<QString, QVariant>(QStringLiteral(":thumbnailUrl"), photo.thumbnailUrl),
        qMakePair<QString, QVariant>(QStringLiteral(":thumbnailPath"), photo.thumbnailPath),
        qMakePair<QString, QVariant>(QStringLiteral(":imageUrl"), photo.imageUrl),
        qMakePair<QString, QVariant>(QStringLiteral(":imagePath"), photo.imagePath),
        qMakePair<QString, QVariant>(QStringLiteral(":imageWidth"), photo.imageWidth),
        qMakePair<QString, QVariant>(QStringLiteral(":imageHeight"), photo.imageHeight)
    };

    auto storeResultHandler = [d, photo, existingPhoto, insert]() -> void {
        d->m_storedPhotos.append(photo);
        if (!insert) {
            if (!existingPhoto.imagePath.isEmpty()
                    && existingPhoto.imagePath != photo.imagePath) {
                d->m_filesToDelete.append(existingPhoto.imagePath.toString());
            }
            if (!existingPhoto.thumbnailPath.isEmpty()
                    && existingPhoto.thumbnailPath != photo.thumbnailPath) {
                d->m_filesToDelete.append(existingPhoto.thumbnailPath.toString());
            }
        }
    };

    DatabaseImpl::store<SyncCache::Photo>(
            d,
            queryString,
            bindValues,
            storeResultHandler,
            QStringLiteral("photo"),
            error);
}

void ImageDatabase::deleteUser(const User &user, DatabaseError *error)
{
    SYNCCACHE_DB_D(ImageDatabase);

    DatabaseError err;
    const User existingUser = this->user(user.accountId, user.userId, &err);
    if (err.errorCode != DatabaseError::NoError) {
        setDatabaseError(error, err.errorCode,
                         QStringLiteral("Error while querying existing user %1 for delete: %2")
                                   .arg(user.userId, err.errorMessage));
        return;
    }

    if (existingUser.userId.isEmpty()) {
        // does not exist in database, already deleted.
        return;
    }

    // we need to delete all albums for this user, to ensure artifacts (e.g. downloaded files) are also deleted.
    QVector<SyncCache::Album> userAlbums = this->albums(user.accountId, user.userId, &err);
    if (err.errorCode != DatabaseError::NoError) {
        setDatabaseError(error, err.errorCode,
                         QStringLiteral("Error while querying albums for user %1 for delete: %2")
                                   .arg(user.userId, err.errorMessage));
        return;
    }

    auto deleteRelatedValues = [this, userAlbums] (DatabaseError *error) -> void {
        DatabaseError err;
        for (const Album &doomed : userAlbums) {
            this->deleteAlbum(doomed, &err);
            if (err.errorCode != DatabaseError::NoError) {
                setDatabaseError(error, err.errorCode,
                                 QStringLiteral("Error while deleting album %1 for user %2: %3")
                                           .arg(doomed.albumId, doomed.userId, err.errorMessage));
                return;
            }
        }
    };

    const QString queryString = QStringLiteral("DELETE FROM Users"
                                               " WHERE accountId = :accountId AND userId = :userId");

    const QList<QPair<QString, QVariant> > bindValues {
        qMakePair<QString, QVariant>(QStringLiteral(":accountId"), user.accountId),
        qMakePair<QString, QVariant>(QStringLiteral(":userId"), user.userId)
    };

    auto deleteResultHandler = [d, existingUser]() -> void {
        d->m_deletedUsers.append(existingUser);
        if (!existingUser.thumbnailPath.isEmpty()) {
            d->m_filesToDelete.append(existingUser.thumbnailPath.toString());
        }
    };

    DatabaseImpl::deleteValue<SyncCache::User>(
            d,
            deleteRelatedValues,
            queryString,
            bindValues,
            deleteResultHandler,
            QStringLiteral("user"),
            error);
}

void ImageDatabase::deleteAlbum(const Album &album, DatabaseError *error)
{
    SYNCCACHE_DB_D(ImageDatabase);

    DatabaseError err;
    const Album existingAlbum = this->album(album.accountId, album.userId, album.albumId, &err);
    if (err.errorCode != DatabaseError::NoError) {
        setDatabaseError(error, err.errorCode,
                         QStringLiteral("Error while querying existing album %1 for delete: %2")
                                   .arg(album.albumId, err.errorMessage));
        return;
    }

    if (existingAlbum.albumId.isEmpty()) {
        // does not exist in database, already deleted.
        return;
    }

    // we need to delete all photos in this album, to ensure artifacts (e.g. downloaded files) are also deleted.
    QVector<SyncCache::Photo> albumPhotos = this->photos(album.accountId, album.userId, album.albumId, &err);
    if (err.errorCode != DatabaseError::NoError) {
        setDatabaseError(error, err.errorCode,
                         QStringLiteral("Error while querying photos from album %1 for delete: %2")
                                   .arg(album.albumId, err.errorMessage));
        return;
    }

    auto deleteRelatedValues = [this, albumPhotos] (DatabaseError *error) -> void {
        DatabaseError err;
        for (const Photo &doomed : albumPhotos) {
            this->deletePhoto(doomed, &err);
            if (err.errorCode != DatabaseError::NoError) {
                setDatabaseError(error, err.errorCode,
                                 QStringLiteral("Error while deleting photo %1 from album %2: %3")
                                           .arg(doomed.photoId, doomed.albumId, err.errorMessage));
                return;
            }
        }
    };

    const QString queryString = QStringLiteral("DELETE FROM Albums"
                                               " WHERE accountId = :accountId AND userId = :userId AND albumId = :albumId");

    const QList<QPair<QString, QVariant> > bindValues {
        qMakePair<QString, QVariant>(QStringLiteral(":accountId"), album.accountId),
        qMakePair<QString, QVariant>(QStringLiteral(":userId"), album.userId),
        qMakePair<QString, QVariant>(QStringLiteral(":albumId"), album.albumId)
    };

    auto deleteResultHandler = [d, existingAlbum]() -> void {
        d->m_deletedAlbums.append(existingAlbum);
        if (!existingAlbum.thumbnailUrl.isEmpty() && !existingAlbum.thumbnailPath.isEmpty()) {
            // only delete the thumbnail if it was downloaded especially from url,
            // but not if it is from a photo (in that case, thumbnailUrl will be empty).
            d->m_filesToDelete.append(existingAlbum.thumbnailPath.toString());
        }
    };

    DatabaseImpl::deleteValue<SyncCache::Album>(
            d,
            deleteRelatedValues,
            queryString,
            bindValues,
            deleteResultHandler,
            QStringLiteral("album"),
            error);
}

void ImageDatabase::deletePhoto(const Photo &photo, DatabaseError *error)
{
    SYNCCACHE_DB_D(ImageDatabase);

    DatabaseError err;
    const Photo existingPhoto = this->photo(photo.accountId, photo.userId, photo.albumId, photo.photoId, &err);
    if (err.errorCode != DatabaseError::NoError) {
        setDatabaseError(error, err.errorCode,
                         QStringLiteral("Error while querying existing photo %1 for delete: %2")
                                   .arg(photo.photoId, err.errorMessage));
        return;
    }

    if (existingPhoto.photoId.isEmpty()) {
        // does not exist in database, already deleted.
        return;
    }

    auto deleteRelatedValues = [] (DatabaseError *) -> void { };

    const QString queryString = QStringLiteral("DELETE FROM Photos"
                                               " WHERE accountId = :accountId AND userId = :userId AND albumId = :albumId AND photoId = :photoId");

    const QList<QPair<QString, QVariant> > bindValues {
        qMakePair<QString, QVariant>(QStringLiteral(":accountId"), photo.accountId),
        qMakePair<QString, QVariant>(QStringLiteral(":userId"), photo.userId),
        qMakePair<QString, QVariant>(QStringLiteral(":albumId"), photo.albumId),
        qMakePair<QString, QVariant>(QStringLiteral(":photoId"), photo.albumId)
    };

    auto deleteResultHandler = [d, existingPhoto]() -> void {
        d->m_deletedPhotos.append(existingPhoto);
        if (!existingPhoto.thumbnailPath.isEmpty()) {
            d->m_filesToDelete.append(existingPhoto.thumbnailPath.toString());
        }
        if (!existingPhoto.imagePath.isEmpty()) {
            d->m_filesToDelete.append(existingPhoto.imagePath.toString());
        }
    };

    DatabaseImpl::deleteValue<SyncCache::Photo>(
            d,
            deleteRelatedValues,
            queryString,
            bindValues,
            deleteResultHandler,
            QStringLiteral("photo"),
            error);
}
