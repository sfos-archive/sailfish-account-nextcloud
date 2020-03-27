/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "networkrequestgenerator_p.h"

#include <QBuffer>

bool NetworkRequestGenerator::debugEnabled = false;

const QByteArray NetworkRequestGenerator::XmlContentType = "application/xml; charset=utf-8";
const QByteArray NetworkRequestGenerator::JsonContentType = "application/json";

NetworkRequestGenerator::NetworkRequestGenerator(QNetworkAccessManager *networkAccessManager, const QString &serverUrl, const QString &username, const QString &password)
    : m_username(username)
    , m_password(password)
    , m_serverUrl(serverUrl)
    , m_networkAccessManager(networkAccessManager)
{
    if (serverUrl.isEmpty()) {
        qWarning("Network requests will fail, url is empty!");
    }
}

NetworkRequestGenerator::NetworkRequestGenerator(QNetworkAccessManager *networkAccessManager, const QString &serverUrl, const QString &accessToken)
    : m_accessToken(accessToken)
    , m_serverUrl(serverUrl)
    , m_networkAccessManager(networkAccessManager)
{
}

QNetworkReply *NetworkRequestGenerator::sendRequest(const QNetworkRequest &request, const QByteArray &requestType, const QByteArray &requestData) const
{
    QBuffer *requestDataBuffer = nullptr;
    if (!requestData.isEmpty()) {
        requestDataBuffer = new QBuffer;
        requestDataBuffer->setData(requestData);
    }

    if (debugEnabled) {
        qDebug() << "Sending request:" << requestType << "to:" << request.url().toString()
                 << "data:" << QString::fromUtf8(requestData);
    }

    QNetworkReply *reply = m_networkAccessManager->sendCustomRequest(request, requestType, requestDataBuffer);
    if (requestDataBuffer) {
        QObject::connect(reply, &QNetworkReply::finished,
                         requestDataBuffer, &QBuffer::deleteLater);
    }
    return reply;
}

QNetworkRequest NetworkRequestGenerator::networkRequest(const QUrl &url, const QString &contentType, const QByteArray &requestData, bool basicAuth) const
{
    QNetworkRequest request(url);

    if (url.path().startsWith(QStringLiteral("/ocs/")) || basicAuth) {
        // Nextcloud APIs require Basic Authentication. Qt 5.6 QNetworkAccessManager does not
        // generate the expected request headers for this if user/pass are set in the URL, so
        // set them manually instead.
        QByteArray credentials((m_username + ':' + m_password).toUtf8());
        request.setRawHeader("Authorization", QByteArray("Basic ") + credentials.toBase64());

        // Nextcloud APIs require this to avoid "CSRF check failed" error.
        request.setRawHeader("OCS-APIRequest", "true");

    } else if (!m_username.isEmpty() && !m_password.isEmpty()) {
        QUrl authenticatedUrl = url;
        authenticatedUrl.setUserName(m_username);
        authenticatedUrl.setPassword(m_password);
        request.setUrl(authenticatedUrl);
    }

    if (!contentType.isEmpty()) {
        request.setHeader(QNetworkRequest::ContentTypeHeader, contentType.toUtf8());
    }
    request.setHeader(QNetworkRequest::ContentLengthHeader, requestData.length());

    if (!m_accessToken.isEmpty()) {
        request.setRawHeader("Authorization", QString(QLatin1String("Bearer ") + m_accessToken).toUtf8());
    }

    return request;
}

QUrl NetworkRequestGenerator::networkRequestUrl(const QString &path, const QUrlQuery &query)
{
    QUrl ret(m_serverUrl);
    QString modifiedPath(path);
    if (!path.isEmpty()) {
        // common case: the path may contain %40 instead of @ symbol,
        // if the server returns paths in percent-encoded form.
        // QUrl::setPath() will automatically percent-encode the input,
        // so if we have received percent-encoded path, we need to undo
        // the percent encoding first.  This is suboptimal but works
        // at least for the common case.
        if (path.contains(QStringLiteral("%40"))) {
            modifiedPath = QUrl::fromPercentEncoding(path.toUtf8());
        }

        // override the path from the given url with the path argument.
        // this is because the initial URL may be a user-principals URL
        // but subsequent paths are not relative to that one, but instead
        // are relative to the root path /
        if (path.startsWith('/')) {
            ret.setPath(modifiedPath);
        } else {
            ret.setPath('/' + modifiedPath);
        }
    }
    if (!query.isEmpty()) {
        ret.setQuery(query);
    }
    return ret;
}

