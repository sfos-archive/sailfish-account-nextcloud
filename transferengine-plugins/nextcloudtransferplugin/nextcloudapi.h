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

#ifndef NEXTCLOUDAPI_H
#define NEXTCLOUDAPI_H

#include <QtCore/QObject>
#include <QtCore/QTimer>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

class NextcloudApi : public QObject
{
    Q_OBJECT

public:
    enum API_CALL {
        NONE,
        REQUEST_FOLDER,
        CREATE_FOLDER,
        UPLOAD_FILE,
    };

    NextcloudApi(QNetworkAccessManager *qnam, QObject *parent = 0);
    ~NextcloudApi();

    void propfindPath(const QString &accessToken,
                      const QString &username,
                      const QString &password,
                      const QString &serverAddress,
                      const QString &uploadPath);

    void uploadFile(const QString &filePath,
                    const QString &accessToken,
                    const QString &username,
                    const QString &password,
                    const QString &serverAddress,
                    const QString &uploadPath,
                    bool ignoreSslErrors = false);

    void cancelUpload();

Q_SIGNALS:
    void propfindFinished();
    void transferProgressUpdated(qreal progress);
    void transferFinished();
    void transferError();
    void transferCanceled();
    void credentialsExpired();

private Q_SLOTS:
    void replyError(QNetworkReply::NetworkError error);
    void replyErrorTimerExpired();
    void finished();
    void networkError(QNetworkReply::NetworkError error);
    void uploadProgress(qint64 received, qint64 total);
    void timedOut();

private:
    void finishTransfer(QNetworkReply::NetworkError error, int httpCode, const QByteArray &data);
    void clearPendingErrors();
    QMap<QNetworkReply*, API_CALL> m_replies;
    QMap<QTimer*, QNetworkReply*> m_timeouts;
    QNetworkAccessManager *m_qnam;
    QTimer m_replyErrorTimer;
    QNetworkReply::NetworkError m_error;
};

#endif // NEXTCLOUDAPI_H
