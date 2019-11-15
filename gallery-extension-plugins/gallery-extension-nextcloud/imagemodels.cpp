/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "imagemodels.h"

#include <QtCore/QDebug>
#include <QtQml/QQmlInfo>

NextcloudUserModel::NextcloudUserModel(QObject *parent)
    : QAbstractListModel(parent)
{
    qRegisterMetaType<SyncCache::User>();
    qRegisterMetaType<QVector<SyncCache::User> >();
}

void NextcloudUserModel::classBegin()
{
    m_deferLoad = true;
}

void NextcloudUserModel::componentComplete()
{
    m_deferLoad = false;
    if (m_imageCache) {
        loadData();
    }
}

QModelIndex NextcloudUserModel::index(int row, int column, const QModelIndex &parent) const
{
    return !parent.isValid() && column == 0 && row >= 0 && row < m_data.size()
            ? createIndex(row, column)
            : QModelIndex();
}

QVariant NextcloudUserModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (!index.isValid() || row < 0 || row >= m_data.size()) {
        return QVariant();
    }

    // TODO: if thumbnail path is requested but empty,
    //       call m_cache->populateUserThumbnail(),
    //       and when it succeeds, emit dataChanged(row).
    switch (role) {
        case AccountIdRole:     return m_data[row].accountId;
        case UserIdRole:        return m_data[row].userId;
        case ThumbnailUrlRole:  return m_data[row].thumbnailUrl;
        case ThumbnailPathRole: return m_data[row].thumbnailPath;
        default:                return QVariant();
    }
}

int NextcloudUserModel::rowCount(const QModelIndex &) const
{
    return m_data.size();
}

QHash<int, QByteArray> NextcloudUserModel::roleNames() const
{
    static QHash<int, QByteArray> retn {
        { AccountIdRole,        "accountId" },
        { UserIdRole,           "userId" },
        { ThumbnailUrlRole,     "thumbnailUrl" },
        { ThumbnailPathRole,    "thumbnailPath" }
    };

    return retn;
}

SyncCache::ImageCache *NextcloudUserModel::imageCache() const
{
    return m_imageCache;
}

void NextcloudUserModel::setImageCache(SyncCache::ImageCache *cache)
{
    if (m_imageCache == cache) {
        return;
    }

    if (m_imageCache) {
        disconnect(m_imageCache, 0, this, 0);
    }

    m_imageCache = cache;
    emit imageCacheChanged();

    if (!m_deferLoad) {
        loadData();
    }

    connect(m_imageCache, &SyncCache::ImageCache::usersStored,
            this, [this] (const QVector<SyncCache::User> &users) {
        for (const SyncCache::User &user : users) {
            bool foundUser = false;
            for (int row = 0; row < m_data.size(); ++row) {
                const SyncCache::User &existing(m_data[row]);
                if (user.accountId == existing.accountId
                        && user.userId == existing.userId) {
                    m_data.replace(row, user);
                    emit dataChanged(index(row, 0, QModelIndex()), index(row, 0, QModelIndex()));
                    foundUser = true;
                }
            }

            if (!foundUser) {
                emit beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
                m_data.append(user);
                emit endInsertRows();
                emit rowCountChanged();
            }
        }
    });

    connect(m_imageCache, &SyncCache::ImageCache::usersDeleted,
            this, [this] (const QVector<SyncCache::User> &users) {
        for (const SyncCache::User &user : users) {
            for (int row = 0; row < m_data.size(); ++row) {
                const SyncCache::User &existing(m_data[row]);
                if (user.accountId == existing.accountId
                        && user.userId == existing.userId) {
                    emit beginRemoveRows(QModelIndex(), row, row);
                    m_data.remove(row);
                    emit endRemoveRows();
                    emit rowCountChanged();
                }
            }
        }
    });
}

QVariantMap NextcloudUserModel::at(int row) const
{
    QVariantMap retn;
    if (row < 0 || row >= rowCount()) {
        return retn;
    }

    const QHash<int, QByteArray> roles = roleNames();
    QHash<int, QByteArray>::const_iterator it = roles.constBegin();
    QHash<int, QByteArray>::const_iterator end = roles.constEnd();
    for ( ; it != end; it++) {
        retn.insert(QString::fromLatin1(it.value()), data(index(row, 0, QModelIndex()), it.key()));
    }
    return retn;
}

