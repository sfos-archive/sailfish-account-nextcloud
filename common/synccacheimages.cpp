/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "synccacheimages.h"
#include "synccacheimages_p.h"
#include "synccacheimagedownloads_p.h"

#include <QtCore/QThread>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtCore/QDebug>

using namespace SyncCache;

User& User::operator=(const User &other)
{
    if (this == &other) {
        return *this;
    }

    accountId = other.accountId;
    userId = other.userId;
    displayName = other.displayName;
    thumbnailUrl = other.thumbnailUrl;
    thumbnailPath = other.thumbnailPath;
    thumbnailFileName = other.thumbnailFileName;

    return *this;
}

Album& Album::operator=(const Album &other)
{
    if (this == &other) {
        return *this;
    }

    accountId = other.accountId;
    userId = other.userId;
    albumId = other.albumId;
    parentAlbumId = other.parentAlbumId;
    albumName = other.albumName;
    photoCount = other.photoCount;
    thumbnailUrl = other.thumbnailUrl;
    thumbnailPath = other.thumbnailPath;
    thumbnailFileName = other.thumbnailFileName;

    return *this;
}

Photo& Photo::operator=(const Photo &other)
{
    if (this == &other) {
        return *this;
    }

    accountId = other.accountId;
    userId = other.userId;
    albumId = other.albumId;
    photoId = other.photoId;
    createdTimestamp = other.createdTimestamp;
    updatedTimestamp = other.updatedTimestamp;
    fileName = other.fileName;
    albumPath = other.albumPath;
    description = other.description;
    thumbnailUrl = other.thumbnailUrl;
    thumbnailPath = other.thumbnailPath;
    imageUrl = other.imageUrl;
    imagePath = other.imagePath;
    imageWidth = other.imageWidth;
    imageHeight = other.imageHeight;
    fileSize = other.fileSize;
    fileType = other.fileType;

    return *this;
}

//-----------------------------------------------------------------------------

ImageCacheThreadWorker::ImageCacheThreadWorker(QObject *parent)
    : QObject(parent), m_downloader(nullptr)
{
}

ImageCacheThreadWorker::~ImageCacheThreadWorker()
{
}

void ImageCacheThreadWorker::openDatabase(const QString &accountType)
{
    DatabaseError error;
    m_db.openDatabase(
            QStringLiteral("%1/%2.db").arg(ImageCache::imageCacheRootDir(), accountType),
            &error);
    if (error.errorCode != DatabaseError::NoError) {
        emit openDatabaseFailed(error.errorMessage);
    } else {
        connect(&m_db, &ImageDatabase::usersStored,
                this, &ImageCacheThreadWorker::usersStored);
        connect(&m_db, &ImageDatabase::albumsStored,
                this, &ImageCacheThreadWorker::albumsStored);
        connect(&m_db, &ImageDatabase::photosStored,
                this, &ImageCacheThreadWorker::photosStored);
        connect(&m_db, &ImageDatabase::usersDeleted,
                this, &ImageCacheThreadWorker::usersDeleted);
        connect(&m_db, &ImageDatabase::albumsDeleted,
                this, &ImageCacheThreadWorker::albumsDeleted);
        connect(&m_db, &ImageDatabase::photosDeleted,
                this, &ImageCacheThreadWorker::photosDeleted);
        emit openDatabaseFinished();
    }
}

void ImageCacheThreadWorker::requestUsers()
{
    DatabaseError error;
    QVector<SyncCache::User> users = m_db.users(&error);
    if (error.errorCode != DatabaseError::NoError) {
        emit requestUsersFailed(error.errorMessage);
    } else {
        emit requestUsersFinished(users);
    }
}

void ImageCacheThreadWorker::requestUser(int accountId, const QString &userId)
{
    DatabaseError error;
    SyncCache::User user = m_db.user(accountId, userId, &error);
    if (error.errorCode != DatabaseError::NoError) {
        emit requestUserFailed(accountId, userId, error.errorMessage);
    } else {
        emit requestUserFinished(accountId, userId, user);
    }
}