QNetworkReply *NetworkRequestGenerator::userInfo(const QByteArray &acceptContentType)
{
    QNetworkRequest request = networkRequest(networkRequestUrl("/ocs/v2.php/cloud/user"));
    if (!acceptContentType.isEmpty()) {
        request.setRawHeader("Accept", acceptContentType);
    }
    return sendRequest(request, "GET");
}

QNetworkReply *NetworkRequestGenerator::capabilities(const QByteArray &acceptContentType)
{
    QNetworkRequest request = networkRequest(networkRequestUrl("/ocs/v2.php/cloud/capabilities"));
    if (!acceptContentType.isEmpty()) {
        request.setRawHeader("Accept", acceptContentType);
    }
    return sendRequest(request, "GET");
}

QNetworkReply *NetworkRequestGenerator::notificationList(const QByteArray &acceptContentType)
{
    QNetworkRequest request = networkRequest(networkRequestUrl("/ocs/v2.php/apps/notifications/api/v2/notifications"));
    if (!acceptContentType.isEmpty()) {
        request.setRawHeader("Accept", acceptContentType);
    }
    return sendRequest(request, "GET");
}

QNetworkReply *NetworkRequestGenerator::deleteNotification(const QString &notificationId)
{
    QNetworkRequest request = networkRequest(
                networkRequestUrl("/ocs/v2.php/apps/notifications/api/v2/notifications/" + notificationId));
    return sendRequest(request, "DELETE");
}

QNetworkReply *NetworkRequestGenerator::deleteAllNotifications()
{
    QNetworkRequest request = networkRequest(
                networkRequestUrl("/ocs/v2.php/apps/notifications/api/v2/notifications"));
    return sendRequest(request, "DELETE");
}

QNetworkReply *NetworkRequestGenerator::dirListing(const QString &remoteDirPath)
{
    if (Q_UNLIKELY(remoteDirPath.isEmpty())) {
        qWarning() << "remotePath path empty, aborting";
        return nullptr;
    }

    QByteArray requestData = "<d:propfind xmlns:d=\"DAV:\">" \
        "<d:prop xmlns:oc=\"http://owncloud.org/ns\">" \
             "<d:getlastmodified />" \
             "<d:getcontenttype />" \
             "<d:resourcetype />" \
             "<oc:fileid />" \
             "<oc:owner-id />" \
             "<oc:size />" \
            "</d:prop>" \
        "</d:propfind>";

    QNetworkRequest request = networkRequest(networkRequestUrl(remoteDirPath), XmlContentType, requestData);
    request.setRawHeader("Depth", "1");
    return sendRequest(request, "PROPFIND", requestData);
}

QNetworkReply *NetworkRequestGenerator::dirCreation(const QString &remoteDirPath)
{
    if (Q_UNLIKELY(remoteDirPath.isEmpty())) {
        qWarning() << "remotePath path empty, aborting";
        return nullptr;
    }

    QNetworkRequest request = networkRequest(networkRequestUrl(remoteDirPath));
    return sendRequest(request, "MKCOL");
}

QNetworkReply *NetworkRequestGenerator::upload(const QString &dataContentType, const QByteArray &data, const QString &remoteDirPath)
{
    if (Q_UNLIKELY(remoteDirPath.isEmpty())) {
        qWarning() << "remotePath path empty, aborting";
        return nullptr;
    }

    if (Q_UNLIKELY(data.isEmpty())) {
        qWarning() << "bytes empty, aborting";
        return nullptr;
    }

    // dataContentType may be empty.
    QNetworkRequest request = networkRequest(networkRequestUrl(remoteDirPath), dataContentType.toUtf8(), data);
    return sendRequest(request, "PUT", data);
}

QNetworkReply *NetworkRequestGenerator::download(const QString &remoteFilePath)
{
    if (Q_UNLIKELY(remoteFilePath.isEmpty())) {
        qWarning() << "remotePath path empty, aborting";
        return nullptr;
    }

    QNetworkRequest request = networkRequest(networkRequestUrl(remoteFilePath));
    request.setRawHeader("Depth", "1");
    return sendRequest(request, "GET");
}
