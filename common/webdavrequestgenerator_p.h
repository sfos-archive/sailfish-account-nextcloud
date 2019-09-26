/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_WEBDAVREQUESTGENERATOR_P_H
#define NEXTCLOUD_WEBDAVREQUESTGENERATOR_P_H

#include <QString>
#include <QNetworkReply>
#include <QNetworkAccessManager>

class Syncer;
class WebDavRequestGenerator
{
public:
    WebDavRequestGenerator(QNetworkAccessManager *networkAccessManager, const QString &username, const QString &password);
    WebDavRequestGenerator(QNetworkAccessManager *networkAccessManager, const QString &accessToken);

    QNetworkReply *dirListing(const QString &serverUrl, const QString &remoteDirPath);
    QNetworkReply *dirCreation(const QString &serverUrl, const QString &remoteDirPath);
    QNetworkReply *upload(const QString &serverUrl, const QString &dataContentType, const QByteArray &data, const QString &remoteDirPath);
    QNetworkReply *download(const QString &serverUrl, const QString &remoteFilePath);

private:
    QUrl networkRequestUrl(const QString &url, const QString &path) const;
    QNetworkRequest networkRequest(const QUrl &url, const QString &contentType = QString(), const QByteArray &requestData = QByteArray()) const;
    QNetworkReply *sendRequest(const QNetworkRequest &request, const QByteArray &requestType, const QByteArray &requestData = QByteArray()) const;

    QString m_username;
    QString m_password;
    QString m_accessToken;
    QNetworkAccessManager *m_networkAccessManager = nullptr;
};

#endif // NEXTCLOUD_WEBDAVREQUESTGENERATOR_P_H
