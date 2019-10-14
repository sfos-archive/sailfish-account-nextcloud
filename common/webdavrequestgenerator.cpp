/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "webdavrequestgenerator_p.h"

#include <LogMacros.h>

#include <QUrl>
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <QStringList>
#include <QBuffer>
#include <QByteArray>

namespace {
    const QByteArray XmlContentType("application/xml; charset=utf-8");
    const QByteArray JsonContentType("application/json");

    QUrl networkRequestUrl(const QString &url, const QString &path)
    {
        QUrl ret(url);
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
        return ret;
    }
}

WebDavRequestGenerator::WebDavRequestGenerator(QNetworkAccessManager *networkAccessManager, const QString &username, const QString &password)
    : m_username(username)
    , m_password(password)
    , m_networkAccessManager(networkAccessManager)
{
}

WebDavRequestGenerator::WebDavRequestGenerator(QNetworkAccessManager *networkAccessManager, const QString &accessToken)
    : m_accessToken(accessToken)
    , m_networkAccessManager(networkAccessManager)
{
}

QNetworkReply *WebDavRequestGenerator::sendRequest(const QNetworkRequest &request, const QByteArray &requestType, const QByteArray &requestData) const
{
    QBuffer *requestDataBuffer = 0;
    if (!requestData.isEmpty()) {
        requestDataBuffer = new QBuffer;
        requestDataBuffer->setData(requestData);
    }

    LOG_DEBUG("Sending request:"
              << request.url().toString() << requestType
              << "token:" << m_accessToken
              << "data:" << QString::fromUtf8(requestData));
    QNetworkReply *reply = m_networkAccessManager->sendCustomRequest(request, requestType, requestDataBuffer);

    if (requestDataBuffer) {
        QObject::connect(reply, &QNetworkReply::finished,
                         requestDataBuffer, &QBuffer::deleteLater);
    }
    return reply;
}

QNetworkRequest WebDavRequestGenerator::networkRequest(const QUrl &url, const QString &contentType, const QByteArray &requestData) const
{
    QNetworkRequest request(url);

    if (url.path().startsWith(QStringLiteral("/ocs/"))) {
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

QNetworkReply *WebDavRequestGenerator::capabilities(const QString &serverUrl)
{
    if (Q_UNLIKELY(serverUrl.isEmpty())) {
        LOG_WARNING(Q_FUNC_INFO << "server url empty, aborting");
        return 0;
    }

    QNetworkRequest request = networkRequest(networkRequestUrl(serverUrl, "/ocs/v2.php/cloud/capabilities"));
    request.setRawHeader("Accept", JsonContentType);
    return sendRequest(request, "GET");
}

QNetworkReply *WebDavRequestGenerator::dirListing(const QString &serverUrl, const QString &remoteDirPath)
{
    if (Q_UNLIKELY(serverUrl.isEmpty())) {
        LOG_WARNING(Q_FUNC_INFO << "server url empty, aborting");
        return 0;
    }

    if (Q_UNLIKELY(remoteDirPath.isEmpty())) {
        LOG_WARNING(Q_FUNC_INFO << "remotePath path empty, aborting");
        return 0;
    }

    QByteArray requestData = "<d:propfind xmlns:d=\"DAV:\">" \
          "<d:propname/>" \
        "</d:propfind>";

    QNetworkRequest request = networkRequest(networkRequestUrl(serverUrl, remoteDirPath), XmlContentType, requestData);
    request.setRawHeader("Depth", "1");
    return sendRequest(request, "PROPFIND", requestData);
}

QNetworkReply *WebDavRequestGenerator::dirCreation(const QString &serverUrl, const QString &remoteDirPath)
{
    if (Q_UNLIKELY(serverUrl.isEmpty())) {
        LOG_WARNING(Q_FUNC_INFO << "server url empty, aborting");
        return 0;
    }

    if (Q_UNLIKELY(remoteDirPath.isEmpty())) {
        LOG_WARNING(Q_FUNC_INFO << "remotePath path empty, aborting");
        return 0;
    }

    QNetworkRequest request = networkRequest(networkRequestUrl(serverUrl, remoteDirPath));
    return sendRequest(request, "MKCOL");
}

QNetworkReply *WebDavRequestGenerator::upload(const QString &serverUrl, const QString &dataContentType, const QByteArray &data, const QString &remoteDirPath)
{
    if (Q_UNLIKELY(serverUrl.isEmpty())) {
        LOG_WARNING(Q_FUNC_INFO << "server url empty, aborting");
        return 0;
    }

    if (Q_UNLIKELY(remoteDirPath.isEmpty())) {
        LOG_WARNING(Q_FUNC_INFO << "remotePath path empty, aborting");
        return 0;
    }

    if (Q_UNLIKELY(data.isEmpty())) {
        LOG_WARNING(Q_FUNC_INFO << "bytes empty, aborting");
        return 0;
    }

    // dataContentType may be empty.
    QNetworkRequest request = networkRequest(networkRequestUrl(serverUrl, remoteDirPath), dataContentType.toUtf8(), data);
    return sendRequest(request, "PUT", data);
}

QNetworkReply *WebDavRequestGenerator::download(const QString &serverUrl, const QString &remoteFilePath)
{
    if (Q_UNLIKELY(serverUrl.isEmpty())) {
        LOG_WARNING(Q_FUNC_INFO << "server url empty, aborting");
        return 0;
    }

    if (Q_UNLIKELY(remoteFilePath.isEmpty())) {
        LOG_WARNING(Q_FUNC_INFO << "remotePath path empty, aborting");
        return 0;
    }

    QNetworkRequest request = networkRequest(networkRequestUrl(serverUrl, remoteFilePath));
    request.setRawHeader("Depth", "1");
    return sendRequest(request, "GET");
}

QNetworkReply *WebDavRequestGenerator::notificationList(const QString &serverUrl)
{
    if (Q_UNLIKELY(serverUrl.isEmpty())) {
        LOG_WARNING(Q_FUNC_INFO << "server url empty, aborting");
        return 0;
    }

    QNetworkRequest request = networkRequest(networkRequestUrl(serverUrl, "/ocs/v2.php/apps/notifications/api/v2/notifications"));
    request.setRawHeader("Accept", JsonContentType);
    return sendRequest(request, "GET");
}
