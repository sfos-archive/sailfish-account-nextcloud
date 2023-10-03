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

#ifndef NEXTCLOUD_SYNCCACHEEVENTDOWNLOADS_P_H
#define NEXTCLOUD_SYNCCACHEEVENTDOWNLOADS_P_H

#include <QtCore/QPointer>
#include <QtCore/QQueue>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

namespace SyncCache {

class EventImageDownloadWatcher : public QObject
{
    Q_OBJECT

public:
    EventImageDownloadWatcher(int idempToken, const QUrl &imageUrl, QObject *parent = nullptr);
    ~EventImageDownloadWatcher();

    int idempToken() const;
    QUrl imageUrl() const;

Q_SIGNALS:
    void downloadFailed(const QString &errorMessage);
    void downloadFinished(const QUrl &filePath);

private:
    int m_idempToken;
    QUrl m_imageUrl;
};

class EventImageDownload
{
public:
    EventImageDownload(int idempToken,
            const QUrl &imageUrl,
            const QString &filePath,
            const QNetworkRequest &templateRequest,
            EventImageDownloadWatcher *watcher);
    ~EventImageDownload();

    int m_idempToken = 0;
    QUrl m_imageUrl;
    QString m_filePath;
    QNetworkRequest m_templateRequest;
    QTimer *m_timeoutTimer = nullptr;
    QNetworkReply *m_reply = nullptr;
    QPointer<SyncCache::EventImageDownloadWatcher> m_watcher;
};

class EventImageDownloader : public QObject
{
    Q_OBJECT

public:
    EventImageDownloader(QObject *parent = nullptr);
    ~EventImageDownloader();

    EventImageDownloadWatcher *downloadImage(int idempToken,
            const QUrl &imageUrl,
            const QString &filePath,
            const QNetworkRequest &requestTemplate);

private Q_SLOTS:
    void triggerDownload();

private:
    void eraseActiveDownload(EventImageDownload *download);

    QNetworkAccessManager m_qnam;
    QQueue<EventImageDownload*> m_pending;
    QQueue<EventImageDownload*> m_active;
};

}

#endif
