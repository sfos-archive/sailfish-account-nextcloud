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

#include "synccacheevents.h"
#include "synccacheevents_p.h"
#include "synccacheeventdownloads_p.h"

#include <QtCore/QFile>
#include <QtCore/QStandardPaths>

using namespace SyncCache;

//-----------------------------------------------------------------------------

EventCacheThreadWorker::EventCacheThreadWorker(QObject *parent)
    : QObject(parent), m_downloader(nullptr)
{
}

EventCacheThreadWorker::~EventCacheThreadWorker()
{
}

void EventCacheThreadWorker::openDatabase(const QString &accountType)
{
    DatabaseError error;
    m_db.openDatabase(
            QStringLiteral("%1/%2.db").arg(EventCache::eventCacheRootDir(), accountType),
            &error);
    if (error.errorCode != DatabaseError::NoError) {
        emit openDatabaseFailed(error.errorMessage);
    } else {
        connect(&m_db, &EventDatabase::eventsStored,
                this, &EventCacheThreadWorker::eventsStored);
        connect(&m_db, &EventDatabase::eventsDeleted,
                this, &EventCacheThreadWorker::eventsDeleted);
        emit openDatabaseFinished();
    }
}

void EventCacheThreadWorker::requestEvents(int accountId, bool includeLocallyDeleted)
{
    DatabaseError error;
    QVector<SyncCache::Event> events = m_db.events(accountId, &error, includeLocallyDeleted);
    if (error.errorCode != DatabaseError::NoError) {
        emit requestEventsFailed(accountId, error.errorMessage);
    } else {
        emit requestEventsFinished(accountId, events);
    }
}

void EventCacheThreadWorker::deleteEvent(int accountId, const QString &eventId)
{
    DatabaseError error;
    m_db.deleteEvent(accountId, eventId, &error);
    if (error.errorCode != DatabaseError::NoError) {
        emit deleteEventFailed(accountId, eventId, error.errorMessage);
    } else {
        emit deleteEventFinished(accountId, eventId);
    }
}

void EventCacheThreadWorker::flagEventForDeletion(int accountId, const QString &eventId)
{
    DatabaseError error;
    m_db.flagEventForDeletion(accountId, eventId, &error);
    if (error.errorCode != DatabaseError::NoError) {
        emit flagEventForDeletionFailed(accountId, eventId, error.errorMessage);
    } else {
        emit flagEventForDeletionFinished(accountId, eventId);
    }
}

void EventCacheThreadWorker::populateEventImage(int idempToken, int accountId, const QString &eventId, const QNetworkRequest &requestTemplate)
{
    DatabaseError error;
    Event event = m_db.event(accountId, eventId, &error);
    if (error.errorCode != DatabaseError::NoError) {
        emit populateEventImageFailed(idempToken, QStringLiteral("Error occurred while reading event info from db: %1").arg(error.errorMessage));
        return;
    }

    // the image already exists.
    const QString imagePath = event.imagePath.toString();
    if (!event.imagePath.isEmpty() && QFile::exists(imagePath)) {
        emit populateEventImageFinished(idempToken, imagePath);
        return;
    }

    // download the image.
    if (event.eventId.isEmpty()) {
        emit populateEventImageFailed(idempToken, QStringLiteral("Event id is missing"));
        return;
    }

    // download the image.
    if (event.imageUrl.isEmpty()) {
        emit populateEventImageFailed(idempToken, QStringLiteral("Empty image url specified for event"));
        return;
    }
    if (!m_downloader) {
        m_downloader = new EventImageDownloader(this);
    }
    EventImageDownloadWatcher *watcher = m_downloader->downloadImage(
                idempToken,
                event.imageUrl,
                QStringLiteral("%1/event-%2-icon").arg(EventCache::eventCacheDir(accountId)).arg(event.eventId),
                requestTemplate);
    connect(watcher, &EventImageDownloadWatcher::downloadFailed, this, [this, watcher, idempToken] (const QString &errorMessage) {
        emit populateEventImageFailed(idempToken, errorMessage);
        watcher->deleteLater();
    });
    connect(watcher, &EventImageDownloadWatcher::downloadFinished, this, [this, watcher, event, idempToken] (const QUrl &filePath) {
        // the file has been downloaded to disk.  attempt to update the database.
        DatabaseError storeError;
        Event eventToStore;
        eventToStore.accountId = event.accountId;
        eventToStore.eventId = event.eventId;
        eventToStore.eventSubject = event.eventSubject;
        eventToStore.eventText = event.eventText;
        eventToStore.eventUrl = event.eventUrl;
        eventToStore.imageUrl = event.imageUrl;
        eventToStore.imagePath = filePath;
        eventToStore.timestamp = event.timestamp;
        m_db.storeEvent(eventToStore, &storeError);
        if (storeError.errorCode != DatabaseError::NoError) {
            QFile::remove(filePath.toString());
            emit populateEventImageFailed(idempToken, storeError.errorMessage);
        } else {
            emit populateEventImageFinished(idempToken, filePath.toString());
        }
        watcher->deleteLater();
    });
}

