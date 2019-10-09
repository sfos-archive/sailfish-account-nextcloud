/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "eventcache.h"
#include "eventcache_p.h"

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QUuid>

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>

using namespace SyncCache;

EventDatabasePrivate::EventDatabasePrivate(EventDatabase *parent)
    : DatabasePrivate(parent), m_eventDbParent(parent)
{
}

int EventDatabasePrivate::currentSchemaVersion() const
{
    return 1;
}

QVector<const char *> EventDatabasePrivate::createStatements() const
{
    static const char *createEventsTable =
            "\n CREATE TABLE Events ("
            "\n accountId INTEGER,"
            "\n eventId TEXT,"
            "\n eventText TEXT,"
            "\n eventUrl TEXT,"
            "\n imageUrl TEXT,"
            "\n imagePath TEXT,"
            "\n timestamp TEXT,"
            "\n PRIMARY KEY (accountId, eventId));";
    static QVector<const char *> retn { createEventsTable };
    return retn;
}

QVector<UpgradeOperation> EventDatabasePrivate::upgradeVersions() const
{
    static const char *upgradeVersion0to1[] = {
        "PRAGMA user_version=1",
        0 // NULL-terminated
    };

    // example for future: upgrading the schema can be done with upgrade version elements as follows:
    //static const char *anotherUpgradeStatement = "\n UPDATE Events ... etc etc";
    //static const char *upgradeVersion1to2[] = {
    //     anotherUpgradeStatement,
    //     "PRAGMA user_version=2",
    //     0 // NULL-terminated
    //};
    //static bool twiddleSomeDataViaCpp(QSqlDatabase &database) { /* do queries on database... */ return true; }

    static QVector<UpgradeOperation> retn {
        { 0, upgradeVersion0to1 },
    //  { twiddleSomeDataViaCpp, upgradeVersion1to2 },
    };

    return retn;
}

void EventDatabasePrivate::preTransactionCommit()
{
    // nothing to do.
}

void EventDatabasePrivate::transactionCommittedPreUnlock()
{
    m_tempFilesToDelete = m_filesToDelete;
    m_tempDeletedEvents = m_deletedEvents;
    m_tempStoredEvents = m_storedEvents;
    m_filesToDelete.clear();
    m_deletedEvents.clear();
    m_storedEvents.clear();
}

void EventDatabasePrivate::transactionCommittedPostUnlock()
{
    for (const QString &doomed : m_tempFilesToDelete) {
        if (QFile::exists(doomed)) {
            QFile::remove(doomed);
        }
    }
    if (!m_tempDeletedEvents.isEmpty()) {
        emit m_eventDbParent->eventsDeleted(m_tempDeletedEvents);
    }
    if (!m_tempStoredEvents.isEmpty()) {
        emit m_eventDbParent->eventsStored(m_tempStoredEvents);
    }
}

void EventDatabasePrivate::transactionRolledBackPreUnlocked()
{
    m_filesToDelete.clear();
    m_deletedEvents.clear();
    m_storedEvents.clear();
}

//-----------------------------------------------------------------------------

EventDatabase::EventDatabase(QObject *parent)
    : Database(new EventDatabasePrivate(this), parent)
{
    qRegisterMetaType<SyncCache::Event>();
    qRegisterMetaType<QVector<SyncCache::Event> >();
}

QVector<SyncCache::Event> EventDatabase::events(int accountId, DatabaseError *error) const
{
    SYNCCACHE_DB_D(const EventDatabase);

    const QString queryString = QStringLiteral("SELECT eventId, eventText, eventUrl, imageUrl, imagePath, timestamp FROM Events"
                                               " WHERE accountId = :accountId"
                                               " ORDER BY timestamp DESC, eventId ASC");

    const QList<QPair<QString, QVariant> > bindValues {
        qMakePair<QString, QVariant>(QStringLiteral(":accountId"), accountId)
    };

    auto resultHandler = [accountId](DatabaseQuery &selectQuery) -> SyncCache::Event {
        int whichValue = 0;
        Event currEvent;
        currEvent.accountId = accountId;
        currEvent.eventId = selectQuery.value(whichValue++).toString();
        currEvent.eventText = selectQuery.value(whichValue++).toString();
        currEvent.eventUrl = QUrl(selectQuery.value(whichValue++).toString());
        currEvent.imageUrl = QUrl(selectQuery.value(whichValue++).toString());
        currEvent.imagePath = QUrl(selectQuery.value(whichValue++).toString());
        currEvent.timestamp = QDateTime::fromString(selectQuery.value(whichValue++).toString(), Qt::ISODate);
        return currEvent;
    };

    return DatabaseImpl::fetchMultiple<SyncCache::Event>(
            d,
            queryString,
            bindValues,
            resultHandler,
            QStringLiteral("events"),
            error);
}