void NextcloudUserModel::loadData()
{
    if (!m_imageCache) {
        return;
    }

    QObject *contextObject = new QObject(this);
    connect(m_imageCache, &SyncCache::ImageCache::requestUsersFinished,
            contextObject, [this, contextObject] (const QVector<SyncCache::User> &users) {
        contextObject->deleteLater();
        const int oldSize = m_data.size();
        if (m_data.size()) {
            emit beginRemoveRows(QModelIndex(), 0, m_data.size() - 1);
            m_data.clear();
            emit endRemoveRows();
        }
        if (users.size()) {
            emit beginInsertRows(QModelIndex(), 0, users.size() - 1);
            m_data = users;
            emit endInsertRows();
        }
        if (m_data.size() != oldSize) {
            emit rowCountChanged();
        }
    });
    connect(m_imageCache, &SyncCache::ImageCache::requestUsersFailed,
            contextObject, [this, contextObject] (const QString &errorMessage) {
        contextObject->deleteLater();
        qWarning() << "NextcloudUserModel::loadData: failed:" << errorMessage;
    });
    m_imageCache->requestUsers();
}

//-----------------------------------------------------------------------------

NextcloudAlbumModel::NextcloudAlbumModel(QObject *parent)
    : QAbstractListModel(parent)
{
    qRegisterMetaType<SyncCache::Album>();
    qRegisterMetaType<QVector<SyncCache::Album> >();
}

void NextcloudAlbumModel::classBegin()
{
    m_deferLoad = true;
}

void NextcloudAlbumModel::componentComplete()
{
    m_deferLoad = false;
    if (m_imageCache) {
        loadData();
    }
}

QModelIndex NextcloudAlbumModel::index(int row, int column, const QModelIndex &parent) const
{
    return !parent.isValid() && column == 0 && row >= 0 && row < m_data.size()
            ? createIndex(row, column)
            : QModelIndex();
}

QVariant NextcloudAlbumModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (!index.isValid() || row < 0 || row >= m_data.size()) {
        return QVariant();
    }

    // TODO: if thumbnail path is requested but empty,
    //       call m_cache->populateAlbumThumbnail(),
    //       and when it succeeds, emit dataChanged(row).
    switch (role) {
        case AccountIdRole:     return m_data[row].accountId;
        case UserIdRole:        return m_data[row].userId;
        case AlbumIdRole:       return m_data[row].albumId;
        case ParentAlbumIdRole: return m_data[row].parentAlbumId;
        case AlbumNameRole:     return m_data[row].albumName;
        case PhotoCountRole:    return m_data[row].photoCount;
        case ThumbnailUrlRole:  return m_data[row].thumbnailUrl;
        case ThumbnailPathRole: return m_data[row].thumbnailPath;
        case ThumbnailFileNameRole: return m_data[row].thumbnailFileName;
        default:                return QVariant();
    }
}

int NextcloudAlbumModel::rowCount(const QModelIndex &) const
{
    return m_data.size();
}

QHash<int, QByteArray> NextcloudAlbumModel::roleNames() const
{
    static QHash<int, QByteArray> retn {
        { AccountIdRole,        "accountId" },
        { UserIdRole,           "userId" },
        { AlbumIdRole,          "albumId" },
        { ParentAlbumIdRole,    "parentAlbumId" },
        { AlbumNameRole,        "albumName" },
        { PhotoCountRole,       "photoCount" },
        { ThumbnailUrlRole,     "thumbnailUrl" },
        { ThumbnailPathRole,    "thumbnailPath" },
        { ThumbnailFileNameRole, "thumbnailFileName" },
    };

    return retn;
}

SyncCache::ImageCache *NextcloudAlbumModel::imageCache() const
{
    return m_imageCache;
}

void NextcloudAlbumModel::setImageCache(SyncCache::ImageCache *cache)
{
    if (m_imageCache == cache) {
        return;
    }

    if (m_imageCache) {
        disconnect(m_imageCache, 0, this, 0);
    }

    m_imageCache = cache;
    emit imageCacheChanged();

    if (!m_deferLoad) {
        loadData();
    }

    connect(m_imageCache, &SyncCache::ImageCache::albumsStored,
            this, [this] (const QVector<SyncCache::Album> &albums) {
        for (const SyncCache::Album &album : albums) {
            bool foundAlbum = false;
            for (int row = 0; row < m_data.size(); ++row) {
                const SyncCache::Album &existing(m_data[row]);
                if (album.accountId == existing.accountId
                        && album.userId == existing.userId
                        && album.albumId == existing.albumId) {
                    m_data.replace(row, album);
                    emit dataChanged(index(row, 0, QModelIndex()), index(row, 0, QModelIndex()));
                    foundAlbum = true;
                }
            }

            if (!foundAlbum
                    && (album.accountId == accountId() || accountId() == 0)
                    && (album.userId == userId() || userId().isEmpty())) {
                emit beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
                m_data.append(album);
                emit endInsertRows();
                emit rowCountChanged();
            }
        }
    });

    connect(m_imageCache, &SyncCache::ImageCache::albumsDeleted,
            this, [this] (const QVector<SyncCache::Album> &albums) {
        for (const SyncCache::Album &album : albums) {
            for (int row = 0; row < m_data.size(); ++row) {
                const SyncCache::Album &existing(m_data[row]);
                if (album.accountId == existing.accountId
                        && album.userId == existing.userId
                        && album.albumId == existing.albumId) {
                    emit beginRemoveRows(QModelIndex(), row, row);
                    m_data.remove(row);
                    emit endRemoveRows();
                    emit rowCountChanged();
                }
            }
        }
    });
}

