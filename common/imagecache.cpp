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

#include <QtCore/QThread>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>

using namespace SyncCache;

namespace {
    QString privilegedImagesDirPath() {
        return QStringLiteral("%1/system/privileged/Images").arg(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
    }
}

//-----------------------------------------------------------------------------

ImageDownloadWatcher::ImageDownloadWatcher(int idempToken, const QUrl &imageUrl, QObject *parent)
    : QObject(parent), m_idempToken(idempToken), m_imageUrl(imageUrl)
{
}

ImageDownloadWatcher::~ImageDownloadWatcher()
{
}

int ImageDownloadWatcher::idempToken() const
{
    return m_idempToken;
}

QUrl ImageDownloadWatcher::imageUrl() const
{
    return m_imageUrl;
}

//-----------------------------------------------------------------------------

ImageDownload::ImageDownload(
        int idempToken,
        const QUrl &imageUrl,
        const QString &fileName,
        const QString &albumPath,
        const QNetworkRequest &templateRequest,
        ImageDownloadWatcher *watcher)
    : m_idempToken(idempToken)
    , m_imageUrl(imageUrl)
    , m_fileName(fileName)
    , m_albumPath(albumPath)
    , m_templateRequest(templateRequest)
    , m_watcher(watcher)
{
    m_timeoutTimer = new QTimer;
}

ImageDownload::~ImageDownload()
{
    m_timeoutTimer->deleteLater();
    if (m_reply) {
        m_reply->deleteLater();
    }
}

//-----------------------------------------------------------------------------

ImageDownloader::ImageDownloader(int maxActive, QObject *parent)
    : QObject(parent), m_maxActive(maxActive > 0 && maxActive < 20 ? maxActive : 4), m_imageDirectory(QStringLiteral("Nextcloud"))
{
}

ImageDownloader::~ImageDownloader()
{
}

void ImageDownloader::setImageDirectory(const QString &path)
{
    m_imageDirectory = path;
}

ImageDownloadWatcher *ImageDownloader::downloadImage(int idempToken, const QUrl &imageUrl, const QString &fileName, const QString &albumPath, const QNetworkRequest &templateRequest)
{
    ImageDownloadWatcher *watcher = new ImageDownloadWatcher(idempToken, imageUrl, this);
    ImageDownload *download = new ImageDownload(idempToken, imageUrl, fileName, albumPath, templateRequest, watcher);
    m_pending.enqueue(download);
    QMetaObject::invokeMethod(this, "triggerDownload", Qt::QueuedConnection);
    return watcher; // caller takes ownership.
}

void ImageDownloader::triggerDownload()
{
    while (m_active.size() && (m_active.head() == Q_NULLPTR || !m_active.head()->m_timeoutTimer->isActive())) {
        delete m_active.dequeue();
    }

    while (m_active.size() < m_maxActive && m_pending.size()) {
        ImageDownload *download = m_pending.dequeue();
        m_active.enqueue(download);

        connect(download->m_timeoutTimer, &QTimer::timeout, this, [this, download] {
            if (download->m_watcher) {
                emit download->m_watcher->downloadFailed(QStringLiteral("Image download timed out"));
            }
            eraseActiveDownload(download);
        });

        download->m_timeoutTimer->setInterval(20 * 1000);
        download->m_timeoutTimer->setSingleShot(true);
        download->m_timeoutTimer->start();

        QNetworkRequest request(download->m_templateRequest);
        QUrl imageUrl(download->m_imageUrl);
        imageUrl.setUserName(download->m_templateRequest.url().userName());
        imageUrl.setPassword(download->m_templateRequest.url().password());
        request.setUrl(imageUrl);
        QNetworkReply *reply = m_qnam.get(request);
        download->m_reply = reply;
        if (reply) {
            connect(reply, &QNetworkReply::finished, this, [this, reply, download] {
                if (reply->error() != QNetworkReply::NoError) {
                    if (download->m_watcher) {
                        emit download->m_watcher->downloadFailed(QStringLiteral("Image download error: %1").arg(reply->errorString()));
                    }
                } else {
                    // save the file to the appropriate location.
                    const QByteArray replyData = reply->readAll();
                    if (replyData.isEmpty()) {
                        if (download->m_watcher) {
                            emit download->m_watcher->downloadFailed(QStringLiteral("Empty image data received, aborting"));
                        }
                    } else {
                        const QString albumpath = download->m_albumPath;
                        const QString filename = download->m_fileName;
                        const QString filepath = QStringLiteral("%1/%2/%3/%4")
                                .arg(privilegedImagesDirPath(), m_imageDirectory, albumpath, filename);
                        QDir dir = QFileInfo(filepath).dir();
                        if (!dir.exists()) {
                            dir.mkpath(QStringLiteral("."));
                        }
                        QFile file(filepath);
                        if (!file.open(QFile::WriteOnly)) {
                            if (download->m_watcher) {
                                emit download->m_watcher->downloadFailed(QStringLiteral("Error opening image file %1 for writing: %2")
                                                                                   .arg(filepath, file.errorString()));
                            }
                        } else {
                            file.write(replyData);
                            file.close();
                            emit download->m_watcher->downloadFinished(filepath);
                        }
                    }
                }

                eraseActiveDownload(download);
            });
        }
    }
}

void ImageDownloader::eraseActiveDownload(ImageDownload *download)
{
    QQueue<ImageDownload*>::iterator it = m_active.begin();
    QQueue<ImageDownload*>::iterator end = m_active.end();
    for ( ; it != end; ++it) {
        if (*it == download) {
            m_active.erase(it);
            break;
        }
    }

    delete download;

    QMetaObject::invokeMethod(this, "triggerDownload", Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------

ImageCacheThreadWorker::ImageCacheThreadWorker(QObject *parent)
    : QObject(parent), m_downloader(Q_NULLPTR)
{
}

ImageCacheThreadWorker::~ImageCacheThreadWorker()
{
}

void ImageCacheThreadWorker::openDatabase(const QString &accountType)
{
    DatabaseError error;
    m_db.openDatabase(
            QStringLiteral("%1/%2.db").arg(privilegedImagesDirPath(), accountType),
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

void ImageCacheThreadWorker::populateUserThumbnail(int idempToken, int accountId, const QString &userId, const QNetworkRequest &requestTemplate)
{
    DatabaseError error;
    User user = m_db.user(accountId, userId, &error);
    if (error.errorCode != DatabaseError::NoError) {
        emit populateUserThumbnailFailed(idempToken, QStringLiteral("Error occurred while reading user info from db: %1").arg(error.errorMessage));
        return;
    }

    // the thumbnail already exists.
    const QString thumbnailPath = user.thumbnailPath.toLocalFile();
    if (!user.thumbnailPath.isEmpty() && QFile::exists(thumbnailPath)) {
        emit populateUserThumbnailFinished(idempToken, thumbnailPath);
        return;
    }

    // download the thumbnail.
    if (user.thumbnailUrl.isEmpty()) {
        emit populateUserThumbnailFailed(idempToken, QStringLiteral("Empty thumbnail url specified for user"));
        return;
    }
    if (!m_downloader) {
        m_downloader = new ImageDownloader(4, this);
    }
    ImageDownloadWatcher *watcher = m_downloader->downloadImage(idempToken, user.thumbnailUrl, user.thumbnailUrl.fileName(), QStringLiteral("Thumbnails"), requestTemplate);
    connect(watcher, &ImageDownloadWatcher::downloadFailed, this, [this, watcher, idempToken] (const QString &errorMessage) {
        emit populateUserThumbnailFailed(idempToken, errorMessage);
        watcher->deleteLater();
    });
    connect(watcher, &ImageDownloadWatcher::downloadFinished, this, [this, watcher, user, idempToken] (const QUrl &filePath) {
        // the file has been downloaded to disk.  attempt to update the database.
        DatabaseError storeError;
        User userToStore;
        userToStore.accountId = user.accountId;
        userToStore.userId = user.userId;
        userToStore.thumbnailUrl = user.thumbnailUrl;
        userToStore.thumbnailPath = filePath;
        m_db.storeUser(userToStore, &storeError);
        if (storeError.errorCode != DatabaseError::NoError) {
            QFile::remove(filePath.toLocalFile());
            emit populateUserThumbnailFailed(idempToken, storeError.errorMessage);
        } else {
            emit populateUserThumbnailFinished(idempToken, filePath.toLocalFile());
        }
        watcher->deleteLater();
    });
}

void ImageCacheThreadWorker::populateAlbumThumbnail(int idempToken, int accountId, const QString &userId, const QString &albumId, const QNetworkRequest &requestTemplate)
{
    DatabaseError error;
    Album album = m_db.album(accountId, userId, albumId, &error);
    if (error.errorCode != DatabaseError::NoError) {
        emit populateAlbumThumbnailFailed(idempToken, QStringLiteral("Error occurred while reading album info from db: %1").arg(error.errorMessage));
        return;
    }

    // the thumbnail already exists.
    const QString thumbnailPath = album.thumbnailPath.toLocalFile();
    if (!album.thumbnailPath.isEmpty() && QFile::exists(thumbnailPath)) {
        emit populateAlbumThumbnailFinished(idempToken, thumbnailPath);
        return;
    }

    // download the thumbnail.
    if (album.thumbnailUrl.isEmpty()) {
        emit populateAlbumThumbnailFailed(idempToken, QStringLiteral("Empty thumbnail url specified for album"));
        return;
    }
    if (!m_downloader) {
        m_downloader = new ImageDownloader(4, this);
    }
    ImageDownloadWatcher *watcher = m_downloader->downloadImage(idempToken, album.thumbnailUrl, album.thumbnailUrl.fileName(), QStringLiteral("Thumbnails"), requestTemplate);
    connect(watcher, &ImageDownloadWatcher::downloadFailed, this, [this, watcher, idempToken] (const QString &errorMessage) {
        emit populateAlbumThumbnailFailed(idempToken, errorMessage);
        watcher->deleteLater();
    });
    connect(watcher, &ImageDownloadWatcher::downloadFinished, this, [this, watcher, album, idempToken] (const QUrl &filePath) {
        // the file has been downloaded to disk.  attempt to update the database.
        DatabaseError storeError;
        Album albumToStore;
        albumToStore.accountId = album.accountId;
        albumToStore.userId = album.userId;
        albumToStore.albumId = album.albumId;
        albumToStore.photoCount = album.photoCount;
        albumToStore.thumbnailUrl = album.thumbnailUrl;
        albumToStore.thumbnailPath = filePath;
        m_db.storeAlbum(albumToStore, &storeError);
        if (storeError.errorCode != DatabaseError::NoError) {
            QFile::remove(filePath.toLocalFile());
            emit populateAlbumThumbnailFailed(idempToken, storeError.errorMessage);
        } else {
            emit populateAlbumThumbnailFinished(idempToken, filePath.toLocalFile());
        }
        watcher->deleteLater();
    });
}

void ImageCacheThreadWorker::populatePhotoThumbnail(int idempToken, int accountId, const QString &userId, const QString &albumId, const QString &photoId, const QNetworkRequest &requestTemplate)
{
    DatabaseError error;
    Photo photo = m_db.photo(accountId, userId, albumId, photoId, &error);
    if (error.errorCode != DatabaseError::NoError) {
        emit populatePhotoThumbnailFailed(idempToken, QStringLiteral("Error occurred while reading photo info from db: %1").arg(error.errorMessage));
        return;
    }

    // the thumbnail already exists.
    const QString thumbnailPath = photo.thumbnailPath.toLocalFile();
    if (!photo.thumbnailPath.isEmpty() && QFile::exists(thumbnailPath)) {
        emit populatePhotoThumbnailFinished(idempToken, thumbnailPath);
        return;
    }

    // download the thumbnail.
    if (photo.thumbnailUrl.isEmpty()) {
        emit populatePhotoThumbnailFailed(idempToken, QStringLiteral("Empty thumbnail url specified for photo"));
        return;
    }
    if (!m_downloader) {
        m_downloader = new ImageDownloader(4, this);
    }
    ImageDownloadWatcher *watcher = m_downloader->downloadImage(idempToken, photo.thumbnailUrl, photo.thumbnailUrl.fileName(), QStringLiteral("Thumbnails"), requestTemplate);
    connect(watcher, &ImageDownloadWatcher::downloadFailed, this, [this, watcher, idempToken] (const QString &errorMessage) {
        emit populatePhotoThumbnailFailed(idempToken, errorMessage);
        watcher->deleteLater();
    });
    connect(watcher, &ImageDownloadWatcher::downloadFinished, this, [this, watcher, photo, idempToken] (const QUrl &filePath) {
        // the file has been downloaded to disk.  attempt to update the database.
        DatabaseError storeError;
        Photo photoToStore;
        photoToStore.accountId = photo.accountId;
        photoToStore.userId = photo.userId;
        photoToStore.albumId = photo.albumId;
        photoToStore.photoId = photo.photoId;
        photoToStore.createdTimestamp = photo.createdTimestamp;
        photoToStore.updatedTimestamp = photo.updatedTimestamp;
        photoToStore.fileName = photo.fileName;
        photoToStore.albumPath = photo.albumPath;
        photoToStore.description = photo.description;
        photoToStore.thumbnailUrl = photo.thumbnailUrl;
        photoToStore.thumbnailPath = filePath;
        photoToStore.imageUrl = photo.imageUrl;
        photoToStore.imagePath = photo.imagePath;
        photoToStore.imageWidth = photo.imageWidth;
        photoToStore.imageHeight = photo.imageHeight;
        m_db.storePhoto(photoToStore, &storeError);
        if (storeError.errorCode != DatabaseError::NoError) {
            QFile::remove(filePath.toLocalFile());
            emit populatePhotoThumbnailFailed(idempToken, storeError.errorMessage);
        } else {
            emit populatePhotoThumbnailFinished(idempToken, filePath.toLocalFile());
        }
        watcher->deleteLater();
    });
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
    if (!photo.imagePath.isEmpty() && QFile::exists(imagePath)) {
        emit populatePhotoImageFinished(idempToken, imagePath);
        return;
    }

    // download the image.
    if (photo.imageUrl.isEmpty()) {
        emit populatePhotoImageFailed(idempToken, QStringLiteral("Empty image url specified for photo"));
        return;
    }
    if (!m_downloader) {
        m_downloader = new ImageDownloader(4, this);
    }
    ImageDownloadWatcher *watcher = m_downloader->downloadImage(idempToken, photo.imageUrl, photo.fileName, photo.albumPath, requestTemplate);
    connect(watcher, &ImageDownloadWatcher::downloadFailed, this, [this, watcher, idempToken] (const QString &errorMessage) {
        emit populatePhotoImageFailed(idempToken, errorMessage);
        watcher->deleteLater();
    });
    connect(watcher, &ImageDownloadWatcher::downloadFinished, this, [this, watcher, photo, idempToken] (const QUrl &filePath) {
        // the file has been downloaded to disk.  attempt to update the database.
        DatabaseError storeError;
        Photo photoToStore;
        photoToStore.accountId = photo.accountId;
        photoToStore.userId = photo.userId;
        photoToStore.albumId = photo.albumId;
        photoToStore.photoId = photo.photoId;
        photoToStore.createdTimestamp = photo.createdTimestamp;
        photoToStore.updatedTimestamp = photo.updatedTimestamp;
        photoToStore.fileName = photo.fileName;
        photoToStore.albumPath = photo.albumPath;
        photoToStore.description = photo.description;
        photoToStore.thumbnailUrl = photo.thumbnailUrl;
        photoToStore.thumbnailPath = photo.thumbnailPath;
        photoToStore.imageUrl = photo.imageUrl;
        photoToStore.imagePath = filePath;
        photoToStore.imageWidth = photo.imageWidth;
        photoToStore.imageHeight = photo.imageHeight;
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
    connect(this, &ImageCachePrivate::requestUsers, m_worker, &ImageCacheThreadWorker::requestUsers);
    connect(this, &ImageCachePrivate::requestAlbums, m_worker, &ImageCacheThreadWorker::requestAlbums);
    connect(this, &ImageCachePrivate::requestPhotos, m_worker, &ImageCacheThreadWorker::requestPhotos);

    connect(this, &ImageCachePrivate::populateUserThumbnail, m_worker, &ImageCacheThreadWorker::populateUserThumbnail);
    connect(this, &ImageCachePrivate::populateAlbumThumbnail, m_worker, &ImageCacheThreadWorker::populateAlbumThumbnail);
    connect(this, &ImageCachePrivate::populatePhotoThumbnail, m_worker, &ImageCacheThreadWorker::populatePhotoThumbnail);
    connect(this, &ImageCachePrivate::populatePhotoImage, m_worker, &ImageCacheThreadWorker::populatePhotoImage);

    connect(m_worker, &ImageCacheThreadWorker::openDatabaseFailed, parent, &ImageCache::openDatabaseFailed);
    connect(m_worker, &ImageCacheThreadWorker::openDatabaseFinished, parent, &ImageCache::openDatabaseFinished);
    connect(m_worker, &ImageCacheThreadWorker::requestUsersFailed, parent, &ImageCache::requestUsersFailed);
    connect(m_worker, &ImageCacheThreadWorker::requestUsersFinished, parent, &ImageCache::requestUsersFinished);
    connect(m_worker, &ImageCacheThreadWorker::requestAlbumsFailed, parent, &ImageCache::requestAlbumsFailed);
    connect(m_worker, &ImageCacheThreadWorker::requestAlbumsFinished, parent, &ImageCache::requestAlbumsFinished);
    connect(m_worker, &ImageCacheThreadWorker::requestPhotosFailed, parent, &ImageCache::requestPhotosFailed);
    connect(m_worker, &ImageCacheThreadWorker::requestPhotosFinished, parent, &ImageCache::requestPhotosFinished);

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

void ImageCache::openDatabase(const QString &databaseFile)
{
    Q_D(ImageCache);
    emit d->openDatabase(databaseFile);
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
