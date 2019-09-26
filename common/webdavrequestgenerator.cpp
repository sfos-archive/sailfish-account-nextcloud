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

    QUrl setRequestUrl(const QString &url, const QString &path, const QString &username, const QString &password)
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
        if (!username.isEmpty() && !password.isEmpty()) {
            ret.setUserName(username);
            ret.setPassword(password);
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

    LOG_DEBUG("Sending request:" << request.url().toString() << requestType
                << "token:" << m_accessToken
                << "data:" << QString::fromUtf8(requestData));
    QNetworkReply *reply = m_networkAccessManager->sendCustomRequest(request, requestType, requestDataBuffer);

    if (requestDataBuffer) {
        QObject::connect(reply, &QNetworkReply::finished,
                         requestDataBuffer, &QBuffer::deleteLater);
    }
    return reply;
}

QUrl WebDavRequestGenerator::networkRequestUrl(const QString &url, const QString &path) const
{
    return setRequestUrl(url, path, m_username, m_password);
}

QNetworkRequest WebDavRequestGenerator::networkRequest(const QUrl &url, const QString &contentType, const QByteArray &requestData) const
{
    QNetworkRequest request(url);
    if (!contentType.isEmpty()) {
        request.setHeader(QNetworkRequest::ContentTypeHeader, contentType.toUtf8());
    }
    request.setHeader(QNetworkRequest::ContentLengthHeader, requestData.length());
    if (!m_accessToken.isEmpty()) {
        request.setRawHeader("Authorization", QString(QLatin1String("Bearer ") + m_accessToken).toUtf8());
    }
    return request;
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