void ImageCacheThreadWorker::requestAlbums(int accountId, const QString &userId)
{
    DatabaseError error;
    QVector<SyncCache::Album> albums = m_db.albums(accountId, userId, &error);
    if (error.errorCode != DatabaseError::NoError) {
        emit requestAlbumsFailed(accountId, userId, error.errorMessage);
    } else {
        emit requestAlbumsFinished(accountId, userId, albums);
    }
}

void ImageCacheThreadWorker::requestPhotos(int accountId, const QString &userId, const QString &albumId)
{
    DatabaseError error;
    QVector<SyncCache::Photo> photos = m_db.photos(accountId, userId, albumId, &error);
    if (error.errorCode != DatabaseError::NoError) {
        emit requestPhotosFailed(accountId, userId, albumId, error.errorMessage);
    } else {
        emit requestPhotosFinished(accountId, userId, albumId, photos);
    }
}

void ImageCacheThreadWorker::requestPhotoCount()
{
    DatabaseError error;
    SyncCache::PhotoCounter photoCounter = m_db.photoCount(&error);
    if (error.errorCode != DatabaseError::NoError) {
        emit requestPhotoCountFailed(error.errorMessage);
    } else {
        emit requestPhotoCountFinished(photoCounter.count);
    }
}

void ImageCacheThreadWorker::populateUserThumbnail(int idempToken, int accountId, const QString &userId, const QNetworkRequest &requestTemplate)
{
    DatabaseError error;
    User user = m_db.user(accountId, userId, &error);
    if (error.errorCode != DatabaseError::NoError) {
        emit populateUserThumbnailFailed(idempToken, QStringLiteral("Error occurred while reading user %1 info from db: %2")
                                         .arg(userId)
                                         .arg(error.errorMessage));
        return;
    }

    // the thumbnail already exists.
    const QString thumbnailPath = user.thumbnailPath.toString();
    if (!thumbnailPath.isEmpty() && QFile::exists(thumbnailPath)) {
        emit populateUserThumbnailFinished(idempToken, thumbnailPath);
        return;
    }

    // download the thumbnail.
    if (user.thumbnailUrl.isEmpty()) {
        emit populateUserThumbnailFailed(idempToken, QStringLiteral("Empty thumbnail url specified for user %1").arg(userId));
        return;
    }
    if (!m_downloader) {
        m_downloader = new ImageDownloader(this);
    }

    ImageDownloadWatcher *watcher = m_downloader->downloadImage(
                idempToken,
                user.thumbnailUrl,
                user.thumbnailFileName,
                SyncCache::userImageDownloadDir(accountId, userId, true),
                requestTemplate);
    connect(watcher, &ImageDownloadWatcher::downloadFailed, this, [this, watcher, idempToken] (const QString &errorMessage) {
        emit populateUserThumbnailFailed(idempToken, errorMessage);
        watcher->deleteLater();
    });
    connect(watcher, &ImageDownloadWatcher::downloadFinished, this, [this, watcher, user, idempToken] (const QUrl &filePath) {
        // the file has been downloaded to disk.  attempt to update the database.
        DatabaseError storeError;
        User userToStore = user;
        userToStore.thumbnailPath = filePath;
        m_db.storeUser(userToStore, &storeError);
        if (storeError.errorCode != DatabaseError::NoError) {
            QFile::remove(filePath.toString());
            emit populateUserThumbnailFailed(idempToken, storeError.errorMessage);
        } else {
            emit populateUserThumbnailFinished(idempToken, filePath.toString());
        }
        watcher->deleteLater();
    });
}

