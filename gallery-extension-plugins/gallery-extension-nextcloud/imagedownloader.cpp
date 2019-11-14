/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "imagedownloader.h"
#include "imagecache.h"

#include <QtCore/QDebug>
#include <QtQml/QQmlInfo>

NextcloudImageDownloader::NextcloudImageDownloader(QObject *parent)
    : QObject(parent)
{
}

void NextcloudImageDownloader::classBegin()
{
    m_deferLoad = true;
}

void NextcloudImageDownloader::componentComplete()
{
    m_deferLoad = false;
    if (m_imageCache) {
        loadImage();
    }
}

SyncCache::ImageCache *NextcloudImageDownloader::imageCache() const
{
    return m_imageCache;
}

void NextcloudImageDownloader::setImageCache(SyncCache::ImageCache *cache)
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
        loadImage();
    }
}

int NextcloudImageDownloader::accountId() const
{
    return m_accountId;
}

void NextcloudImageDownloader::setAccountId(int id)
{
    if (m_accountId == id) {
        return;
    }

    m_accountId = id;
    emit accountIdChanged();

    if (!m_deferLoad) {
        loadImage();
    }
}

QString NextcloudImageDownloader::userId() const
{
    return m_userId;
}

void NextcloudImageDownloader::setUserId(const QString &id)
{
    if (m_userId == id) {
        return;
    }

    m_userId = id;
    emit userIdChanged();

    if (!m_deferLoad) {
        loadImage();
    }
}

QString NextcloudImageDownloader::albumId() const
{
    return m_albumId;
}

void NextcloudImageDownloader::setAlbumId(const QString &id)
{
    if (m_albumId == id) {
        return;
    }

    m_albumId = id;
    emit albumIdChanged();

    if (!m_deferLoad) {
        loadImage();
    }
}

QString NextcloudImageDownloader::photoId() const
{
    return m_photoId;
}
void NextcloudImageDownloader::setPhotoId(const QString &id)
{
    if (m_photoId == id) {
        return;
    }

    m_photoId = id;
    emit photoIdChanged();

    if (!m_deferLoad) {
        loadImage();
    }
}

bool NextcloudImageDownloader::downloadThumbnail() const
{
    return m_downloadThumbnail;
}

void NextcloudImageDownloader::setDownloadThumbnail(bool v)
{
    if (m_downloadThumbnail != v) {
        m_downloadThumbnail = v;
        emit downloadThumbnailChanged();
        if (!m_deferLoad) {
            loadImage();
        }
    }
}

bool NextcloudImageDownloader::downloadImage() const
{
    return m_downloadImage;
}

void NextcloudImageDownloader::setDownloadImage(bool v)
{
    if (m_downloadImage != v) {
        m_downloadImage = v;
        emit downloadImageChanged();
        if (!m_deferLoad) {
            loadImage();
        }
    }
}

QUrl NextcloudImageDownloader::imagePath() const
{
    return m_imagePath;
}

void NextcloudImageDownloader::loadImage()
{
    if (!m_imageCache) {
        qWarning() << "No image cache, cannot load image";
        return;
    }

    if (m_downloadImage) {
        m_idempToken = qHash(QStringLiteral("%1|%2|%3").arg(m_userId, m_albumId, m_photoId));

        connect(m_imageCache, &SyncCache::ImageCache::populatePhotoImageFinished,
                this, &NextcloudImageDownloader::populateFinished,
                Qt::UniqueConnection);
        connect(m_imageCache, &SyncCache::ImageCache::populatePhotoImageFailed,
                this, &NextcloudImageDownloader::populateFailed,
                Qt::UniqueConnection);

        m_imageCache->populatePhotoImage(m_idempToken, m_accountId, m_userId, m_albumId, m_photoId, QNetworkRequest());

    } else if (m_downloadThumbnail) {
        m_idempToken = qHash(QStringLiteral("%1|%2|%3").arg(m_userId, m_albumId, m_photoId));
        NextcloudImageCache *nextcloudImageCache = qobject_cast<NextcloudImageCache*>(m_imageCache);
        const QNetworkRequest networkRequest = nextcloudImageCache
                ? nextcloudImageCache->templateRequest(m_accountId, true)
                : QNetworkRequest();

        if (m_photoId.isEmpty()) {
            connect(m_imageCache, &SyncCache::ImageCache::populateAlbumThumbnailFinished,
                    this, &NextcloudImageDownloader::populateFinished,
                    Qt::UniqueConnection);
            connect(m_imageCache, &SyncCache::ImageCache::populateAlbumThumbnailFailed,
                    this, &NextcloudImageDownloader::populateFailed,
                    Qt::UniqueConnection);
            m_imageCache->populateAlbumThumbnail(m_idempToken, m_accountId, m_userId, m_albumId, networkRequest);

        } else {
            connect(m_imageCache, &SyncCache::ImageCache::populatePhotoThumbnailFinished,
                    this, &NextcloudImageDownloader::populateFinished,
                    Qt::UniqueConnection);
            connect(m_imageCache, &SyncCache::ImageCache::populatePhotoThumbnailFailed,
                    this, &NextcloudImageDownloader::populateFailed,
                    Qt::UniqueConnection);
            m_imageCache->populatePhotoThumbnail(m_idempToken, m_accountId, m_userId, m_albumId, m_photoId, networkRequest);
        }
    }
}

void NextcloudImageDownloader::populateFinished(int idempToken, const QString &path)
{
    if (m_idempToken == idempToken) {
        const QUrl imagePath = QUrl::fromLocalFile(path);
        if (m_imagePath != imagePath) {
            m_imagePath = imagePath;
            emit imagePathChanged();
        }
    }
}

void NextcloudImageDownloader::populateFailed(int idempToken, const QString &errorMessage)
{
    if (m_idempToken == idempToken) {
        qWarning() << "NextcloudImageDownloader failed to load image:" << errorMessage;
    }
}
