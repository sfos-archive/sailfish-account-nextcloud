/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_IMAGECACHE_P_H
#define NEXTCLOUD_IMAGECACHE_P_H

#include "imagecache.h"
#include "synccachedatabase_p.h"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QVector>
#include <QtCore/QQueue>
#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QtCore/QScopedPointer>
#include <QtCore/QPointer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtSql/QSqlDatabase>

namespace SyncCache {

class ImageDownloadWatcher : public QObject
{
    Q_OBJECT

public:
    ImageDownloadWatcher(int idempToken, const QUrl &imageUrl, QObject *parent = Q_NULLPTR);
    ~ImageDownloadWatcher();

    int idempToken() const;
    QUrl imageUrl() const;

Q_SIGNALS:
    void downloadFailed(const QString &errorMessage);
    void downloadFinished(const QUrl &filePath);

private:
    int m_idempToken;
    QUrl m_imageUrl;
};

class ImageDownload
{
public:
    ImageDownload(
            int idempToken = 0,
            const QUrl &imageUrl = QUrl(),
            const QString &fileName = QString(),
            const QString &albumPath = QString(),
            const QNetworkRequest &templateRequest = QNetworkRequest(QUrl()),
            ImageDownloadWatcher *watcher = Q_NULLPTR);
    ~ImageDownload();

    int m_idempToken = 0;
    QUrl m_imageUrl;
    QString m_fileName;
    QString m_albumPath;
    QNetworkRequest m_templateRequest;
    QTimer *m_timeoutTimer = Q_NULLPTR;
    QNetworkReply *m_reply = Q_NULLPTR;
    QPointer<SyncCache::ImageDownloadWatcher> m_watcher;
};

class ImageDownloader : public QObject
{
    Q_OBJECT

public:
    ImageDownloader(int maxActive = 4, QObject *parent = Q_NULLPTR);
    ~ImageDownloader();

    void setImageDirectory(const QString &path);
    ImageDownloadWatcher *downloadImage(
            int idempToken,
            const QUrl &imageUrl,
            const QString &fileName,
            const QString &albumPath,
            const QNetworkRequest &requestTemplate);

private Q_SLOTS:
    void triggerDownload();

private:
    void eraseActiveDownload(ImageDownload *download);

    QNetworkAccessManager m_qnam;
    QQueue<ImageDownload*> m_pending;
    QQueue<ImageDownload*> m_active;
    int m_maxActive;
    QString m_imageDirectory;
};

class ImageCacheThreadWorker : public QObject
{
    Q_OBJECT

public:
    ImageCacheThreadWorker(QObject *parent = Q_NULLPTR);
    ~ImageCacheThreadWorker();

public Q_SLOTS:
    void openDatabase(const QString &accountType);

    void requestUsers();
    void requestAlbums(int accountId, const QString &userId);
    void requestPhotos(int accountId, const QString &userId, const QString &albumId);

    void populateUserThumbnail(int idempToken, int accountId, const QString &userId, const QNetworkRequest &requestTemplate);
    void populateAlbumThumbnail(int idempToken, int accountId, const QString &userId, const QString &albumId, const QNetworkRequest &requestTemplate);
    void populatePhotoThumbnail(int idempToken, int accountId, const QString &userId, const QString &albumId, const QString &photoId, const QNetworkRequest &requestTemplate);
    void populatePhotoImage(int idempToken, int accountId, const QString &userId, const QString &albumId, const QString &photoId, const QNetworkRequest &requestTemplate);

Q_SIGNALS:
    void openDatabaseFailed(const QString &errorMessage);
    void openDatabaseFinished();

    void requestUsersFailed(const QString &errorMessage);
    void requestUsersFinished(const QVector<SyncCache::User> &users);

    void requestAlbumsFailed(int accountId, const QString &userId, const QString &errorMessage);
    void requestAlbumsFinished(int accountId, const QString &userId, const QVector<SyncCache::Album> &albums);

    void requestPhotosFailed(int accountId, const QString &userId, const QString &albumId, const QString &errorMessage);
    void requestPhotosFinished(int accountId, const QString &userId, const QString &albumId, const QVector<SyncCache::Photo> &photos);