int NextcloudAlbumModel::accountId() const
{
    return m_accountId;
}

void NextcloudAlbumModel::setAccountId(int id)
{
    if (m_accountId == id) {
        return;
    }

    m_accountId = id;
    emit accountIdChanged();

    if (!m_deferLoad) {
        loadData();
    }
}

QString NextcloudAlbumModel::userId() const
{
    return m_userId;
}

void NextcloudAlbumModel::setUserId(const QString &id)
{
    if (m_userId == id) {
        return;
    }

    m_userId = id;
    emit userIdChanged();

    if (!m_deferLoad) {
        loadData();
    }
}

QVariantMap NextcloudAlbumModel::at(int row) const
{
    QVariantMap retn;
    if (row < 0 || row >= rowCount()) {
        return retn;
    }

    const QHash<int, QByteArray> roles = roleNames();
    QHash<int, QByteArray>::const_iterator it = roles.constBegin();
    QHash<int, QByteArray>::const_iterator end = roles.constEnd();
    for ( ; it != end; it++) {
        retn.insert(QString::fromLatin1(it.value()), data(index(row, 0, QModelIndex()), it.key()));
    }
    return retn;
}

void NextcloudAlbumModel::loadData()
{
    if (!m_imageCache) {
        return;
    }

    if (m_accountId <= 0) {
        return;
    }

    if (m_userId.isEmpty()) {
        return;
    }

    QObject *contextObject = new QObject(this);
    connect(m_imageCache, &SyncCache::ImageCache::requestAlbumsFinished,
            contextObject, [this, contextObject] (int accountId,
                                                  const QString &userId,
                                                  const QVector<SyncCache::Album> &albums) {
        if (accountId != this->accountId()
                || userId != this->userId()) {
            return;
        }
        contextObject->deleteLater();
        const int oldSize = m_data.size();
        if (m_data.size()) {
            emit beginRemoveRows(QModelIndex(), 0, m_data.size() - 1);
            m_data.clear();
            emit endRemoveRows();
        }
        if (albums.size()) {
            emit beginInsertRows(QModelIndex(), 0, albums.size() - 1);
            m_data = albums;
            emit endInsertRows();
        }
        if (m_data.size() != oldSize) {
            emit rowCountChanged();
        }
    });
    connect(m_imageCache, &SyncCache::ImageCache::requestAlbumsFailed,
            contextObject, [this, contextObject] (int accountId,
                                                  const QString &userId,
                                                  const QString &errorMessage) {
        if (accountId != this->accountId()
                || userId != this->userId()) {
            return;
        }
        contextObject->deleteLater();
        qWarning() << "NextcloudAlbumModel::loadData: failed:" << errorMessage;
    });
    m_imageCache->requestAlbums(m_accountId, m_userId);
}

//-----------------------------------------------------------------------------

NextcloudPhotoModel::NextcloudPhotoModel(QObject *parent)
    : QAbstractListModel(parent)
{
    qRegisterMetaType<SyncCache::Photo>();
    qRegisterMetaType<QVector<SyncCache::Photo> >();
}

void NextcloudPhotoModel::classBegin()
{
    m_deferLoad = true;
}

void NextcloudPhotoModel::componentComplete()
{
    m_deferLoad = false;
    if (m_imageCache) {
        loadData();
    }
}

QModelIndex NextcloudPhotoModel::index(int row, int column, const QModelIndex &parent) const
{
    return !parent.isValid() && column == 0 && row >= 0 && row < m_data.size()
            ? createIndex(row, column)
            : QModelIndex();
}