void ImageCacheThreadWorker::populateAlbumThumbnail(int idempToken, int accountId, const QString &userId, const QString &albumId, const QNetworkRequest &requestTemplate)
{
    DatabaseError error;
    Album album = m_db.album(accountId, userId, albumId, &error);
    if (error.errorCode != DatabaseError::NoError) {
        emit populateAlbumThumbnailFailed(idempToken, QStringLiteral("Error occurred while reading album %1 info from db: %2")
                                          .arg(albumId)
                                          .arg(error.errorMessage));
        return;
    }

    // the thumbnail already exists.
    const QString thumbnailPath = album.thumbnailPath.toString();
    if (!thumbnailPath.isEmpty() && QFile::exists(thumbnailPath)) {
        emit populateAlbumThumbnailFinished(idempToken, thumbnailPath);
        return;
    }

    // download the thumbnail.
    if (album.thumbnailUrl.isEmpty()) {
        emit populateAlbumThumbnailFailed(idempToken, QStringLiteral("Empty thumbnail url specified for album %1").arg(albumId));
        return;
    }
    if (!m_downloader) {
        m_downloader = new ImageDownloader(this);
    }
    ImageDownloadWatcher *watcher = m_downloader->downloadImage(
                idempToken,
                album.thumbnailUrl,
                album.thumbnailFileName,
                SyncCache::albumImageDownloadDir(accountId, album.albumName, true),
                requestTemplate);
    connect(watcher, &ImageDownloadWatcher::downloadFailed, this, [this, watcher, idempToken] (const QString &errorMessage) {
        emit populateAlbumThumbnailFailed(idempToken, errorMessage);
        watcher->deleteLater();
    });
    connect(watcher, &ImageDownloadWatcher::downloadFinished, this, [this, watcher, album, idempToken] (const QUrl &filePath) {
        // the file has been downloaded to disk.  attempt to update the database.
        DatabaseError storeError;
        Album albumToStore = album;
        albumToStore.thumbnailPath = filePath;
        m_db.storeAlbum(albumToStore, &storeError);
        if (storeError.errorCode != DatabaseError::NoError) {
            QFile::remove(filePath.toString());
            emit populateAlbumThumbnailFailed(idempToken, storeError.errorMessage);
        } else {
            emit populateAlbumThumbnailFinished(idempToken, filePath.toString());
        }
        watcher->deleteLater();
    });
}

void ImageCacheThreadWorker::populatePhotoThumbnail(int idempToken, int accountId, const QString &userId, const QString &albumId, const QString &photoId, const QNetworkRequest &requestTemplate)
{
    DatabaseError error;
    Photo photo = m_db.photo(accountId, userId, albumId, photoId, &error);
    if (error.errorCode != DatabaseError::NoError) {
        emit populatePhotoThumbnailFailed(idempToken, QStringLiteral("Error occurred while reading photo %1 info from db: %2")
                                          .arg(photoId)
                                          .arg(error.errorMessage));
        return;
    }

    // the thumbnail already exists.
    QString thumbnailPath = photo.thumbnailPath.toString();
    if (!thumbnailPath.isEmpty() && QFile::exists(thumbnailPath)) {
        emit populatePhotoThumbnailFinished(idempToken, thumbnailPath);
        return;
    }

    // download the thumbnail.
    if (photo.thumbnailUrl.isEmpty()) {
        emit populatePhotoThumbnailFailed(idempToken, QStringLiteral("Empty thumbnail url specified for photo %1").arg(photoId));
        return;
    }

    // the thumbnail was already downloaded as the thumbnail for the album.
    const QString &thumbnailDirPath = SyncCache::albumImageDownloadDir(accountId, photo.albumPath, true);
    thumbnailPath = thumbnailDirPath + "/" + photo.fileName;
    if (QFile::exists(thumbnailPath)) {
        photoThumbnailDownloadFinished(idempToken, photo, QUrl(thumbnailPath));
        return;
    }

    if (!m_thumbnailDownloader) {
        m_thumbnailDownloader = new BatchedImageDownloader(photo.thumbnailUrl, requestTemplate, this);
    }

    ImageDownloadWatcher *watcher = m_thumbnailDownloader->downloadImage(
            idempToken,
            photo.photoId,
            photo.fileName,
            thumbnailDirPath);
    connect(watcher, &ImageDownloadWatcher::downloadFailed, this, [this, watcher, idempToken] (const QString &errorMessage) {
        emit populatePhotoThumbnailFailed(idempToken, errorMessage);
        watcher->deleteLater();
    });
    connect(watcher, &ImageDownloadWatcher::downloadFinished, this, [this, watcher, photo, idempToken] (const QUrl &filePath) {
        // the file has been downloaded to disk.  attempt to update the database.
        photoThumbnailDownloadFinished(idempToken, photo, filePath);
        watcher->deleteLater();
    });
}