Event EventDatabase::event(int accountId, const QString &eventId, DatabaseError *error) const
{
    SYNCCACHE_DB_D(const EventDatabase);

    const QString queryString = QStringLiteral("SELECT eventText, eventUrl, imageUrl, imagePath, timestamp FROM Events"
                                               " WHERE accountId = :accountId AND eventId = :eventId");

    const QList<QPair<QString, QVariant> > bindValues {
        qMakePair<QString, QVariant>(QStringLiteral(":accountId"), accountId),
        qMakePair<QString, QVariant>(QStringLiteral(":eventId"), eventId)
    };

    auto resultHandler = [accountId, eventId](DatabaseQuery &selectQuery) -> SyncCache::Event {
        int whichValue = 0;
        Event currEvent;
        currEvent.accountId = accountId;
        currEvent.eventId = eventId;
        currEvent.eventText = selectQuery.value(whichValue++).toString();
        currEvent.eventUrl = QUrl(selectQuery.value(whichValue++).toString());
        currEvent.imageUrl = QUrl(selectQuery.value(whichValue++).toString());
        currEvent.imagePath = QUrl(selectQuery.value(whichValue++).toString());
        currEvent.timestamp = QDateTime::fromString(selectQuery.value(whichValue++).toString(), Qt::ISODate);
        return currEvent;
    };

    return DatabaseImpl::fetch<SyncCache::Event>(
            d,
            queryString,
            bindValues,
            resultHandler,
            QStringLiteral("event"),
            error);
}

void EventDatabase::storeEvent(const Event &event, DatabaseError *error)
{
    SYNCCACHE_DB_D(EventDatabase);

    DatabaseError err;
    const Event existingEvent = this->event(event.accountId, event.eventId, &err);
    if (err.errorCode != DatabaseError::NoError) {
        setDatabaseError(error, err.errorCode,
                         QStringLiteral("Error while querying existing event %1 for store: %2")
                                   .arg(event.eventId, err.errorMessage));
        return;
    }

    const QString insertString = QStringLiteral("INSERT INTO Events (accountId, eventId, eventText, eventUrl, imageUrl, imagePath, timestamp)"
                                                " VALUES(:accountId, :eventId, :eventText, :eventUrl, :imageUrl, :imagePath, :timestamp)");
    const QString updateString = QStringLiteral("UPDATE Events SET eventText = :eventText, eventUrl = :eventUrl, imageUrl = :imageUrl, imagePath = :imagePath, timestamp = :timestamp"
                                                " WHERE accountId = :accountId AND eventId = :eventId");

    const bool insert = existingEvent.eventId.isEmpty();
    const QString queryString = insert ? insertString : updateString;

    const QList<QPair<QString, QVariant> > bindValues {
        qMakePair<QString, QVariant>(QStringLiteral(":accountId"), event.accountId),
        qMakePair<QString, QVariant>(QStringLiteral(":eventId"), event.eventId),
        qMakePair<QString, QVariant>(QStringLiteral(":eventText"), event.eventText),
        qMakePair<QString, QVariant>(QStringLiteral(":eventUrl"), event.eventUrl),
        qMakePair<QString, QVariant>(QStringLiteral(":imageUrl"), event.imageUrl),
        qMakePair<QString, QVariant>(QStringLiteral(":imagePath"), event.imagePath),
        qMakePair<QString, QVariant>(QStringLiteral(":timestamp"), event.timestamp.toString(Qt::ISODate)),
    };

    auto storeResultHandler = [d, event]() -> void {
        d->m_storedEvents.append(event);
    };

    DatabaseImpl::store<SyncCache::Event>(
            d,
            queryString,
            bindValues,
            storeResultHandler,
            QStringLiteral("event"),
            error);
}

void EventDatabase::deleteEvent(const Event &event, DatabaseError *error)
{
    SYNCCACHE_DB_D(EventDatabase);

    DatabaseError err;
    const Event existingEvent = this->event(event.accountId, event.eventId, &err);
    if (err.errorCode != DatabaseError::NoError) {
        setDatabaseError(error, err.errorCode,
                         QStringLiteral("Error while querying existing event %1 for delete: %2")
                                   .arg(event.eventId, err.errorMessage));
        return;
    }

    if (existingEvent.eventId.isEmpty()) {
        // does not exist in database, already deleted.
        return;
    }

    auto deleteRelatedValues = [] (DatabaseError *) -> void { };

    const QString queryString = QStringLiteral("DELETE FROM Events"
                                               " WHERE accountId = :accountId AND eventId = :eventId");

    const QList<QPair<QString, QVariant> > bindValues {
        qMakePair<QString, QVariant>(QStringLiteral(":accountId"), event.accountId),
        qMakePair<QString, QVariant>(QStringLiteral(":eventId"), event.eventId)
    };

    auto deleteResultHandler = [d, existingEvent]() -> void {
        d->m_deletedEvents.append(existingEvent);
        if (!existingEvent.imagePath.isEmpty()) {
            d->m_filesToDelete.append(existingEvent.imagePath.toString());
        }
    };

    DatabaseImpl::deleteValue<SyncCache::Event>(
            d,
            deleteRelatedValues,
            queryString,
            bindValues,
            deleteResultHandler,
            QStringLiteral("event"),
            error);
}
