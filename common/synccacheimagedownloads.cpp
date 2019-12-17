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
#include <QtCore/QUrlQuery>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonParseError>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>

using namespace SyncCache;

namespace {

const int ImageDownloadTimeout = 60 * 1000;
const int MaxActiveImageRequests = 10;
const int BatchCoalesceInterval = 250;

}

QString SyncCache::imageDownloadDir(int accountId)
{
    return QStringLiteral("%1/nextcloud/account-%2")
            .arg(ImageCache::imageCacheRootDir()).arg(accountId);
}

QString SyncCache::userImageDownloadDir(int accountId, const QString &userId, bool thumbnail)
{
    const QString path = thumbnail
            ? SyncCache::imageDownloadDir(accountId) + "/users-thumbnails"
            : SyncCache::imageDownloadDir(accountId) + "/users";
    return userId.isEmpty() ? path : path + "/" + userId;
}

QString SyncCache::albumImageDownloadDir(int accountId, const QString &albumName, bool thumbnail)
{
    const QString path = thumbnail
            ? SyncCache::imageDownloadDir(accountId) + "/albums-thumbnails"
            : SyncCache::imageDownloadDir(accountId) + "/albums";
    return albumName.isEmpty() ? path : path + "/" + albumName;
}

//-----------------------------------------------------------------------------

ImageDownloadWatcher::ImageDownloadWatcher(int idempToken, QObject *parent)
    : QObject(parent)
    , m_idempToken(idempToken)
{
}

ImageDownloadWatcher::~ImageDownloadWatcher()
{
}

int ImageDownloadWatcher::idempToken() const
{
    return m_idempToken;
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

ImageDownloader::ImageDownloader(QObject *parent)
    : QObject(parent)
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
    ImageDownloadWatcher *watcher = new ImageDownloadWatcher(idempToken, this);
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

    while (m_active.size() < MaxActiveImageRequests && m_pending.size()) {
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

//-----------------------------------------------------------------------------

BatchedImageDownload::BatchedImageDownload()
{
}

BatchedImageDownload::BatchedImageDownload(int idempToken,
                                           const QString &photoId,
                                           const QString &filePath,
                                           SyncCache::ImageDownloadWatcher *watcher)
    : m_idempToken(idempToken)
    , m_photoId(photoId)
    , m_filePath(filePath)
    , m_watcher(watcher)
{
}

BatchedImageDownload &BatchedImageDownload::operator=(const BatchedImageDownload &other)
{
    if (this == &other) {
        return *this;
    }

    m_idempToken = other.m_idempToken;
    m_photoId = other.m_photoId;
    m_filePath = other.m_filePath;
    m_watcher = other.m_watcher;

    return *this;
}

//-----------------------------------------------------------------------------

BatchedImageDownloader::BatchedImageDownloader(const QUrl &templateUrl,
                                               const QNetworkRequest &templateRequest,
                                               QObject *parent)
    : QObject(parent)
    , m_templateRequest(templateRequest)
{
    QUrl imageUrl = templateUrl;
    imageUrl.setHost(templateUrl.host());
    imageUrl.setPath("/index.php/apps/gallery/api/thumbnails");
    imageUrl.setUserName(templateUrl.userName());
    imageUrl.setPassword(templateUrl.password());
    m_templateRequest.setUrl(imageUrl);
}

ImageDownloadWatcher *BatchedImageDownloader::downloadImage(int idempToken,
                                                            const QString &photoId,
                                                            const QString &fileName,
                                                            const QString &fileDirPath)
{
    QDir dir(fileDirPath);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    ImageDownloadWatcher *watcher = new ImageDownloadWatcher(idempToken, this);
    BatchedImageDownload *download = new BatchedImageDownload(idempToken, photoId, dir.absoluteFilePath(fileName), watcher);
    m_pending.enqueue(download);

    if (!m_batchTimer) {
        m_batchTimer = new QTimer(this);
        m_batchTimer->setInterval(BatchCoalesceInterval);
        m_batchTimer->setSingleShot(true);
        connect(m_batchTimer, &QTimer::timeout, this, &BatchedImageDownloader::triggerDownload);
    }

    m_batchTimer->start();
    return watcher; // caller takes ownership.
}

void BatchedImageDownloader::triggerDownload()
{
    while (m_active.size() < MaxActiveImageRequests && m_pending.size()) {
        BatchedImageDownload *download = m_pending.dequeue();
        m_active.insert(download->m_photoId, download);
    }

    if (m_active.isEmpty()) {
        return;
    }

    QStringList photoIds = m_active.keys();
    QNetworkRequest request(m_templateRequest);
    QUrl imageUrl = request.url();

    // The 'scale' parameter is not well-documented as the API is still in beta. Currently
    // square=true and scale=1.0 gives the 256x256 dimensions we require.
    imageUrl.setQuery(QUrlQuery(QStringLiteral("square=true&scale=1.0&ids=%1").arg(photoIds.join(';'))));
    request.setUrl(imageUrl);

    QNetworkReply *reply = m_qnam.get(request);
    connect(reply, &QNetworkReply::finished, this, &BatchedImageDownloader::downloadFinished);
    connect(reply, &QNetworkReply::downloadProgress, this, &BatchedImageDownloader::downloadProgress);
}

void BatchedImageDownloader::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    Q_UNUSED(bytesReceived)
    Q_UNUSED(bytesTotal)
    readImageData();
}

void BatchedImageDownloader::readImageData()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

    QByteArray bytes = reply->readLine();
    while (!bytes.isEmpty()) {
        if (!bytes.endsWith("\n")) {
            // Wait for the rest of the line to arrive
            m_buffer += bytes;
            break;
        } else if (m_buffer.size() > 0) {
            bytes = m_buffer + bytes;
            m_buffer.clear();
        }

        if (bytes.startsWith("data: \"close\"")) {
            break;
        }

        static const QByteArray dataKey = QByteArrayLiteral("data:");
        if (bytes.startsWith(dataKey)) {
            ImagePreview preview;
            if (!parseImagePreview(bytes.mid(dataKey.length()), &preview)) {
                qWarning() << "Unable to parse image preview data from server response!";
            } else {
                BatchedImageDownload *download = m_active.take(preview.photoId);
                if (download->m_photoId.isEmpty()) {
                    qWarning() << "Error! Cannot find active download" << preview.photoId;
                } else {
                    QFile file(download->m_filePath);
                    if (!file.open(QFile::WriteOnly)) {
                        if (download->m_watcher) {
                            emit download->m_watcher->downloadFailed(QStringLiteral("Error opening image file %1 for writing: %2")
                                                                    .arg(file.fileName(), file.errorString()));
                        }
                    } else {
                        file.write(preview.bytes);
                        file.close();
                        if (download->m_watcher) {
                            emit download->m_watcher->downloadFinished(file.fileName());
                        }
                    }
                }
                delete download;
            }
        }
        bytes = reply->readLine();
    }
}