void ImageCacheThreadWorker::photoThumbnailDownloadFinished(int idempToken, const SyncCache::Photo &photo, const QUrl &filePath)
{
    DatabaseError storeError;
    Photo photoToStore = photo;
    photoToStore.thumbnailPath = filePath;
    m_db.storePhoto(photoToStore, &storeError);

    if (storeError.errorCode != DatabaseError::NoError) {
        QFile::remove(filePath.toString());
        emit populatePhotoThumbnailFailed(idempToken, storeError.errorMessage);
    } else {
        emit populatePhotoThumbnailFinished(idempToken, filePath.toString());
    }
}

void ImageCacheThreadWorker::populatePhotoImage(int idempToken, int accountId, const QString &userId, const QString &albumId, const QString &photoId, const QNetworkRequest &requestTemplate)
{
    DatabaseError error;
    Photo photo = m_db.photo(accountId, userId, albumId, photoId, &error);
    if (error.errorCode != DatabaseError::NoError) {
        emit populatePhotoImageFailed(idempToken, QStringLiteral("Error occurred while reading photo info from db: %1").arg(error.errorMessage));
        return;
    }

    // the image already exists.
    const QString imagePath = photo.imagePath.toString();
    if (!imagePath.isEmpty() && QFile::exists(imagePath)) {
        emit populatePhotoImageFinished(idempToken, imagePath);
        return;
    }

    // download the image.
    if (photo.imageUrl.isEmpty()) {
        emit populatePhotoImageFailed(idempToken, QStringLiteral("Empty image url specified for photo %1").arg(photoId));
        return;
    }

    if (!m_downloader) {
        m_downloader = new ImageDownloader(this);
    }

    ImageDownloadWatcher *watcher = m_downloader->downloadImage(
                idempToken,
                photo.imageUrl,
                photo.fileName,
                SyncCache::albumImageDownloadDir(accountId, photo.albumPath, false),
                requestTemplate);

    connect(watcher, &ImageDownloadWatcher::downloadFailed, this, [this, watcher, idempToken] (const QString &errorMessage) {
        emit populatePhotoImageFailed(idempToken, errorMessage);
        watcher->deleteLater();
    });

    connect(watcher, &ImageDownloadWatcher::downloadFinished, this, [this, watcher, photo, idempToken] (const QUrl &filePath) {
        // the file has been downloaded to disk.  attempt to update the database.
        DatabaseError storeError;
        Photo photoToStore = photo;
        photoToStore.imagePath = filePath;
        m_db.storePhoto(photoToStore, &storeError);

        if (storeError.errorCode != DatabaseError::NoError) {
            QFile::remove(filePath.toString());
            emit populatePhotoImageFailed(idempToken, storeError.errorMessage);
        } else {
            emit populatePhotoImageFinished(idempToken, filePath.toString());
        }
        watcher->deleteLater();
    });
}

//-----------------------------------------------------------------------------

