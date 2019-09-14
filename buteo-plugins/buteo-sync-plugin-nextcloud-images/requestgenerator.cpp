/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "requestgenerator_p.h"
#include "syncer_p.h"

#include <LogMacros.h>

#include <QUrl>
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <QStringList>
#include <QBuffer>
#include <QByteArray>

namespace {
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

    QNetworkRequest setRequestData(const QUrl &url,
                                   const QByteArray &requestData,
                                   const QString &depth,
                                   const QString &ifMatch,
                                   const QString &contentType,
                                   const QString &accessToken)
    {
        QNetworkRequest ret(url);
        if (!contentType.isEmpty()) {
            ret.setHeader(QNetworkRequest::ContentTypeHeader,
                          contentType.toUtf8());
        }
        ret.setHeader(QNetworkRequest::ContentLengthHeader,
                      requestData.length());
        if (!depth.isEmpty()) {
            ret.setRawHeader("Depth", depth.toUtf8());
        }
        if (!ifMatch.isEmpty()) {
            ret.setRawHeader("If-Match", ifMatch.toUtf8());
        }
        if (!accessToken.isEmpty()) {
            ret.setRawHeader("Authorization",
                             QString(QLatin1String("Bearer ")
                             + accessToken).toUtf8());
        }
        return ret;
    }
}

RequestGenerator::RequestGenerator(Syncer *parent,
                                   const QString &username,
                                   const QString &password)
    : q(parent)
    , m_username(username)
    , m_password(password)
{
}

RequestGenerator::RequestGenerator(Syncer *parent,
                                   const QString &accessToken)
    : q(parent)
    , m_accessToken(accessToken)
{
}

QNetworkReply *RequestGenerator::generateRequest(const QString &url,
                                                 const QString &path,
                                                 const QString &depth,
                                                 const QString &requestType,
                                                 const QString &request) const
{
    const QByteArray contentType("application/xml; charset=utf-8");
    QByteArray requestData(request.toUtf8());
    QUrl reqUrl(setRequestUrl(url, path, m_username, m_password));
    QNetworkRequest req(setRequestData(reqUrl, requestData, depth, QString(), contentType, m_accessToken));
    QBuffer *requestDataBuffer = new QBuffer(q);
    requestDataBuffer->setData(requestData);
    LOG_DEBUG("generateRequest():"
            << m_accessToken << reqUrl << depth << requestType
            << QString::fromUtf8(requestData));
    return q->m_qnam.sendCustomRequest(req, requestType.toLatin1(), requestDataBuffer);
}

QNetworkReply *RequestGenerator::albumContentMetadata(const QString &serverUrl, const QString &albumPath)
{
    if (Q_UNLIKELY(albumPath.isEmpty())) {
        LOG_WARNING(Q_FUNC_INFO << "album path empty, aborting");
        return 0;
    }

    if (Q_UNLIKELY(serverUrl.isEmpty())) {
        LOG_WARNING(Q_FUNC_INFO << "server url empty, aborting");
        return 0;
    }

    QString requestStr = QStringLiteral(
        "<d:propfind xmlns:d=\"DAV:\">"
          "<d:propname/>"
        "</d:propfind>");

    return generateRequest(serverUrl, albumPath, QLatin1String("1"), QLatin1String("PROPFIND"), requestStr);
}

