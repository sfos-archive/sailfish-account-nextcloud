/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "nextcloudapi.h"
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QFile>
#include <QFileInfo>
#include <QtDebug>
#include <QStringList>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

NextcloudApi::NextcloudApi(QObject *parent)
    : QObject(parent)
    , m_fileSize(0)
    , m_transferred(0)
{
}

NextcloudApi::~NextcloudApi()
{
}

void NextcloudApi::uploadFile(const QString &filePath,
                              const QVariantMap &nextcloudParameters)
{
    QString accessToken = nextcloudParameters.value(QStringLiteral("AccessToken")).toString();
    if (accessToken.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "no access token";
        emit transferError();
        return;
    }

    QString contentApiBackendUrl = nextcloudParameters.value(QStringLiteral("contentApiUrl")).toString();
    if (contentApiBackendUrl.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "no content backend url";
        emit transferError();
        return;
    }

    QFile f(filePath, this);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << Q_FUNC_INFO << "error opening file: " << filePath;
        emit transferError();
        return;
    }
    m_fileSize = f.size();

    QByteArray imageData(f.readAll());
    f.close();

    QString fileName = filePath.split("/").last();

    QNetworkRequest request;
    QString urlStr = QStringLiteral("%1/me/skydrive/camera_roll/files/%2?access_token=%3").arg(contentApiBackendUrl).arg(fileName).arg(accessToken);
    QUrl url(urlStr);
    request.setUrl(url);

    QNetworkReply *reply = networkManager()->put(request, imageData);

    if (!reply) {
        qWarning() << Q_FUNC_INFO << "Unable to post upload file request";
        emit transferError();
        return;
    }

    QTimer *timer = new QTimer(this);
    timer->setInterval(60000);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(timedOut()));
    timer->start();
    m_timeouts.insert(timer, reply);
    reply->setProperty("timeoutTimer", QVariant::fromValue<QTimer*>(timer));

    m_replies.insert(reply, UPLOAD_FILE);
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(replyError(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(uploadProgress(qint64,qint64)));
}

void NextcloudApi::cancelUpload()
{
    if (m_replies.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "can't cancel upload.";
        return;
    }

    QList<QNetworkReply*> replies = m_replies.keys();
    foreach (QNetworkReply *reply, replies) {
        reply->abort();
    }
    m_replies.clear();
}

void NextcloudApi::finished(QNetworkReply *reply)
{
    QByteArray data = reply->readAll();
    QString replyStr(data);
    int apiCall = m_replies.take(reply);

    clearPendingErrors();
    reply->deleteLater();

    QTimer *timer = reply->property("timeoutTimer").value<QTimer*>();
    if (timer) {
        m_timeouts.remove(timer);
        timer->stop();
        timer->deleteLater();
    }

    QJsonDocument json = QJsonDocument::fromJson(replyStr.toLatin1());
    if (json.isObject()) {
        QJsonObject obj = json.object();
        if (obj.contains(QStringLiteral("error"))) {
            qWarning() << Q_FUNC_INFO << "Nextcloud API error: " << obj.value(QStringLiteral("error")) << " ";
            emit transferError();
            return;
        }
    }

    switch (apiCall) {
    case UPLOAD_FILE:
        // Uploaded, we're done.
        break;
    default:
        qWarning() << Q_FUNC_INFO << " unknown Nextcloud API call: " << apiCall;
        emit transferError();
        return;
    }

    finishTransfer(reply->error());
}

void NextcloudApi::networkError(QNetworkReply::NetworkError error)
{
    int httpStatus = qobject_cast<QNetworkReply*>(sender())->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qWarning() << Q_FUNC_INFO << " network error: " << error << "  http status code: " << httpStatus;
    emit transferError();
}

void NextcloudApi::uploadProgress(qint64 received, qint64 total)
{
    if (m_fileSize > 0 && total > 0) {
        qreal progress = received / (qreal)total;
        qint64 totalreceived = m_transferred + progress * (qreal)total;
        emit transferProgressUpdated(totalreceived / (qreal)m_fileSize);
        QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
        if (reply) {
            QTimer *timer = reply->property("timeoutTimer").value<QTimer*>();
            if (timer) {
                // restart the 60 second timer when there is progress
                timer->start();
            }
        }
    }
}

void NextcloudApi::timedOut()
{
    QTimer *timer = qobject_cast<QTimer*>(sender());
    if (timer) {
        QNetworkReply *reply = m_timeouts.take(timer);
        if (reply) {
            reply->deleteLater();
            timer->deleteLater();
            m_replies.take(reply);
            qWarning() << Q_FUNC_INFO << "File transfer request timed out";
            emit transferError();
        }
    }
}