ImageCachePrivate::ImageCachePrivate(ImageCache *parent)
    : QObject(parent), m_worker(new ImageCacheThreadWorker)
{
    qRegisterMetaType<SyncCache::User>();
    qRegisterMetaType<SyncCache::Album>();
    qRegisterMetaType<SyncCache::Photo>();
    qRegisterMetaType<QVector<SyncCache::User> >();
    qRegisterMetaType<QVector<SyncCache::User> >();
    qRegisterMetaType<QVector<SyncCache::User> >();

    m_worker->moveToThread(&m_dbThread);
    connect(&m_dbThread, &QThread::finished, m_worker, &QObject::deleteLater);

    connect(this, &ImageCachePrivate::openDatabase, m_worker, &ImageCacheThreadWorker::openDatabase);
    connect(this, &ImageCachePrivate::requestUser, m_worker, &ImageCacheThreadWorker::requestUser);
    connect(this, &ImageCachePrivate::requestUsers, m_worker, &ImageCacheThreadWorker::requestUsers);
    connect(this, &ImageCachePrivate::requestAlbums, m_worker, &ImageCacheThreadWorker::requestAlbums);
    connect(this, &ImageCachePrivate::requestPhotos, m_worker, &ImageCacheThreadWorker::requestPhotos);
    connect(this, &ImageCachePrivate::requestPhotoCount, m_worker, &ImageCacheThreadWorker::requestPhotoCount);

    connect(this, &ImageCachePrivate::populateUserThumbnail, m_worker, &ImageCacheThreadWorker::populateUserThumbnail);
    connect(this, &ImageCachePrivate::populateAlbumThumbnail, m_worker, &ImageCacheThreadWorker::populateAlbumThumbnail);
    connect(this, &ImageCachePrivate::populatePhotoThumbnail, m_worker, &ImageCacheThreadWorker::populatePhotoThumbnail);
    connect(this, &ImageCachePrivate::populatePhotoImage, m_worker, &ImageCacheThreadWorker::populatePhotoImage);

    connect(m_worker, &ImageCacheThreadWorker::openDatabaseFailed, parent, &ImageCache::openDatabaseFailed);
    connect(m_worker, &ImageCacheThreadWorker::openDatabaseFinished, parent, &ImageCache::openDatabaseFinished);
    connect(m_worker, &ImageCacheThreadWorker::requestUserFailed, parent, &ImageCache::requestUserFailed);
    connect(m_worker, &ImageCacheThreadWorker::requestUserFinished, parent, &ImageCache::requestUserFinished);
    connect(m_worker, &ImageCacheThreadWorker::requestUsersFailed, parent, &ImageCache::requestUsersFailed);
    connect(m_worker, &ImageCacheThreadWorker::requestUsersFinished, parent, &ImageCache::requestUsersFinished);
    connect(m_worker, &ImageCacheThreadWorker::requestAlbumsFailed, parent, &ImageCache::requestAlbumsFailed);
    connect(m_worker, &ImageCacheThreadWorker::requestAlbumsFinished, parent, &ImageCache::requestAlbumsFinished);
    connect(m_worker, &ImageCacheThreadWorker::requestPhotosFailed, parent, &ImageCache::requestPhotosFailed);
    connect(m_worker, &ImageCacheThreadWorker::requestPhotosFinished, parent, &ImageCache::requestPhotosFinished);
    connect(m_worker, &ImageCacheThreadWorker::requestPhotoCountFailed, parent, &ImageCache::requestPhotoCountFailed);
    connect(m_worker, &ImageCacheThreadWorker::requestPhotoCountFinished, parent, &ImageCache::requestPhotoCountFinished);

    connect(m_worker, &ImageCacheThreadWorker::populateUserThumbnailFailed, parent, &ImageCache::populateUserThumbnailFailed);
    connect(m_worker, &ImageCacheThreadWorker::populateUserThumbnailFinished, parent, &ImageCache::populateUserThumbnailFinished);
    connect(m_worker, &ImageCacheThreadWorker::populateAlbumThumbnailFailed, parent, &ImageCache::populateAlbumThumbnailFailed);
    connect(m_worker, &ImageCacheThreadWorker::populateAlbumThumbnailFinished, parent, &ImageCache::populateAlbumThumbnailFinished);
    connect(m_worker, &ImageCacheThreadWorker::populatePhotoThumbnailFailed, parent, &ImageCache::populatePhotoThumbnailFailed);
    connect(m_worker, &ImageCacheThreadWorker::populatePhotoThumbnailFinished, parent, &ImageCache::populatePhotoThumbnailFinished);
    connect(m_worker, &ImageCacheThreadWorker::populatePhotoImageFailed, parent, &ImageCache::populatePhotoImageFailed);
    connect(m_worker, &ImageCacheThreadWorker::populatePhotoImageFinished, parent, &ImageCache::populatePhotoImageFinished);

    connect(m_worker, &ImageCacheThreadWorker::usersStored, parent, &ImageCache::usersStored);
    connect(m_worker, &ImageCacheThreadWorker::albumsStored, parent, &ImageCache::albumsStored);
    connect(m_worker, &ImageCacheThreadWorker::photosStored, parent, &ImageCache::photosStored);
    connect(m_worker, &ImageCacheThreadWorker::usersDeleted, parent, &ImageCache::usersDeleted);
    connect(m_worker, &ImageCacheThreadWorker::albumsDeleted, parent, &ImageCache::albumsDeleted);
    connect(m_worker, &ImageCacheThreadWorker::photosDeleted, parent, &ImageCache::photosDeleted);

    m_dbThread.start();
    m_dbThread.setPriority(QThread::IdlePriority);
}