    void populateUserThumbnailFailed(int idempToken, const QString &errorMessage);
    void populateUserThumbnailFinished(int idempToken, const QString &path);

    void populateAlbumThumbnailFailed(int idempToken, const QString &errorMessage);
    void populateAlbumThumbnailFinished(int idempToken, const QString &path);

    void populatePhotoThumbnailFailed(int idempToken, const QString &errorMessage);
    void populatePhotoThumbnailFinished(int idempToken, const QString &path);

    void populatePhotoImageFailed(int idempToken, const QString &errorMessage);
    void populatePhotoImageFinished(int idempToken, const QString &path);

    void usersStored(const QVector<SyncCache::User> &users);
    void albumsStored(const QVector<SyncCache::Album> &albums);
    void photosStored(const QVector<SyncCache::Photo> &photos);

    void usersDeleted(const QVector<SyncCache::User> &users);
    void albumsDeleted(const QVector<SyncCache::Album> &albums);
    void photosDeleted(const QVector<SyncCache::Photo> &photos);

private:
    ImageDatabase m_db;
    ImageDownloader *m_downloader;
};

class ImageCachePrivate : public QObject
{
    Q_OBJECT

public:
    ImageCachePrivate(ImageCache *parent);
    ~ImageCachePrivate();

Q_SIGNALS:
    void openDatabase(const QString &accountType);

    void requestUsers();
    void requestAlbums(int accountId, const QString &userId);
    void requestPhotos(int accountId, const QString &userId, const QString &albumId);

    bool populateUserThumbnail(int idempToken, int accountId, const QString &userId, const QNetworkRequest &requestTemplate);
    bool populateAlbumThumbnail(int idempToken, int accountId, const QString &userId, const QString &albumId, const QNetworkRequest &requestTemplate);
    bool populatePhotoThumbnail(int idempToken, int accountId, const QString &userId, const QString &albumId, const QString &photoId, const QNetworkRequest &requestTemplate);
    bool populatePhotoImage(int idempToken, int accountId, const QString &userId, const QString &albumId, const QString &photoId, const QNetworkRequest &requestTemplate);

private:
    QThread m_dbThread;
    ImageCacheThreadWorker *m_worker;
};

class ImageDatabasePrivate : public DatabasePrivate
{
public:
    ImageDatabasePrivate(ImageDatabase *parent);

    int currentSchemaVersion() const Q_DECL_OVERRIDE;
    QVector<const char *> createStatements() const Q_DECL_OVERRIDE;
    QVector<UpgradeOperation> upgradeVersions() const Q_DECL_OVERRIDE;

    void preTransactionCommit() Q_DECL_OVERRIDE;
    void transactionCommittedPreUnlock() Q_DECL_OVERRIDE;
    void transactionCommittedPostUnlock() Q_DECL_OVERRIDE;
    void transactionRolledBackPreUnlocked() Q_DECL_OVERRIDE;

private:
    friend class SyncCache::ImageDatabase;
    ImageDatabase *m_imageDbParent;

    QVector<QString> m_filesToDelete;
    QVector<SyncCache::User> m_deletedUsers;
    QVector<SyncCache::Album> m_deletedAlbums;
    QVector<SyncCache::Photo> m_deletedPhotos;
    QVector<SyncCache::User> m_storedUsers;
    QVector<SyncCache::Album> m_storedAlbums;
    QVector<SyncCache::Photo> m_storedPhotos;

    QVector<QString> m_tempFilesToDelete;
    QVector<SyncCache::User> m_tempDeletedUsers;
    QVector<SyncCache::Album> m_tempDeletedAlbums;
    QVector<SyncCache::Photo> m_tempDeletedPhotos;
    QVector<SyncCache::User> m_tempStoredUsers;
    QVector<SyncCache::Album> m_tempStoredAlbums;
    QVector<SyncCache::Photo> m_tempStoredPhotos;
};

} // namespace SyncCache

#endif // NEXTCLOUD_IMAGECACHE_P_H
