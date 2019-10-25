/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_NETWORKREQUESTGENERATOR_P_H
#define NEXTCLOUD_NETWORKREQUESTGENERATOR_P_H

#include <QString>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>

class NetworkRequestGenerator
{
public:
    NetworkRequestGenerator(QNetworkAccessManager *networkAccessManager, const QString &username, const QString &password);
    NetworkRequestGenerator(QNetworkAccessManager *networkAccessManager, const QString &accessToken);

    static bool debugEnabled;

protected:
    QNetworkRequest networkRequest(const QUrl &url, const QString &contentType = QString(), const QByteArray &requestData = QByteArray(), bool basicAuth = false) const;
    QNetworkReply *sendRequest(const QNetworkRequest &request, const QByteArray &requestType, const QByteArray &requestData = QByteArray()) const;

    QString m_username;
    QString m_password;
    QString m_accessToken;
    QNetworkAccessManager *m_networkAccessManager = nullptr;
};

class JsonRequestGenerator : public NetworkRequestGenerator
{
public:
    JsonRequestGenerator(QNetworkAccessManager *networkAccessManager, const QString &username, const QString &password);
    JsonRequestGenerator(QNetworkAccessManager *networkAccessManager, const QString &accessToken);

    QNetworkReply *capabilities(const QString &serverUrl);
    QNetworkReply *notificationList(const QString &serverUrl);
    QNetworkReply *galleryConfig(const QString &serverUrl);
    QNetworkReply *galleryList(const QString &serverUrl, const QString &location = QString());
};

class WebDavRequestGenerator : public NetworkRequestGenerator
{
public:
    WebDavRequestGenerator(QNetworkAccessManager *networkAccessManager, const QString &username, const QString &password);
    WebDavRequestGenerator(QNetworkAccessManager *networkAccessManager, const QString &accessToken);

    QNetworkReply *capabilities(const QString &serverUrl);

    QNetworkReply *dirListing(const QString &serverUrl, const QString &remoteDirPath);
    QNetworkReply *dirCreation(const QString &serverUrl, const QString &remoteDirPath);
    QNetworkReply *upload(const QString &serverUrl, const QString &dataContentType, const QByteArray &data, const QString &remoteDirPath);
    QNetworkReply *download(const QString &serverUrl, const QString &remoteFilePath);

    QNetworkReply *notificationList(const QString &serverUrl);
};

#endif // NEXTCLOUD_NETWORKREQUESTGENERATOR_P_H
