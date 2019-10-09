/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "syncer_p.h"

#include "accountauthenticator_p.h"
#include "webdavrequestgenerator_p.h"
#include "xmlreplyparser_p.h"

#include "accountauthenticator_p.h"
#include "webdavrequestgenerator_p.h"

#include "eventcache.h"

#include <QtCore/QDateTime>
#include <QtCore/QUrl>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QByteArray>
#include <QtCore/QStandardPaths>

// accounts
#include <Accounts/Manager>
#include <Accounts/Account>

// buteo
#include <SyncProfile.h>
#include <LogMacros.h>
#include <ProfileEngineDefs.h>

namespace {
    const int HTTP_UNAUTHORIZED_ACCESS = 401;
    const QString NEXTCLOUD_USERID = QStringLiteral("nextcloud");
}

Syncer::Syncer(QObject *parent, Buteo::SyncProfile *syncProfile)
    : WebDavSyncer(parent, syncProfile, QStringLiteral("nextcloud-posts"))
{
}

Syncer::~Syncer()
{
}

void Syncer::beginSync()
{
    if (!performCapabilitiesRequest()) {
        finishWithError("Capabilities request failed");
        return;
    }
}

bool Syncer::performCapabilitiesRequest()
{
    qWarning("performCapabilitiesRequest");
    QNetworkReply *reply = m_requestGenerator->capabilities(m_serverUrl);
    if (reply) {
        connect(reply, &QNetworkReply::finished,
                this, &Syncer::handleCapabilitiesReply);
        return true;
    }

    return false;
}

void Syncer::handleCapabilitiesReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    reply->deleteLater();
    const QByteArray replyData = reply->readAll();
    const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() != QNetworkReply::NoError) {
        finishWithHttpError("Capabilities request failed", httpCode);
        return;
    }

    if (XmlReplyParser::findCapabilityfromJson("notifications", replyData).isEmpty()) {
        finishWithError("Server does not support Notifications app!");
        return;
    }

    if (!performNotificationListRequest()) {
        finishWithError("Notification list request failed");
        return;
    }
}

bool Syncer::performNotificationListRequest()
{
    QNetworkReply *reply = m_requestGenerator->notificationList(m_serverUrl);
    if (reply) {
        connect(reply, &QNetworkReply::finished,
                this, &Syncer::handleNotificationListReply);
        return true;
    }

    return false;
}

void Syncer::handleNotificationListReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    reply->deleteLater();
    const QByteArray replyData = reply->readAll();
    const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() != QNetworkReply::NoError) {
        finishWithHttpError("Notifications request failed: " + reply->errorString(), httpCode);
        return;
    }

    SyncCache::EventDatabase db;
    SyncCache::DatabaseError error;
    db.openDatabase(
            QStringLiteral("%1/system/privileged/Posts/nextcloud.db").arg(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)),
            &error);

    if (error.errorCode != SyncCache::DatabaseError::NoError) {
        emit finishWithError(QStringLiteral("Failed to open Posts database: %1: %2").arg(error.errorCode).arg(error.errorMessage));
        return;
    }

    if (!db.beginTransaction(&error)) {
        emit finishWithError(QStringLiteral("Failed to begin Posts transaction: %1: %2").arg(error.errorCode).arg(error.errorMessage));
        return;
    }

    bool storeSucceeded = true;
    QList<XmlReplyParser::Notification> notifs = XmlReplyParser::parseNotificationsFromJson(replyData);
    for (const XmlReplyParser::Notification &notif : notifs) {
        SyncCache::Event event;
        event.accountId = m_accountId;
        event.eventId = notif.notificationId;
        event.eventText = notif.subject;
        event.eventUrl = notif.link;
        event.imageUrl = notif.icon;
        event.timestamp = notif.dateTime;

        db.storeEvent(event, &error);
        if (error.errorCode != SyncCache::DatabaseError::NoError) {
            storeSucceeded = false;
            break;
            LOG_WARNING("failed to store post addition:" << error.errorCode << error.errorMessage);
            break;
        }
    }

    bool commitSucceeded = storeSucceeded && db.commitTransaction(&error);
    if (!commitSucceeded) {
        SyncCache::DatabaseError rollbackError;
        db.rollbackTransaction(&rollbackError);
    }

    if (commitSucceeded) {
        emit finishWithSuccess();
    } else if (storeSucceeded) {
        emit finishWithError(QStringLiteral("Failed to commit Posts transaction: %1: %2").arg(error.errorCode).arg(error.errorMessage));
    } else {
        emit finishWithError(QStringLiteral("Failed to store Posts: %1: %2").arg(error.errorCode).arg(error.errorMessage));
    }
}

void Syncer::purgeAccount(int accountId)
{
    Q_UNUSED(accountId);

    SyncCache::EventDatabase db;
    SyncCache::DatabaseError error;
    db.openDatabase(
            QStringLiteral("%1/system/privileged/Posts/nextcloud.db").arg(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)),
            &error);

    if (error.errorCode != SyncCache::DatabaseError::NoError) {
        emit finishWithError(QStringLiteral("Failed to open Posts database: %1: %2").arg(error.errorCode).arg(error.errorMessage));
        return;
    }

    if (!db.beginTransaction(&error)) {
        emit finishWithError(QStringLiteral("Failed to begin Posts transaction: %1: %2").arg(error.errorCode).arg(error.errorMessage));
        return;
    }

    bool deleteSucceeded = true;
    const QVector<SyncCache::Event> events = db.events(accountId, &error);
    if (error.errorCode != SyncCache::DatabaseError::NoError) {
        LOG_WARNING("Failed to query Posts for purge:" << error.errorCode << error.errorMessage);
        db.rollbackTransaction(&error); // TODO: log the fact that we need to purge this one later.
        return;
    }

    Q_FOREACH (const SyncCache::Event &event, events) {
        db.deleteEvent(event, &error);
        if (error.errorCode != SyncCache::DatabaseError::NoError) {
            deleteSucceeded = false;
            break;
        }
    }

    const bool commitSucceeded = deleteSucceeded && db.commitTransaction(&error);
    if (!commitSucceeded) {
        SyncCache::DatabaseError rollbackError;
        db.rollbackTransaction(&rollbackError); // TODO: log the fact that we need to purge this one later.
    }

    if (commitSucceeded) {
        LOG_DEBUG("Successfully purged posts for deleted account" << accountId);
    } else if (deleteSucceeded) {
        LOG_WARNING("Failed to commit purge Posts transaction:" << error.errorCode << error.errorMessage);
    } else {
        LOG_WARNING("Failed to purge Posts:" << error.errorCode << error.errorMessage);
    }
}
