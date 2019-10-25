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
#include <QUrlQuery>

namespace {
    const QByteArray XmlContentType("application/xml; charset=utf-8");
    const QByteArray JsonContentType("application/json");

    QUrl networkRequestUrl(const QString &url, const QString &path, const QUrlQuery &query = QUrlQuery())
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
        if (!query.isEmpty()) {
            ret.setQuery(query);
        }
        return ret;
    }
}

bool NetworkRequestGenerator::debugEnabled = false;

NetworkRequestGenerator::NetworkRequestGenerator(QNetworkAccessManager *networkAccessManager, const QString &username, const QString &password)
    : m_username(username)
    , m_password(password)
    , m_networkAccessManager(networkAccessManager)
{
}

NetworkRequestGenerator::NetworkRequestGenerator(QNetworkAccessManager *networkAccessManager, const QString &accessToken)
    : m_accessToken(accessToken)
    , m_networkAccessManager(networkAccessManager)
{
}

QNetworkReply *NetworkRequestGenerator::sendRequest(const QNetworkRequest &request, const QByteArray &requestType, const QByteArray &requestData) const
{
    QBuffer *requestDataBuffer = 0;
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

//--- JsonRequestGenerator:

JsonRequestGenerator::JsonRequestGenerator(QNetworkAccessManager *networkAccessManager, const QString &username, const QString &password)
    : NetworkRequestGenerator(networkAccessManager, username, password)
{
}

JsonRequestGenerator::JsonRequestGenerator(QNetworkAccessManager *networkAccessManager, const QString &accessToken)
    : NetworkRequestGenerator(networkAccessManager, accessToken)
{
}

QNetworkReply *JsonRequestGenerator::capabilities(const QString &serverUrl)
{
    if (Q_UNLIKELY(serverUrl.isEmpty())) {
        qWarning() << "server url empty, aborting";
        return 0;
    }

    QNetworkRequest request = networkRequest(networkRequestUrl(serverUrl, "/ocs/v2.php/cloud/capabilities"));
    request.setRawHeader("Accept", JsonContentType);
    return sendRequest(request, "GET");
}

QNetworkReply *JsonRequestGenerator::notificationList(const QString &serverUrl)
{
    if (Q_UNLIKELY(serverUrl.isEmpty())) {
        qWarning() << "server url empty, aborting";
        return 0;
    }

    QNetworkRequest request = networkRequest(networkRequestUrl(serverUrl, "/ocs/v2.php/apps/notifications/api/v2/notifications"));
    request.setRawHeader("Accept", JsonContentType);
    return sendRequest(request, "GET");
}

QNetworkReply *JsonRequestGenerator::galleryConfig(const QString &serverUrl)
{
    if (Q_UNLIKELY(serverUrl.isEmpty())) {
        qWarning() << "server url empty, aborting";
        return 0;
    }
    QNetworkRequest request = networkRequest(networkRequestUrl(serverUrl, "/index.php/apps/gallery/api/config"), QString(), QByteArray(), true);
    request.setRawHeader("Accept", JsonContentType);
    return sendRequest(request, "GET");
}

QNetworkReply *JsonRequestGenerator::galleryList(const QString &serverUrl, const QString &location)
{
    if (Q_UNLIKELY(serverUrl.isEmpty())) {
        qWarning() << "server url empty, aborting";
        return 0;
    }

    const QUrlQuery query(QStringLiteral("location=%1&mediatypes=%2&features=%3&etag=%4")
                                    .arg(location.isEmpty() ? QStringLiteral("Photos") : location)
                                    .arg(QStringLiteral("image/jpeg;image/gif;image/png;image/bmp"))
                                    .arg(QString())
                                    .arg(QString()));
    QNetworkRequest request = networkRequest(networkRequestUrl(serverUrl, "/index.php/apps/gallery/api/files/list", query), QString(), QByteArray(), true);
    request.setRawHeader("Accept", JsonContentType);
    return sendRequest(request, "GET");
}

//--- WebDavRequestGenerator:

WebDavRequestGenerator::WebDavRequestGenerator(QNetworkAccessManager *networkAccessManager, const QString &username, const QString &password)
    : NetworkRequestGenerator(networkAccessManager, username, password)
{
}

WebDavRequestGenerator::WebDavRequestGenerator(QNetworkAccessManager *networkAccessManager, const QString &accessToken)
    : NetworkRequestGenerator(networkAccessManager, accessToken)
{
}

QNetworkReply *WebDavRequestGenerator::dirListing(const QString &serverUrl, const QString &remoteDirPath)
{
    if (Q_UNLIKELY(serverUrl.isEmpty())) {
        qWarning() << "server url empty, aborting";
        return 0;
    }

    if (Q_UNLIKELY(remoteDirPath.isEmpty())) {
        qWarning() << "remotePath path empty, aborting";
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
        qWarning() << "server url empty, aborting";
        return 0;
    }

    if (Q_UNLIKELY(remoteDirPath.isEmpty())) {
        qWarning() << "remotePath path empty, aborting";
        return 0;
    }

    QNetworkRequest request = networkRequest(networkRequestUrl(serverUrl, remoteDirPath));
    return sendRequest(request, "MKCOL");
}

QNetworkReply *WebDavRequestGenerator::upload(const QString &serverUrl, const QString &dataContentType, const QByteArray &data, const QString &remoteDirPath)
{
    if (Q_UNLIKELY(serverUrl.isEmpty())) {
        qWarning() << "server url empty, aborting";
        return 0;
    }

    if (Q_UNLIKELY(remoteDirPath.isEmpty())) {
        qWarning() << "remotePath path empty, aborting";
        return 0;
    }

    if (Q_UNLIKELY(data.isEmpty())) {
        qWarning() << "bytes empty, aborting";
        return 0;
    }

    // dataContentType may be empty.
    QNetworkRequest request = networkRequest(networkRequestUrl(serverUrl, remoteDirPath), dataContentType.toUtf8(), data);
    return sendRequest(request, "PUT", data);
}

QNetworkReply *WebDavRequestGenerator::download(const QString &serverUrl, const QString &remoteFilePath)
{
    if (Q_UNLIKELY(serverUrl.isEmpty())) {
        qWarning() << "server url empty, aborting";
        return 0;
    }

    if (Q_UNLIKELY(remoteFilePath.isEmpty())) {
        qWarning() << "remotePath path empty, aborting";
        return 0;
    }

    QNetworkRequest request = networkRequest(networkRequestUrl(serverUrl, remoteFilePath));
    request.setRawHeader("Depth", "1");
    return sendRequest(request, "GET");
}
