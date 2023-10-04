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
    enum Status {
        InProgress,
        Downloaded,
        Error
    };
    ImageDownload(int idempToken = 0,
            const QUrl &imageUrl = QUrl(),
            const QString &fileName = QString(),
            const QString &fileDirPath = QString(),
            const QNetworkRequest &templateRequest = QNetworkRequest(QUrl()),
            ImageDownloadWatcher *watcher = nullptr);
    ~ImageDownload();

    void setStatus(Status status, const QString &error = QString());

    Status m_status = InProgress;
    int m_idempToken = 0;
    QUrl m_imageUrl;
    QString m_fileName;
    QString m_fileDirPath;
    QString m_errorString;
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
    void eraseInactiveDownloads();

private:
    QNetworkAccessManager m_qnam;
    QQueue<ImageDownload*> m_pending;
    QQueue<ImageDownload*> m_active;
    int m_maxActive;
};

}

#endif
