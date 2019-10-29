/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_GALLERY_IMAGEMODELS_H
#define NEXTCLOUD_GALLERY_IMAGEMODELS_H

#include "imagecache.h"

#include <QtCore/QAbstractListModel>
#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QPair>
#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkRequest>
#include <QtQml/QQmlParserStatus>

class AccountAuthenticator;

class NextcloudImageCache : public SyncCache::ImageCache
{
    Q_OBJECT

public:
    explicit NextcloudImageCache(QObject *parent = Q_NULLPTR);

    void openDatabase(const QString &) Q_DECL_OVERRIDE;
    void populateUserThumbnail(int idempToken, int accountId, const QString &userId, const QNetworkRequest &) Q_DECL_OVERRIDE;
    void populateAlbumThumbnail(int idempToken, int accountId, const QString &userId, const QString &albumId, const QNetworkRequest &) Q_DECL_OVERRIDE;
    void populatePhotoThumbnail(int idempToken, int accountId, const QString &userId, const QString &albumId, const QString &photoId, const QNetworkRequest &) Q_DECL_OVERRIDE;
    void populatePhotoImage(int idempToken, int accountId, const QString &userId, const QString &albumId, const QString &photoId, const QNetworkRequest &) Q_DECL_OVERRIDE;

    enum PendingRequestType {
        PopulateUserThumbnailType,
        PopulateAlbumThumbnailType,
        PopulatePhotoThumbnailType,
        PopulatePhotoImageType
    };

    struct PendingRequest {
        PendingRequestType type;
        int idempToken;
        int accountId;
        QString userId;
        QString albumId;
        QString photoId;
    };

private Q_SLOTS:
    void performRequests();
    void signOnResponse(int accountId, const QString &serviceName, const QString &serverUrl, const QString &webdavPath, const QString &username, const QString &password, const QString &accessToken, bool ignoreSslErrors);
    void signOnError(int accountId, const QString &serviceName);

private:
    QList<PendingRequest> m_pendingRequests;
    QList<int> m_pendingAccountRequests;
    QHash<int, int> m_signOnFailCount;
    QHash<int, QPair<QString, QString> > m_accountIdCredentials;
    QHash<int, QString> m_accountIdAccessTokens;
    AccountAuthenticator *m_auth = Q_NULLPTR;

    void signIn(int accountId);
    void performRequest(const PendingRequest &request);
    QNetworkRequest templateRequest(int accountId) const;
};
Q_DECLARE_METATYPE(NextcloudImageCache::PendingRequest)
Q_DECLARE_TYPEINFO(NextcloudImageCache::PendingRequest, Q_MOVABLE_TYPE);

class NextcloudUsersModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(SyncCache::ImageCache* imageCache READ imageCache WRITE setImageCache NOTIFY imageCacheChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY rowCountChanged)

public:
    explicit NextcloudUsersModel(QObject *parent = Q_NULLPTR);

    // QQmlParserStatus
    void classBegin() Q_DECL_OVERRIDE;
    void componentComplete() Q_DECL_OVERRIDE;

    // QQmlAbstractListModel
    QModelIndex index(int row, int column, const QModelIndex &parent) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QHash<int, QByteArray> roleNames() const Q_DECL_OVERRIDE;

    enum Roles {
        AccountIdRole = Qt::UserRole + 1,
        UserIdRole,
        ThumbnailUrlRole,
        ThumbnailPathRole
    };
    Q_ENUM(Roles)

    SyncCache::ImageCache* imageCache() const;
    void setImageCache(SyncCache::ImageCache *cache);

    Q_INVOKABLE QVariantMap at(int row) const;

Q_SIGNALS:
    void imageCacheChanged();
    void rowCountChanged();

private:
    void loadData();
    bool m_deferLoad;
    SyncCache::ImageCache* m_imageCache;
    QVector<SyncCache::User> m_data;
};

class NextcloudAlbumsModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(SyncCache::ImageCache* imageCache READ imageCache WRITE setImageCache NOTIFY imageCacheChanged)
    Q_PROPERTY(int accountId READ accountId WRITE setAccountId NOTIFY accountIdChanged)
    Q_PROPERTY(QString userId READ userId WRITE setUserId NOTIFY userIdChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY rowCountChanged)

public:
    explicit NextcloudAlbumsModel(QObject *parent = Q_NULLPTR);

    // QQmlParserStatus
    void classBegin() Q_DECL_OVERRIDE;
    void componentComplete() Q_DECL_OVERRIDE;

    // QQmlAbstractListModel
    QModelIndex index(int row, int column, const QModelIndex &parent) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QHash<int, QByteArray> roleNames() const Q_DECL_OVERRIDE;

    enum Roles {
        AccountIdRole = Qt::UserRole + 1,
        UserIdRole,
        AlbumIdRole,
        ParentAlbumIdRole,
        AlbumNameRole,
        PhotoCountRole,
        ThumbnailUrlRole,
        ThumbnailPathRole,
    };
    Q_ENUM(Roles)

    SyncCache::ImageCache* imageCache() const;
    void setImageCache(SyncCache::ImageCache *cache);

    int accountId() const;
    void setAccountId(int id);

    QString userId() const;
    void setUserId(const QString &id);

    Q_INVOKABLE QVariantMap at(int row) const;

