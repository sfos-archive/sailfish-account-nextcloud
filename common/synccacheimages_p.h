/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_SYNCCACHEIMAGES_P_H
#define NEXTCLOUD_SYNCCACHEIMAGES_P_H

#include "synccacheimages.h"
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
    ImageDownloadWatcher(int idempToken, const QUrl &imageUrl, QObject *parent = nullptr);
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
    ImageDownload(int idempToken = 0,
            const QUrl &imageUrl = QUrl(),
            const QString &fileName = QString(),
            const QString &fileDirPath = QString(),
            const QNetworkRequest &templateRequest = QNetworkRequest(QUrl()),
            ImageDownloadWatcher *watcher = nullptr);
    ~ImageDownload();

    int m_idempToken = 0;
    QUrl m_imageUrl;
    QString m_fileName;
    QString m_fileDirPath;
    QNetworkRequest m_templateRequest;
    QTimer *m_timeoutTimer = nullptr;
    QNetworkReply *m_reply = nullptr;
    QPointer<SyncCache::ImageDownloadWatcher> m_watcher;
};

class ImageDownloader : public QObject
{
    Q_OBJECT

public:
    ImageDownloader(int maxActive = 4, QObject *parent = nullptr);
    ~ImageDownloader();

    ImageDownloadWatcher *downloadImage(int idempToken,
            const QUrl &imageUrl,
            const QString &fileName,
            const QString &fileDirPath,
            const QNetworkRequest &requestTemplate);

    QString userImageDownloadDir(int accountId, const QString &userId, bool thumbnail) const;
    QString albumImageDownloadDir(int accountId, const QString &albumName, bool thumbnail) const;

    static QString imageDownloadDir(int accountId);

private Q_SLOTS:
    void triggerDownload();

private:
    void eraseActiveDownload(ImageDownload *download);

    QNetworkAccessManager m_qnam;
    QQueue<ImageDownload*> m_pending;
    QQueue<ImageDownload*> m_active;
    int m_maxActive;
};

class ImageCacheThreadWorker : public QObject
{
    Q_OBJECT

public:
    ImageCacheThreadWorker(QObject *parent = nullptr);
    ~ImageCacheThreadWorker();

public Q_SLOTS:
    void openDatabase(const QString &accountType);

    void requestUser(int accountId, const QString &userId);
    void requestUsers();
    void requestAlbums(int accountId, const QString &userId);
    void requestPhotos(int accountId, const QString &userId, const QString &albumId);
    void requestPhotoCount();

    void populateUserThumbnail(int idempToken, int accountId, const QString &userId, const QNetworkRequest &requestTemplate);
    void populateAlbumThumbnail(int idempToken, int accountId, const QString &userId, const QString &albumId, const QNetworkRequest &requestTemplate);
    void populatePhotoThumbnail(int idempToken, int accountId, const QString &userId, const QString &albumId, const QString &photoId, const QNetworkRequest &requestTemplate);
    void populatePhotoImage(int idempToken, int accountId, const QString &userId, const QString &albumId, const QString &photoId, const QNetworkRequest &requestTemplate);

Q_SIGNALS:
    void openDatabaseFailed(const QString &errorMessage);
    void openDatabaseFinished();

    void requestUserFailed(int accountId, const QString &userId, const QString &errorMessage);
    void requestUserFinished(int accountId, const QString &userId, const SyncCache::User &user);

    void requestUsersFailed(const QString &errorMessage);
    void requestUsersFinished(const QVector<SyncCache::User> &users);

    void requestAlbumsFailed(int accountId, const QString &userId, const QString &errorMessage);
    void requestAlbumsFinished(int accountId, const QString &userId, const QVector<SyncCache::Album> &albums);

    void requestPhotosFailed(int accountId, const QString &userId, const QString &albumId, const QString &errorMessage);
    void requestPhotosFinished(int accountId, const QString &userId, const QString &albumId, const QVector<SyncCache::Photo> &photos);

    void requestPhotoCountFailed(const QString &errorMessage);
    void requestPhotoCountFinished(int photoCount);

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
    void photoThumbnailDownloadFinished(int idempToken, const SyncCache::Photo &photo, const QUrl &filePath);

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

    void requestUser(int accountId, const QString &userId);
    void requestUsers();
    void requestAlbums(int accountId, const QString &userId);
    void requestPhotos(int accountId, const QString &userId, const QString &albumId);
    void requestPhotoCount();

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

    int currentSchemaVersion() const override;
    QVector<const char *> createStatements() const override;
    QVector<UpgradeOperation> upgradeVersions() const override;

    void preTransactionCommit() override;
    void transactionCommittedPreUnlock() override;
    void transactionCommittedPostUnlock() override;
    void transactionRolledBackPreUnlocked() override;

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

#endif // NEXTCLOUD_SYNCCACHEIMAGES_P_H
