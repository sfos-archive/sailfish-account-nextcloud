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

#ifndef NEXTCLOUD_NETWORKREQUESTGENERATOR_P_H
#define NEXTCLOUD_NETWORKREQUESTGENERATOR_P_H

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QUrlQuery>

class NetworkRequestGenerator
{
public:
    NetworkRequestGenerator(QNetworkAccessManager *networkAccessManager, const QString &serverUrl, const QString &username, const QString &password);
    NetworkRequestGenerator(QNetworkAccessManager *networkAccessManager, const QString &serverUrl, const QString &accessToken);

    QNetworkReply *userInfo(const QByteArray &acceptContentType);
    QNetworkReply *capabilities(const QByteArray &acceptContentType);

    QNetworkReply *notificationList(const QByteArray &acceptContentType);
    QNetworkReply *deleteNotification(const QString &notificationId);
    QNetworkReply *deleteAllNotifications();

    QNetworkReply *dirListing(const QString &remoteDirPath);
    QNetworkReply *dirCreation(const QString &remoteDirPath);
    QNetworkReply *upload(const QString &dataContentType, const QByteArray &data, const QString &remoteDirPath);
    QNetworkReply *download(const QString &remoteFilePath);

    static bool debugEnabled;

    static const QByteArray XmlContentType;
    static const QByteArray JsonContentType;

private:
    QNetworkRequest networkRequest(const QString &path,
                                   const QString &contentType = QString(),
                                   const QByteArray &requestData = QByteArray()) const;
    QUrl networkRequestUrl(const QString &path);
    QNetworkReply *sendRequest(const QNetworkRequest &request, const QByteArray &requestType, const QByteArray &requestData = QByteArray()) const;

    QString m_username;
    QString m_password;
    QString m_accessToken;
    QUrl m_serverUrl;
    QNetworkAccessManager *m_networkAccessManager = nullptr;
};

#endif // NEXTCLOUD_NETWORKREQUESTGENERATOR_P_H