QVariant NextcloudPhotoModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (!index.isValid() || row < 0 || row >= m_data.size()) {
        return QVariant();
    }

    // TODO: if thumbnail path is requested but empty,
    //       call m_cache->populatePhotoThumbnail(),
    //       and when it succeeds, emit dataChanged(row).
    //       similarly for image path.
    switch (role) {
        case AccountIdRole:         return m_data[row].accountId;
        case UserIdRole:            return m_data[row].userId;
        case AlbumIdRole:           return m_data[row].albumId;
        case PhotoIdRole:           return m_data[row].photoId;
        case CreatedTimestampRole:  return m_data[row].createdTimestamp;
        case UpdatedTimestampRole:  return m_data[row].updatedTimestamp;
        case FileNameRole:          return m_data[row].fileName;
        case AlbumPathRole:         return m_data[row].albumPath;
        case DescriptionRole:       return m_data[row].description;
        case ThumbnailUrlRole:      return m_data[row].thumbnailUrl;
        case ThumbnailPathRole:     return m_data[row].thumbnailPath;
        case ImageUrlRole:          return m_data[row].imageUrl;
        case ImagePathRole:         return m_data[row].imagePath;
        case ImageWidthRole:        return m_data[row].imageWidth;
        case ImageHeightRole:       return m_data[row].imageHeight;
        case FileSizeRole:          return m_data[row].fileSize;
        case FileTypeRole:          return m_data[row].fileType;
        default:                    return QVariant();
    }
}

int NextcloudPhotoModel::rowCount(const QModelIndex &) const
{
    return m_data.size();
}

QHash<int, QByteArray> NextcloudPhotoModel::roleNames() const
{
    static QHash<int, QByteArray> retn {
        { AccountIdRole,        "accountId" },
        { UserIdRole,           "userId" },
        { AlbumIdRole,          "albumId" },
        { PhotoIdRole,          "photoId" },
        { CreatedTimestampRole, "createdTimestamp" },
        { UpdatedTimestampRole, "updatedTimestamp" },
        { FileNameRole,         "fileName" },
        { AlbumPathRole,        "albumPath" },
        { DescriptionRole,      "description" },
        { ThumbnailUrlRole,     "thumbnailUrl" },
        { ThumbnailPathRole,    "thumbnailPath" },
        { ImageUrlRole,         "imageUrl" },
        { ImagePathRole,        "imagePath" },
        { ImageWidthRole,       "imageWidth" },
        { ImageHeightRole,      "imageHeight" },
        { FileSizeRole,         "fileSize" },
        { FileTypeRole,         "fileType" },
    };

    return retn;
}

SyncCache::ImageCache *NextcloudPhotoModel::imageCache() const
{
    return m_imageCache;
}

void NextcloudPhotoModel::setImageCache(SyncCache::ImageCache *cache)
{
    if (m_imageCache == cache) {
        return;
    }

    if (m_imageCache) {
        disconnect(m_imageCache, 0, this, 0);
    }

    m_imageCache = cache;
    emit imageCacheChanged();

    if (!m_deferLoad) {
        loadData();
    }

    connect(m_imageCache, &SyncCache::ImageCache::photosStored,
            this, [this] (const QVector<SyncCache::Photo> &photos) {
        for (const SyncCache::Photo &photo : photos) {
            bool foundPhoto = false;
            for (int row = 0; row < m_data.size(); ++row) {
                const SyncCache::Photo &existing(m_data[row]);
                if (photo.accountId == existing.accountId
                        && photo.userId == existing.userId
                        && photo.albumId == existing.albumId
                        && photo.photoId == existing.photoId) {
                    m_data.replace(row, photo);
                    emit dataChanged(index(row, 0, QModelIndex()), index(row, 0, QModelIndex()));
                    foundPhoto = true;
                }
            }

            if (!foundPhoto
                    && (photo.accountId == accountId() || accountId() == 0)
                    && (photo.userId == userId() || userId().isEmpty())
                    && (photo.albumId == albumId() || albumId().isEmpty())) {
                emit beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
                m_data.append(photo);
                emit endInsertRows();
                emit rowCountChanged();
            }
        }
    });

    connect(m_imageCache, &SyncCache::ImageCache::photosDeleted,
            this, [this] (const QVector<SyncCache::Photo> &photos) {
        for (const SyncCache::Photo &photo : photos) {
            for (int row = 0; row < m_data.size(); ++row) {
                const SyncCache::Photo &existing(m_data[row]);
                if (photo.accountId == existing.accountId
                        && photo.userId == existing.userId
                        && photo.albumId == existing.albumId
                        && photo.photoId == existing.photoId) {
                    emit beginRemoveRows(QModelIndex(), row, row);
                    m_data.remove(row);
                    emit endRemoveRows();
                    emit rowCountChanged();
                }
            }
        }
    });
}