ImageCachePrivate::~ImageCachePrivate()
{
    m_dbThread.quit();
    m_dbThread.wait();
}

//-----------------------------------------------------------------------------

ImageCache::ImageCache(QObject *parent)
    : QObject(parent), d_ptr(new ImageCachePrivate(this))
{
}

ImageCache::~ImageCache()
{
}

QString ImageCache::imageCacheDir(int accountId)
{
    return SyncCache::imageDownloadDir(accountId);
}

QString ImageCache::imageCacheRootDir()
{
    return QStringLiteral("%1/system/privileged/Images").arg(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
}

void ImageCache::openDatabase(const QString &databaseFile)
{
    Q_D(ImageCache);
    emit d->openDatabase(databaseFile);
}

void ImageCache::requestUser(int accountId, const QString &userId)
{
    Q_D(ImageCache);
    emit d->requestUser(accountId, userId);
}

void ImageCache::requestUsers()
{
    Q_D(ImageCache);
    emit d->requestUsers();
}

void ImageCache::requestAlbums(int accountId, const QString &userId)
{
    Q_D(ImageCache);
    emit d->requestAlbums(accountId, userId);
}

void ImageCache::requestPhotos(int accountId, const QString &userId, const QString &albumId)
{
    Q_D(ImageCache);
    emit d->requestPhotos(accountId, userId, albumId);
}

void ImageCache::requestPhotoCount()
{
    Q_D(ImageCache);
    emit d->requestPhotoCount();
}

void ImageCache::populateUserThumbnail(int idempToken, int accountId, const QString &userId, const QNetworkRequest &requestTemplate)
{
    Q_D(ImageCache);
    emit d->populateUserThumbnail(idempToken, accountId, userId, requestTemplate);
}

void ImageCache::populateAlbumThumbnail(int idempToken, int accountId, const QString &userId, const QString &albumId, const QNetworkRequest &requestTemplate)
{
    Q_D(ImageCache);
    emit d->populateAlbumThumbnail(idempToken, accountId, userId, albumId, requestTemplate);
}

void ImageCache::populatePhotoThumbnail(int idempToken, int accountId, const QString &userId, const QString &albumId, const QString &photoId, const QNetworkRequest &requestTemplate)
{
    Q_D(ImageCache);
    emit d->populatePhotoThumbnail(idempToken, accountId, userId, albumId, photoId, requestTemplate);
}

void ImageCache::populatePhotoImage(int idempToken, int accountId, const QString &userId, const QString &albumId, const QString &photoId, const QNetworkRequest &requestTemplate)
{
    Q_D(ImageCache);
    emit d->populatePhotoImage(idempToken, accountId, userId, albumId, photoId, requestTemplate);
}
