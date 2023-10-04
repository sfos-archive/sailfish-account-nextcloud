/****************************************************************************************
** Copyright (c) 2019 Open Mobile Platform LLC.
** Copyright (c) 2023 Jolla Ltd.
**
** All rights reserved.
**
** This file is part of Sailfish Nextcloud account package.
**
** You may use this file under the terms of BSD license as follows:
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** 1. Redistributions of source code must retain the above copyright notice, this
**    list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright notice,
**    this list of conditions and the following disclaimer in the documentation
**    and/or other materials provided with the distribution.
**
** 3. Neither the name of the copyright holder nor the names of its
**    contributors may be used to endorse or promote products derived from
**    this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
** OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************************/

#ifndef NEXTCLOUD_SYNCCACHEIMAGES_P_H
#define NEXTCLOUD_SYNCCACHEIMAGES_P_H

#include "synccacheimages.h"
#include "synccachedatabase_p.h"

#include <QtCore/QScopedPointer>
#include <QtCore/QVector>
#include <QtCore/QThread>
#include <QtSql/QSqlDatabase>

namespace SyncCache {

class ImageChangeNotifier;
class ImageDownloader;

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
    void requestPhotoCount(int accountId, const QString &userId);

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

    void requestPhotoCountFailed(int accountId, const QString &userId, const QString &errorMessage);
    void requestPhotoCountFinished(int accountId, const QString &userId, int photoCount);

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

    void dataChanged();

private:
    void photoThumbnailDownloadFinished(int idempToken, const SyncCache::Photo &photo, const QUrl &filePath);

    ImageDatabase m_db;
    ImageDownloader *m_downloader = nullptr;
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
    void requestPhotoCount(int accountId, const QString &userId);

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
    ImageDatabasePrivate(ImageDatabase *parent, bool emitCrossProcessChangeNotifications);

    int currentSchemaVersion() const override;
    QVector<const char *> createStatements() const override;
    QVector<UpgradeOperation> upgradeVersions() const override;

    bool preTransactionCommit() override;
    void transactionCommittedPreUnlock() override;
    void transactionCommittedPostUnlock() override;
    void transactionRolledBackPreUnlocked() override;

private:
    friend class SyncCache::ImageDatabase;
    ImageDatabase *m_imageDbParent;
    QScopedPointer<ImageChangeNotifier> m_changeNotifier;
    bool m_emitCrossProcessChangeNotifications = true;

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
