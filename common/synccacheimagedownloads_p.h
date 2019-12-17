/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_SYNCCACHEIMAGEDOWNLOADS_P_H
#define NEXTCLOUD_SYNCCACHEIMAGEDOWNLOADS_P_H

#include <QtCore/QQueue>
#include <QtCore/QTimer>
#include <QtCore/QPointer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

namespace SyncCache {

QString imageDownloadDir(int accountId);
QString userImageDownloadDir(int accountId, const QString &userId, bool thumbnail);
QString albumImageDownloadDir(int accountId, const QString &albumName, bool thumbnail);


class ImageDownloadWatcher : public QObject
{
    Q_OBJECT

public:
    ImageDownloadWatcher(int idempToken, QObject *parent = nullptr);
    ~ImageDownloadWatcher();

    int idempToken() const;

Q_SIGNALS:
    void downloadFailed(const QString &errorMessage);
    void downloadFinished(const QUrl &filePath);

private:
    int m_idempToken;
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
    ImageDownloader(QObject *parent = nullptr);
    ~ImageDownloader();

    ImageDownloadWatcher *downloadImage(int idempToken,
            const QUrl &imageUrl,
            const QString &fileName,
            const QString &fileDirPath,
            const QNetworkRequest &templateRequest);

private Q_SLOTS:
    void triggerDownload();

private:
    void eraseActiveDownload(ImageDownload *download);

    QNetworkAccessManager m_qnam;
    QQueue<ImageDownload*> m_pending;
    QQueue<ImageDownload*> m_active;
    int m_maxActive;
};

class BatchedImageDownload
{
public:
    BatchedImageDownload();
    BatchedImageDownload(int idempToken,
                         const QString &photoId,
                         const QString &filePath,
                         SyncCache::ImageDownloadWatcher *watcher);
    BatchedImageDownload &operator=(const BatchedImageDownload &other);

    int m_idempToken = 0;
    QString m_photoId;
    QString m_filePath;
    QPointer<SyncCache::ImageDownloadWatcher> m_watcher;
};

class BatchedImageDownloader : public QObject
{
    Q_OBJECT

public:
    BatchedImageDownloader(const QUrl &templateUrl,
                           const QNetworkRequest &templateRequest,
                           QObject *parent);

    ImageDownloadWatcher *downloadImage(int idempToken,
                                        const QString &photoId,
                                        const QString &fileName,
                                        const QString &fileDirPath);

private:
    struct ImagePreview {
        QString photoId;
        QString mimeType;
        QByteArray bytes;
    };

    void triggerDownload();
    void readImageData();
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished();
    bool parseImagePreview(const QByteArray &previewData, ImagePreview *preview);

    QTimer *m_batchTimer = nullptr;
    QNetworkAccessManager m_qnam;
    QQueue<BatchedImageDownload *> m_pending;
    QMap<QString, BatchedImageDownload *> m_active;
    QNetworkRequest m_templateRequest;
    QByteArray m_buffer;
    QString m_host;
};

}

#endif