Q_SIGNALS:
    void imageCacheChanged();
    void accountIdChanged();
    void userIdChanged();
    void rowCountChanged();

private:
    void loadData();
    bool m_deferLoad;
    SyncCache::ImageCache* m_imageCache;
    int m_accountId;
    QString m_userId;
    QVector<SyncCache::Album> m_data;
};

class NextcloudPhotosModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(SyncCache::ImageCache* imageCache READ imageCache WRITE setImageCache NOTIFY imageCacheChanged)
    Q_PROPERTY(int accountId READ accountId WRITE setAccountId NOTIFY accountIdChanged)
    Q_PROPERTY(QString userId READ userId WRITE setUserId NOTIFY userIdChanged)
    Q_PROPERTY(QString albumId READ albumId WRITE setAlbumId NOTIFY albumIdChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY rowCountChanged)

public:
    explicit NextcloudPhotosModel(QObject *parent = Q_NULLPTR);

    // QQmlParserStatus
    void classBegin() Q_DECL_OVERRIDE;
    void componentComplete() Q_DECL_OVERRIDE;

    // QQmlAbstractListModel
    QModelIndex index(int row, int column, const QModelIndex &parent) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QHash<int, QByteArray> roleNames() const Q_DECL_OVERRIDE;

    enum Roles {
        AccountIdRole = Qt::UserRole + 1,
        UserIdRole,
        AlbumIdRole,
        PhotoIdRole,
        CreatedTimestampRole,
        UpdatedTimestampRole,
        FileNameRole,
        AlbumPathRole,
        DescriptionRole,
        ThumbnailUrlRole,
        ThumbnailPathRole,
        ImageUrlRole,
        ImagePathRole,
        ImageWidthRole,
        ImageHeightRole
    };
    Q_ENUM(Roles)

    SyncCache::ImageCache* imageCache() const;
    void setImageCache(SyncCache::ImageCache *cache);

    int accountId() const;
    void setAccountId(int id);

    QString userId() const;
    void setUserId(const QString &id);

    QString albumId() const;
    void setAlbumId(const QString &albumId);

    Q_INVOKABLE QVariantMap at(int row) const;

Q_SIGNALS:
    void imageCacheChanged();
    void accountIdChanged();
    void userIdChanged();
    void albumIdChanged();
    void rowCountChanged();

private:
    void loadData();
    bool m_deferLoad;
    SyncCache::ImageCache* m_imageCache;
    int m_accountId;
    QString m_userId;
    QString m_albumId;
    QVector<SyncCache::Photo> m_data;
};

class NextcloudImageDownloader : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(SyncCache::ImageCache* imageCache READ imageCache WRITE setImageCache NOTIFY imageCacheChanged)
    Q_PROPERTY(int accountId READ accountId WRITE setAccountId NOTIFY accountIdChanged)
    Q_PROPERTY(QString userId READ userId WRITE setUserId NOTIFY userIdChanged)
    Q_PROPERTY(QString albumId READ albumId WRITE setAlbumId NOTIFY albumIdChanged)
    Q_PROPERTY(QString photoId READ photoId WRITE setPhotoId NOTIFY photoIdChanged)
    Q_PROPERTY(bool downloadThumbnail READ downloadThumbnail WRITE setDownloadThumbnail NOTIFY downloadThumbnailChanged)
    Q_PROPERTY(bool downloadImage READ downloadImage WRITE setDownloadImage NOTIFY downloadImageChanged)
    Q_PROPERTY(QUrl imagePath READ imagePath NOTIFY imagePathChanged)

public:
    explicit NextcloudImageDownloader(QObject *parent = Q_NULLPTR);

    // QQmlParserStatus
    void classBegin() Q_DECL_OVERRIDE;
    void componentComplete() Q_DECL_OVERRIDE;

    SyncCache::ImageCache* imageCache() const;
    void setImageCache(SyncCache::ImageCache *cache);

    int accountId() const;
    void setAccountId(int id);

    QString userId() const;
    void setUserId(const QString &id);

    QString albumId() const;
    void setAlbumId(const QString &albumId);

    QString photoId() const;
    void setPhotoId(const QString &photoId);

    bool downloadThumbnail() const;
    void setDownloadThumbnail(bool v);

    bool downloadImage() const;
    void setDownloadImage(bool v);

    QUrl imagePath() const;

Q_SIGNALS:
    void imageCacheChanged();
    void accountIdChanged();
    void userIdChanged();
    void albumIdChanged();
    void photoIdChanged();
    void downloadThumbnailChanged();
    void downloadImageChanged();
    void imagePathChanged();

private:
    void loadImage();
    bool m_deferLoad;
    SyncCache::ImageCache* m_imageCache;
    int m_accountId;
    QString m_userId;
    QString m_albumId;
    QString m_photoId;
    bool m_downloadThumbnail;
    bool m_downloadImage;
    QUrl m_imagePath;
    int m_idempToken;
};

#endif // NEXTCLOUD_GALLERY_IMAGEMODELS_H
