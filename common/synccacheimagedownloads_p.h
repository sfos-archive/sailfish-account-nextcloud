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

class ImageDownloadWatcher : public QObject
{
    Q_OBJECT

public:
    ImageDownloadWatcher(int idempToken, const QUrl &imageUrl, QObject *parent = nullptr);
    ~ImageDownloadWatcher();

    int idempToken() const;
    QUrl imageUrl() const;

Q_SIGNALS:
    void downloadFailed(const QString &errorMessage);
    void downloadFinished(const QUrl &filePath);

private:
    int m_idempToken;
    QUrl m_imageUrl;
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
    ImageDownloader(int maxActive = 4, QObject *parent = nullptr);
    ~ImageDownloader();

    ImageDownloadWatcher *downloadImage(int idempToken,
            const QUrl &imageUrl,
            const QString &fileName,
            const QString &fileDirPath,
            const QNetworkRequest &requestTemplate);

    QString userImageDownloadDir(int accountId, const QString &userId, bool thumbnail) const;
    QString albumImageDownloadDir(int accountId, const QString &albumName, bool thumbnail) const;

    static QString imageDownloadDir(int accountId);

private Q_SLOTS:
    void triggerDownload();

private:
    void eraseActiveDownload(ImageDownload *download);

    QNetworkAccessManager m_qnam;
    QQueue<ImageDownload*> m_pending;
    QQueue<ImageDownload*> m_active;
    int m_maxActive;
};

}

#endif