//-----------------------------------------------------------------------------

EventCachePrivate::EventCachePrivate(EventCache *parent)
    : QObject(parent), m_worker(new EventCacheThreadWorker)
{
    qRegisterMetaType<SyncCache::Event>();
    qRegisterMetaType<QVector<SyncCache::Event> >();

    m_worker->moveToThread(&m_dbThread);
    connect(&m_dbThread, &QThread::finished, m_worker, &QObject::deleteLater);

    connect(this, &EventCachePrivate::openDatabase, m_worker, &EventCacheThreadWorker::openDatabase);
    connect(this, &EventCachePrivate::requestEvents, m_worker, &EventCacheThreadWorker::requestEvents);
    connect(this, &EventCachePrivate::deleteEvent, m_worker, &EventCacheThreadWorker::deleteEvent);
    connect(this, &EventCachePrivate::flagEventForDeletion, m_worker, &EventCacheThreadWorker::flagEventForDeletion);
    connect(this, &EventCachePrivate::populateEventImage, m_worker, &EventCacheThreadWorker::populateEventImage);

    connect(m_worker, &EventCacheThreadWorker::openDatabaseFailed, parent, &EventCache::openDatabaseFailed);
    connect(m_worker, &EventCacheThreadWorker::openDatabaseFinished, parent, &EventCache::openDatabaseFinished);
    connect(m_worker, &EventCacheThreadWorker::requestEventsFailed, parent, &EventCache::requestEventsFailed);
    connect(m_worker, &EventCacheThreadWorker::requestEventsFinished, parent, &EventCache::requestEventsFinished);
    connect(m_worker, &EventCacheThreadWorker::deleteEventFailed, parent, &EventCache::deleteEventFailed);
    connect(m_worker, &EventCacheThreadWorker::deleteEventFinished, parent, &EventCache::deleteEventFinished);
    connect(m_worker, &EventCacheThreadWorker::flagEventForDeletionFailed, parent, &EventCache::flagEventForDeletionFailed);
    connect(m_worker, &EventCacheThreadWorker::flagEventForDeletionFinished, parent, &EventCache::flagEventForDeletionFinished);

    connect(m_worker, &EventCacheThreadWorker::populateEventImageFailed, parent, &EventCache::populateEventImageFailed);
    connect(m_worker, &EventCacheThreadWorker::populateEventImageFinished, parent, &EventCache::populateEventImageFinished);

    connect(m_worker, &EventCacheThreadWorker::eventsStored, parent, &EventCache::eventsStored);
    connect(m_worker, &EventCacheThreadWorker::eventsDeleted, parent, &EventCache::eventsDeleted);

    m_dbThread.start();
    m_dbThread.setPriority(QThread::IdlePriority);
}

EventCachePrivate::~EventCachePrivate()
{
    m_dbThread.quit();
    m_dbThread.wait();
}

//-----------------------------------------------------------------------------

EventCache::EventCache(QObject *parent)
    : QObject(parent), d_ptr(new EventCachePrivate(this))
{
}

EventCache::~EventCache()
{
}

QString EventCache::eventCacheDir(int accountId)
{
    return QStringLiteral("%1/nextcloud/account-%2")
            .arg(EventCache::eventCacheRootDir()).arg(accountId);
}

QString EventCache::eventCacheRootDir()
{
    return QStringLiteral("%1/system/privileged/Posts").arg(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
}

void EventCache::openDatabase(const QString &databaseFile)
{
    Q_D(EventCache);
    emit d->openDatabase(databaseFile);
}

void EventCache::requestEvents(int accountId, bool includeLocallyDeleted)
{
    Q_D(EventCache);
    emit d->requestEvents(accountId, includeLocallyDeleted);
}

void EventCache::deleteEvent(int accountId, const QString &eventId)
{
    Q_D(EventCache);
    emit d->deleteEvent(accountId, eventId);
}

void EventCache::flagEventForDeletion(int accountId, const QString &eventId)
{
    Q_D(EventCache);
    emit d->flagEventForDeletion(accountId, eventId);
}

void EventCache::populateEventImage(int idempToken, int accountId, const QString &eventId, const QNetworkRequest &requestTemplate)
{
    Q_D(EventCache);
    emit d->populateEventImage(idempToken, accountId, eventId, requestTemplate);
}
