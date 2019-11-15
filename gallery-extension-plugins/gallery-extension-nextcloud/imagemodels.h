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

#include "synccacheimages.h"

#include <QtCore/QAbstractListModel>
#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QPair>
#include <QtQml/QQmlParserStatus>

class NextcloudUsersModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(SyncCache::ImageCache* imageCache READ imageCache WRITE setImageCache NOTIFY imageCacheChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY rowCountChanged)

public:
    explicit NextcloudUsersModel(QObject *parent = nullptr);

    // QQmlParserStatus
    void classBegin() override;
    void componentComplete() override;

    // QQmlAbstractListModel
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    enum Roles {
        AccountIdRole = Qt::UserRole + 1,
        UserIdRole,
        ThumbnailUrlRole,
        ThumbnailPathRole
    };
    Q_ENUM(Roles)

    SyncCache::ImageCache *imageCache() const;
    void setImageCache(SyncCache::ImageCache *cache);

    Q_INVOKABLE QVariantMap at(int row) const;

Q_SIGNALS:
    void imageCacheChanged();
    void rowCountChanged();

private:
    void loadData();

    bool m_deferLoad = false;
    SyncCache::ImageCache *m_imageCache = nullptr;
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
    explicit NextcloudAlbumsModel(QObject *parent = nullptr);

    // QQmlParserStatus
    void classBegin() override;
    void componentComplete() override;

    // QQmlAbstractListModel
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    enum Roles {
        AccountIdRole = Qt::UserRole + 1,
        UserIdRole,
        AlbumIdRole,
        ParentAlbumIdRole,
        AlbumNameRole,
        PhotoCountRole,
        ThumbnailUrlRole,
        ThumbnailPathRole,
        ThumbnailFileNameRole,
    };
    Q_ENUM(Roles)

    SyncCache::ImageCache *imageCache() const;
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

    bool m_deferLoad = false;
    SyncCache::ImageCache *m_imageCache = nullptr;
    int m_accountId = 0;
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
    explicit NextcloudPhotosModel(QObject *parent = nullptr);

    // QQmlParserStatus
    void classBegin() override;
    void componentComplete() override;

    // QQmlAbstractListModel
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

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
        ImageHeightRole,
        FileSizeRole,
        FileTypeRole
    };
    Q_ENUM(Roles)

    SyncCache::ImageCache *imageCache() const;
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

    bool m_deferLoad = false;
    SyncCache::ImageCache *m_imageCache = nullptr;
    int m_accountId = 0;
    QString m_userId;
    QString m_albumId;
    QVector<SyncCache::Photo> m_data;
};

class NextcloudPhotoCounter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(SyncCache::ImageCache* imageCache READ imageCache WRITE setImageCache NOTIFY imageCacheChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    explicit NextcloudPhotoCounter(QObject *parent = nullptr);

    SyncCache::ImageCache *imageCache() const;
    void setImageCache(SyncCache::ImageCache *cache);

    int count();

Q_SIGNALS:
    void countChanged();
    void imageCacheChanged();

private:
    SyncCache::ImageCache *m_imageCache = nullptr;
    int m_count = 0;
};

#endif // NEXTCLOUD_GALLERY_IMAGEMODELS_H