int NextcloudPhotoModel::accountId() const
{
    return m_accountId;
}

void NextcloudPhotoModel::setAccountId(int id)
{
    if (m_accountId == id) {
        return;
    }

    m_accountId = id;
    emit accountIdChanged();

    if (!m_deferLoad) {
        loadData();
    }
}

QString NextcloudPhotoModel::userId() const
{
    return m_userId;
}

void NextcloudPhotoModel::setUserId(const QString &id)
{
    if (m_userId == id) {
        return;
    }

    m_userId = id;
    emit userIdChanged();

    if (!m_deferLoad) {
        loadData();
    }
}

QString NextcloudPhotoModel::albumId() const
{
    return m_albumId;
}

void NextcloudPhotoModel::setAlbumId(const QString &id)
{
    if (m_albumId == id) {
        return;
    }

    m_albumId = id;
    emit albumIdChanged();

    if (!m_deferLoad) {
        loadData();
    }
}

QVariantMap NextcloudPhotoModel::at(int row) const
{
    QVariantMap retn;
    if (row < 0 || row >= rowCount()) {
        return retn;
    }

    const QHash<int, QByteArray> roles = roleNames();
    QHash<int, QByteArray>::const_iterator it = roles.constBegin();
    QHash<int, QByteArray>::const_iterator end = roles.constEnd();
    for ( ; it != end; it++) {
        retn.insert(QString::fromLatin1(it.value()), data(index(row, 0, QModelIndex()), it.key()));
    }
    return retn;
}

void NextcloudPhotoModel::loadData()
{
    if (!m_imageCache || m_accountId < 0) {
        return;
    }

    QObject *contextObject = new QObject(this);
    connect(m_imageCache, &SyncCache::ImageCache::requestPhotosFinished,
            contextObject, [this, contextObject] (int accountId,
                                                  const QString &userId,
                                                  const QString &albumId,
                                                  const QVector<SyncCache::Photo> &photos) {
        if (accountId != this->accountId()
                || userId != this->userId()
                || albumId != this->albumId()) {
            return;
        }
        contextObject->deleteLater();
        const int oldSize = m_data.size();
        if (m_data.size()) {
            emit beginRemoveRows(QModelIndex(), 0, m_data.size() - 1);
            m_data.clear();
            emit endRemoveRows();
        }
        if (photos.size()) {
            emit beginInsertRows(QModelIndex(), 0, photos.size() - 1);
            m_data = photos;
            emit endInsertRows();
        }
        if (m_data.size() != oldSize) {
            emit rowCountChanged();
        }
    });
    connect(m_imageCache, &SyncCache::ImageCache::requestPhotosFailed,
            contextObject, [this, contextObject] (int accountId,
                                                  const QString &userId,
                                                  const QString &albumId,
                                                  const QString &errorMessage) {
        if (accountId != this->accountId()
                || userId != this->userId()
                || albumId != this->albumId()) {
            return;
        }
        contextObject->deleteLater();
        qWarning() << "NextcloudPhotoModel::loadData: failed:" << errorMessage;
    });
    m_imageCache->requestPhotos(m_accountId, m_userId, m_albumId);
}

//-----------------------------------------------------------------------------

NextcloudPhotoCounter::NextcloudPhotoCounter(QObject *parent)
    : QObject(parent)
{
}

SyncCache::ImageCache *NextcloudPhotoCounter::imageCache() const
{
    return m_imageCache;
}

void NextcloudPhotoCounter::setImageCache(SyncCache::ImageCache *cache)
{
    if (m_imageCache == cache) {
        return;
    }

    if (m_imageCache) {
        disconnect(m_imageCache, 0, this, 0);
    }

    m_imageCache = cache;
    emit imageCacheChanged();

    connect(m_imageCache, &SyncCache::ImageCache::requestPhotoCountFinished,
            this, [this] (int photoCount) {
        if (m_count != photoCount) {
            m_count = photoCount;
            emit countChanged();
        }
    });
    connect(m_imageCache, &SyncCache::ImageCache::requestPhotoCountFailed,
            this, [this] (const QString &errorMessage) {
        qmlInfo(this) << "Request total photo count failed:" << errorMessage;
    });

    connect(m_imageCache, &SyncCache::ImageCache::photosStored,
            m_imageCache, &SyncCache::ImageCache::requestPhotoCount);
    connect(m_imageCache, &SyncCache::ImageCache::photosDeleted,
            m_imageCache, &SyncCache::ImageCache::requestPhotoCount);
    m_imageCache->requestPhotoCount();
}

int NextcloudPhotoCounter::count()
{
    return m_count;
}
