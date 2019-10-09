/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_EVENTCACHE_H
#define NEXTCLOUD_EVENTCACHE_H

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
    QString eventText;
    QUrl eventUrl;
    QUrl imageUrl;
    QUrl imagePath;
    QDateTime timestamp;
};

struct DatabaseError {
    enum ErrorCode {
        NoError = 0,
        CreateError,
        OpenError,
        AlreadyOpenError,
        NotOpenError,
        ConfigurationError,
        IntegrityCheckError,
        UpgradeError,
        ProcessMutexError,
        VersionQueryError,
        VersionMismatchError,
        TransactionError,
        TransactionLockError,
        PrepareQueryError,
        QueryError,
        UnknownError = 1024
    };
    ErrorCode errorCode = NoError;
    QString errorMessage;
};

class EventDatabasePrivate;
class EventDatabase : public QObject
{
    Q_OBJECT

public:
    EventDatabase(QObject *parent = Q_NULLPTR);
    virtual ~EventDatabase();

    ProcessMutex *processMutex() const;
    void openDatabase(const QString &fileName, SyncCache::DatabaseError *error);

    QVector<SyncCache::Event> events(int accountId, SyncCache::DatabaseError *error) const;
    SyncCache::Event event(int accountId, const QString &eventId, SyncCache::DatabaseError *error) const;
    void storeEvent(const SyncCache::Event &event, SyncCache::DatabaseError *error);
    void deleteEvent(const SyncCache::Event &event, SyncCache::DatabaseError *error);

    bool inTransaction() const;
    bool beginTransaction(SyncCache::DatabaseError *error);
    bool commitTransation(SyncCache::DatabaseError *error);
    bool rollbackTransaction(SyncCache::DatabaseError *error);

Q_SIGNALS:
    void eventsStored(const QVector<SyncCache::Event> &event);
    void eventsDeleted(const QVector<SyncCache::Event> &event);

private:
    Q_DECLARE_PRIVATE(EventDatabase)
    QScopedPointer<EventDatabasePrivate> d_ptr;
};

class EventCachePrivate;
class EventCache : public QObject
{
    Q_OBJECT

public:
    EventCache(QObject *parent = Q_NULLPTR);
    ~EventCache();

public Q_SLOTS:
    virtual void openDatabase(const QString &accountType); // e.g. "nextcloud"

    virtual void requestEvents(int accountId);

    virtual void populateEventImage(int idempToken, int accountId, const QString &eventId, const QNetworkRequest &requestTemplate);

Q_SIGNALS:
    void openDatabaseFailed(const QString &errorMessage);
    void openDatabaseFinished();

    void requestEventsFailed(int accountId, const QString &errorMessage);
    void requestEventsFinished(int accountId, const QVector<SyncCache::Event> &events);

    void populateEventImageFailed(int idempToken, const QString &errorMessage);
    void populateEventImageFinished(int idempToken, const QString &path);

    void eventsStored(const QVector<SyncCache::Event> &photos);
    void eventsDeleted(const QVector<SyncCache::Event> &photos);

private:
    Q_DECLARE_PRIVATE(EventCache)
    QScopedPointer<EventCachePrivate> d_ptr;
};

} // namespace SyncCache

Q_DECLARE_METATYPE(SyncCache::Event)
Q_DECLARE_METATYPE(QVector<SyncCache::Event>)
Q_DECLARE_TYPEINFO(SyncCache::Event, Q_MOVABLE_TYPE);

#endif // NEXTCLOUD_EVENTCACHE_H
