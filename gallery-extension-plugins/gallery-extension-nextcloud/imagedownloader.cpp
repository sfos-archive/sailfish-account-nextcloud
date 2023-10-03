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

NextcloudImageDownloader::Status NextcloudImageDownloader::status() const
{
    return m_status;
}

void NextcloudImageDownloader::setStatus(Status status)
{
    if (status != m_status) {
        m_status = status;
        emit statusChanged();
    }
}

void NextcloudImageDownloader::loadImage()
{
    if (!m_imageCache) {
        return;
    }

    if (m_userId.isEmpty()) {
        qmlInfo(this) << "userId not set, cannot load image";
        return;
    }

    setStatus(Downloading);

    NextcloudImageCache *nextcloudImageCache = qobject_cast<NextcloudImageCache*>(m_imageCache);
    const QNetworkRequest networkRequest = nextcloudImageCache
            ? nextcloudImageCache->templateRequest(m_accountId, true)
            : QNetworkRequest();

    if (!m_albumId.isEmpty()) {
        m_idempToken = qHash(QStringLiteral("%1|%2|%3").arg(m_accountId).arg(m_albumId).arg(m_photoId));

        if (m_downloadImage) {
            // Download photo image
            connect(m_imageCache, &SyncCache::ImageCache::populatePhotoImageFinished,
                    this, &NextcloudImageDownloader::populateFinished,
                    Qt::UniqueConnection);
            connect(m_imageCache, &SyncCache::ImageCache::populatePhotoImageFailed,
                    this, &NextcloudImageDownloader::populateFailed,
                    Qt::UniqueConnection);
            m_imageCache->populatePhotoImage(m_idempToken, m_accountId, m_userId, m_albumId, m_photoId, QNetworkRequest());

        } else if (m_downloadThumbnail) {
            if (m_photoId.isEmpty()) {
                // Download album thumbnail
                connect(m_imageCache, &SyncCache::ImageCache::populateAlbumThumbnailFinished,
                        this, &NextcloudImageDownloader::populateFinished,
                        Qt::UniqueConnection);
                connect(m_imageCache, &SyncCache::ImageCache::populateAlbumThumbnailFailed,
                        this, &NextcloudImageDownloader::populateFailed,
                        Qt::UniqueConnection);
                m_imageCache->populateAlbumThumbnail(m_idempToken, m_accountId, m_userId, m_albumId, networkRequest);

            } else {
                // Download photo thumbnail
                connect(m_imageCache, &SyncCache::ImageCache::populatePhotoThumbnailFinished,
                        this, &NextcloudImageDownloader::populateFinished,
                        Qt::UniqueConnection);
                connect(m_imageCache, &SyncCache::ImageCache::populatePhotoThumbnailFailed,
                        this, &NextcloudImageDownloader::populateFailed,
                        Qt::UniqueConnection);
                m_imageCache->populatePhotoThumbnail(m_idempToken, m_accountId, m_userId, m_albumId, m_photoId, networkRequest);
            }
        }
    } else {
        m_idempToken = qHash(QStringLiteral("%1|%2").arg(m_accountId).arg(m_userId));

        if (m_downloadImage) {
            qmlInfo(this) << "downloadImage option is not supported for user images, set downloadThumbnail instead";

        } else if (m_downloadThumbnail) {
            // Download user thumbnail
            connect(m_imageCache, &SyncCache::ImageCache::populateUserThumbnailFinished,
                    this, &NextcloudImageDownloader::populateFinished,
                    Qt::UniqueConnection);
            connect(m_imageCache, &SyncCache::ImageCache::populateUserThumbnailFailed,
                    this, &NextcloudImageDownloader::populateFailed,
                    Qt::UniqueConnection);
            m_imageCache->populateUserThumbnail(m_idempToken, m_accountId, m_userId, networkRequest);
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
        setStatus(path.isEmpty() ? Error : Ready);
    }
}

void NextcloudImageDownloader::populateFailed(int idempToken, const QString &errorMessage)
{
    if (m_idempToken == idempToken) {
        qmlInfo(this) << "NextcloudImageDownloader failed to load image:" << errorMessage;
        setStatus(Error);
    }
}