void BatchedImageDownloader::downloadFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

    for (BatchedImageDownload *download : m_active.values()) {
        if (download->m_watcher) {
            if (reply->error() == QNetworkReply::NoError) {
                emit download->m_watcher->downloadFailed(QStringLiteral("No image data received for %1").arg(download->m_photoId));
            } else {
                emit download->m_watcher->downloadFailed(QStringLiteral("Image download error: %1").arg(reply->errorString()));
            }
        }
        delete download;
    }
    m_active.clear();
    m_buffer.clear();

    reply->deleteLater();

    if (!m_pending.isEmpty()) {
        m_batchTimer->start();
    }
}

bool BatchedImageDownloader::parseImagePreview(const QByteArray &previewData, ImagePreview *preview)
{
    /*
      Expect data like:
      {"preview": <base64-encoded-image-data>, "mimetype":"image\/jpeg","fileid":"777","status":200}
    */

    if (!preview) {
        qWarning() << "ImagePreview is invalid!";
        return false;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(previewData, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "JSON parsing failed:" << err.errorString();
        return false;
    }

    const QJsonObject json = doc.object();

    const QString photoId = json.value(QLatin1String("fileid")).toString();
    if (photoId.isEmpty()) {
        qWarning() << "Failed to read photoId!";
        return false;
    }

    int status = json.value(QLatin1String("status")).toInt();
    if (status != 200) {
        qWarning() << "Image preview for" << photoId << "has non-success code:" << status;
        return false;
    }

    preview->bytes = QByteArray::fromBase64(json.value(QLatin1String("preview")).toString().toUtf8());
    if (preview->bytes.isEmpty()) {
        qWarning() << "Failed to read image preview data for photo" << photoId;
        return false;
    }

    preview->photoId = photoId;
    preview->mimeType = json.value(QLatin1String("mimetype")).toString();
    return true;
}
