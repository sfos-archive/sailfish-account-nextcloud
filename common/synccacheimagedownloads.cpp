/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "synccacheimages.h"
#include "synccacheimages_p.h"
#include "synccacheimagedownloads_p.h"

#include <QtCore/QThread>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtCore/QDebug>

using namespace SyncCache;

namespace {

static const int ImageDownloadTimeout = 60 * 1000;

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

ImageDownload::ImageDownload(int idempToken,
        const QUrl &imageUrl,
        const QString &fileName,
        const QString &fileDirPath,
        const QNetworkRequest &templateRequest,
        ImageDownloadWatcher *watcher)
    : m_idempToken(idempToken)
    , m_imageUrl(imageUrl)
    , m_fileName(fileName)
    , m_fileDirPath(fileDirPath)
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
    : QObject(parent)
    , m_maxActive(maxActive > 0 && maxActive < 20 ? maxActive : 4)
{
}

ImageDownloader::~ImageDownloader()
{
}

ImageDownloadWatcher *ImageDownloader::downloadImage(int idempToken,
                                                     const QUrl &imageUrl,
                                                     const QString &fileName,
                                                     const QString &fileDirPath,
                                                     const QNetworkRequest &templateRequest)
{
    ImageDownloadWatcher *watcher = new ImageDownloadWatcher(idempToken, imageUrl, this);
    ImageDownload *download = new ImageDownload(idempToken, imageUrl, fileName, fileDirPath, templateRequest, watcher);
    m_pending.enqueue(download);
    QMetaObject::invokeMethod(this, "triggerDownload", Qt::QueuedConnection);
    return watcher; // caller takes ownership.
}

void ImageDownloader::triggerDownload()
{
    while (m_active.size() && (m_active.head() == nullptr || !m_active.head()->m_timeoutTimer->isActive())) {
        delete m_active.dequeue();
    }

    while (m_active.size() < m_maxActive && m_pending.size()) {
        ImageDownload *download = m_pending.dequeue();
        m_active.enqueue(download);

        connect(download->m_timeoutTimer, &QTimer::timeout, this, [this, download] {
            if (download->m_watcher) {
                emit download->m_watcher->downloadFailed(QStringLiteral("Image download timed out for %1")
                                                         .arg(download->m_imageUrl.toString()));
            }
            eraseActiveDownload(download);
        });

        download->m_timeoutTimer->setInterval(ImageDownloadTimeout);
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
                        QDir dir(download->m_fileDirPath);
                        if (!dir.exists()) {
                            dir.mkpath(QStringLiteral("."));
                        }
                        QFile file(dir.absoluteFilePath(download->m_fileName));
                        if (!file.open(QFile::WriteOnly)) {
                            if (download->m_watcher) {
                                emit download->m_watcher->downloadFailed(QStringLiteral("Error opening image file %1 for writing: %2")
                                                                                   .arg(file.fileName(), file.errorString()));
                            }
                        } else {
                            file.write(replyData);
                            file.close();
                            emit download->m_watcher->downloadFinished(file.fileName());
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

QString ImageDownloader::userImageDownloadDir(int accountId, const QString &userId, bool thumbnail) const
{
    const QString path = thumbnail
            ? imageDownloadDir(accountId) + "/users-thumbnails"
            : imageDownloadDir(accountId) + "/users";
    return userId.isEmpty() ? path : path + "/" + userId;
}

QString ImageDownloader::albumImageDownloadDir(int accountId, const QString &albumName, bool thumbnail) const
{
    const QString path = thumbnail
            ? imageDownloadDir(accountId) + "/albums-thumbnails"
            : imageDownloadDir(accountId) + "/albums";
    return albumName.isEmpty() ? path : path + "/" + albumName;
}

QString ImageDownloader::imageDownloadDir(int accountId)
{
    return QStringLiteral("%1/nextcloud/account-%2")
            .arg(ImageCache::imageCacheRootDir()).arg(accountId);
}
