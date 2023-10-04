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

#ifndef NEXTCLOUD_GALLERY_IMAGEDOWNLOADER_H
#define NEXTCLOUD_GALLERY_IMAGEDOWNLOADER_H

#include "synccacheimages.h"

#include <QtCore/QUrl>
#include <QtQml/QQmlParserStatus>

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
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)

public:
    enum Status {
        Null,
        Ready,
        Downloading,
        Error
    };
    Q_ENUM(Status)

    explicit NextcloudImageDownloader(QObject *parent = nullptr);

    // QQmlParserStatus
    void classBegin() override;
    void componentComplete() override;

    SyncCache::ImageCache *imageCache() const;
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
    Status status() const;

Q_SIGNALS:
    void imageCacheChanged();
    void accountIdChanged();
    void userIdChanged();
    void albumIdChanged();
    void photoIdChanged();
    void downloadThumbnailChanged();
    void downloadImageChanged();
    void imagePathChanged();
    void statusChanged();

private:
    void loadImage();
    void setStatus(Status status);
    void populateFinished(int idempToken, const QString &path);
    void populateFailed(int idempToken, const QString &errorMessage);

    bool m_deferLoad = false;
    SyncCache::ImageCache *m_imageCache = nullptr;
    int m_accountId = 0;
    Status m_status = Null;
    QString m_userId;
    QString m_albumId;
    QString m_photoId;
    bool m_downloadThumbnail = false;
    bool m_downloadImage = false;
    QUrl m_imagePath;
    int m_idempToken = 0;
};

#endif // NEXTCLOUD_GALLERY_IMAGEDOWNLOADER_H
