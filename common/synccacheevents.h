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

#ifndef NEXTCLOUD_SYNCCACHEEVENTS_H
#define NEXTCLOUD_SYNCCACHEEVENTS_H

#include "synccachedatabase.h"

#include <QtCore/QMetaType>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QDateTime>
#include <QtCore/QUrl>
#include <QtCore/QVector>
#include <QtCore/QScopedPointer>
#include <QtNetwork/QNetworkRequest>

namespace SyncCache {

class ProcessMutex;

struct Event {
    int accountId = 0;
    QString eventId;
    QString eventSubject;
    QString eventText;
    QUrl eventUrl;
    QUrl imageUrl;
    QUrl imagePath;
    bool deletedLocally = false;
    QDateTime timestamp;
};

class EventDatabase : public Database
{
    Q_OBJECT

public:
    EventDatabase(QObject *parent = nullptr);

    QVector<SyncCache::Event> events(int accountId, SyncCache::DatabaseError *error, bool includeLocallyDeleted = true) const;
    SyncCache::Event event(int accountId, const QString &eventId, SyncCache::DatabaseError *error) const;
    void storeEvent(const SyncCache::Event &event, SyncCache::DatabaseError *error);
    void deleteEvent(int accountId, const QString &eventId, SyncCache::DatabaseError *error);
    void flagEventForDeletion(int accountId, const QString &eventId, SyncCache::DatabaseError *error);

Q_SIGNALS:
    void eventsStored(const QVector<SyncCache::Event> &events);
    void eventsDeleted(const QVector<SyncCache::Event> &events);
    void eventsFlaggedForDeletion(const QVector<SyncCache::Event> &events);
};

class EventCachePrivate;
class EventCache : public QObject
{
    Q_OBJECT

public:
    EventCache(QObject *parent = nullptr);
    ~EventCache();

    static QString eventCacheDir(int accountId);
    static QString eventCacheRootDir();

public Q_SLOTS:
    virtual void openDatabase(const QString &accountType); // e.g. "nextcloud"

    virtual void requestEvents(int accountId, bool includeLocallyDeleted = true);
    virtual void deleteEvent(int accountId, const QString &eventId);
    virtual void flagEventForDeletion(int accountId, const QString &eventId);

    virtual void populateEventImage(int idempToken, int accountId, const QString &eventId, const QNetworkRequest &requestTemplate);

Q_SIGNALS:
    void openDatabaseFailed(const QString &errorMessage);
    void openDatabaseFinished();

    void requestEventsFailed(int accountId, const QString &errorMessage);
    void requestEventsFinished(int accountId, const QVector<SyncCache::Event> &events);

    void deleteEventFailed(int accountId, const QString &eventId, const QString &errorMessage);
    void deleteEventFinished(int accountId, const QString &eventId);

    void flagEventForDeletionFailed(int accountId, const QString &eventId, const QString &errorMessage);
    void flagEventForDeletionFinished(int accountId, const QString &eventId);

    void populateEventImageFailed(int idempToken, const QString &errorMessage);
    void populateEventImageFinished(int idempToken, const QString &path);

    void eventsStored(const QVector<SyncCache::Event> &photos);
    void eventsDeleted(const QVector<SyncCache::Event> &photos);
    void eventsFlaggedForDeletion(const QVector<SyncCache::Event> &events);

private:
    Q_DECLARE_PRIVATE(EventCache)
    QScopedPointer<EventCachePrivate> d_ptr;
};

} // namespace SyncCache

Q_DECLARE_METATYPE(SyncCache::Event)
Q_DECLARE_METATYPE(QVector<SyncCache::Event>)
Q_DECLARE_TYPEINFO(SyncCache::Event, Q_MOVABLE_TYPE);

#endif // NEXTCLOUD_SYNCCACHEEVENTS_H
