/****************************************************************************************
**
** Copyright (C) 2013-2019 Jolla Ltd
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

static const char *setupEncoding =
        "\n PRAGMA encoding = \"UTF-16\";";

static const char *setupTempStore =
        "\n PRAGMA temp_store = MEMORY;";

static const char *setupJournal =
        "\n PRAGMA journal_mode = WAL;";

static const char *setupSynchronous =
        "\n PRAGMA synchronous = FULL;";

static const char *setupForeignKeys =
        "\n PRAGMA foreign_keys = ON;";

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

static const char *createStatements[] =
{
    createEventsTable,
};

typedef bool (*UpgradeFunction)(QSqlDatabase &database);
struct UpgradeOperation {
    UpgradeFunction fn;
    const char **statements;
};

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

static UpgradeOperation upgradeVersions[] = {
    { 0,  upgradeVersion0to1 },
//    { twiddleSomeDataViaCpp,                      upgradeVersion1to2 },
};

static const int currentSchemaVersion = 1;

template <typename T> static int lengthOf(T) { return 0; }
template <typename T, int N> static int lengthOf(const T(&)[N]) { return N; }

static bool execute(QSqlDatabase &database, const QString &statement)
{
    QSqlQuery query(database);
    if (!query.exec(statement)) {
        qWarning() << QString::fromLatin1("Query failed: %1\n%2")
                                     .arg(query.lastError().text())
                                     .arg(statement);
        return false;
    } else {
        return true;
    }
}

static bool beginTransaction(QSqlDatabase &database)
{
    // Use immediate lock acquisition; we should already have an IPC lock, so
    // there will be no lock contention with other writing processes
    return execute(database, QString::fromLatin1("BEGIN IMMEDIATE TRANSACTION"));
}

static bool commitTransaction(QSqlDatabase &database)
{
    return execute(database, QString::fromLatin1("COMMIT TRANSACTION"));
}

static bool rollbackTransaction(QSqlDatabase &database)
{
    return execute(database, QString::fromLatin1("ROLLBACK TRANSACTION"));
}

static bool finalizeTransaction(QSqlDatabase &database, bool success)
{
    if (success) {
        return commitTransaction(database);
    }

    rollbackTransaction(database);
    return false;
}

static bool executeUpgradeStatements(QSqlDatabase &database)
{
    // Check that the defined schema matches the array of upgrade scripts
    if (currentSchemaVersion != lengthOf(upgradeVersions)) {
        qWarning() << "Invalid schema version:" << currentSchemaVersion;
        return false;
    }

    QSqlQuery versionQuery(database);
    versionQuery.prepare("PRAGMA user_version");
    if (!versionQuery.exec() || !versionQuery.next()) {
        qWarning() << "User version query failed:" << versionQuery.lastError();
        return false;
    }

    quint16 schemaVersion = versionQuery.value(0).toInt();
    versionQuery.finish();

    while (schemaVersion < currentSchemaVersion) {
        qWarning() << "Upgrading Nextcloud events database from schema version" << schemaVersion;

        if (upgradeVersions[schemaVersion].fn) {
            if (!(*upgradeVersions[schemaVersion].fn)(database)) {
                qWarning() << "Unable to update data for schema version" << schemaVersion;
                return false;
            }
        }
        if (upgradeVersions[schemaVersion].statements) {
            for (unsigned i = 0; upgradeVersions[schemaVersion].statements[i]; i++) {
                if (!execute(database, QLatin1String(upgradeVersions[schemaVersion].statements[i])))
                    return false;
            }
        }

        if (!versionQuery.exec() || !versionQuery.next()) {
            qWarning() << "User version query failed:" << versionQuery.lastError();
            return false;
        }

        int version = versionQuery.value(0).toInt();
        versionQuery.finish();

        if (version <= schemaVersion) {
            qWarning() << "Nextcloud events database schema upgrade cycle detected - aborting";
            return false;
        } else {
            schemaVersion = version;
            if (schemaVersion == currentSchemaVersion) {
                qWarning() << "Nextcloud events database upgraded to version" << schemaVersion;
            }
        }
    }

    if (schemaVersion > currentSchemaVersion) {
        qWarning() << "Nextcloud events database schema is newer than expected - this may result in failures or corruption";
    }

    return true;
}

static bool checkDatabase(QSqlDatabase &database)
{
    QSqlQuery query(database);
    if (query.exec(QLatin1String("PRAGMA quick_check"))) {
        while (query.next()) {
            const QString result(query.value(0).toString());
            if (result == QLatin1String("ok")) {
                return true;
            }
            qWarning() << "Integrity problem:" << result;
        }
    }

    return false;
}

static bool upgradeDatabase(QSqlDatabase &database)
{
    if (!beginTransaction(database))
        return false;

    bool success = executeUpgradeStatements(database);

    return finalizeTransaction(database, success);
}

static bool configureDatabase(QSqlDatabase &database)
{
    if (!execute(database, QLatin1String(setupEncoding))
            || !execute(database, QLatin1String(setupTempStore))
            || !execute(database, QLatin1String(setupJournal))
            || !execute(database, QLatin1String(setupSynchronous))
            || !execute(database, QLatin1String(setupForeignKeys))) {
        return false;
    }

    return true;
}

static bool executeCreationStatements(QSqlDatabase &database)
{
    for (int i = 0; i < lengthOf(createStatements); ++i) {
        QSqlQuery query(database);

        if (!query.exec(QLatin1String(createStatements[i]))) {
            qWarning() << QString::fromLatin1("Database creation failed: %1\n%2")
                                         .arg(query.lastError().text())
                                         .arg(createStatements[i]);
            return false;
        }
    }

    if (!execute(database, QString::fromLatin1("PRAGMA user_version=%1").arg(currentSchemaVersion))) {
        return false;
    }

    return true;
}

static bool prepareDatabase(QSqlDatabase &database)
{
    if (!configureDatabase(database))
        return false;

    if (!beginTransaction(database))
        return false;

    bool success = executeCreationStatements(database);

    return finalizeTransaction(database, success);
}

namespace {
    void setDatabaseError(DatabaseError *error, DatabaseError::ErrorCode code, const QString &message) {
        if (error) {
            error->errorCode = code;
            error->errorMessage = message;
        }
    }
}


EventDatabase::EventDatabase(QObject *parent)
    : QObject(parent), d_ptr(new EventDatabasePrivate)
{
    qRegisterMetaType<SyncCache::Event>();
    qRegisterMetaType<QVector<SyncCache::Event> >();
}

EventDatabase::~EventDatabase()
{
}

ProcessMutex *EventDatabase::processMutex() const
{
    Q_D(const EventDatabase);

    if (!d->m_processMutex) {
        Q_ASSERT(d->m_database.isOpen());
        d->m_processMutex.reset(new ProcessMutex(d->m_database.databaseName()));
    }
    return d->m_processMutex.data();
}

void EventDatabase::openDatabase(const QString &fileName, DatabaseError *error)
{
    Q_D(EventDatabase);

    if (d->m_database.isOpen()) {
        setDatabaseError(error, DatabaseError::AlreadyOpenError,
                         QString::fromLatin1("Unable to open database when already open: %1").arg(fileName));
        return;
    }

    QDir dir = QFileInfo(fileName).dir();
    if (!dir.exists()) {
        if (!dir.mkpath(QStringLiteral("."))) {
            setDatabaseError(error, DatabaseError::CreateError,
                             QString::fromLatin1("Unable to create database directory: %1").arg(dir.path()));
            return;
        }
    }

    const bool databasePreexisting = QFile::exists(fileName);
    const QString connectionName = QUuid::createUuid().toString().mid(1, 36);
    d->m_database = QSqlDatabase::addDatabase(QString::fromLatin1("QSQLITE"), connectionName);
    d->m_database.setDatabaseName(fileName);

    if (!d->m_database.open()) {
        setDatabaseError(error,
                         databasePreexisting ? DatabaseError::OpenError
                                             : DatabaseError::CreateError,
                         databasePreexisting ? QString::fromLatin1("Unable to open database: %1: %2")
                                                              .arg(fileName, d->m_database.lastError().text())
                                             : QString::fromLatin1("Unable to create database: %1: %2")
                                                              .arg(fileName, d->m_database.lastError().text()));
        return;
    }

    if (!databasePreexisting && !prepareDatabase(d->m_database)) {
        setDatabaseError(error, DatabaseError::CreateError,
                         QString::fromLatin1("Failed to prepare database: %1")
                                        .arg(d->m_database.lastError().text()));
        d->m_database.close();
        QFile::remove(fileName);
        return;
    } else if (databasePreexisting && !configureDatabase(d->m_database)) {
        setDatabaseError(error, DatabaseError::ConfigurationError,
                         QString::fromLatin1("Failed to configure Nextcloud events database: %1")
                                        .arg(d->m_database.lastError().text()));
        d->m_database.close();
        return;
    }

    // Get the process mutex for this database
    ProcessMutex *mutex(processMutex());

    // Only the first connection in the first process to concurrently open the DB is the owner
    const bool databaseOwner(mutex->isInitialProcess());

    if (databasePreexisting && databaseOwner) {
        // Try to upgrade, if necessary
        if (!mutex->lock()) {
            setDatabaseError(error, DatabaseError::ProcessMutexError,
                             QString::fromLatin1("Failed to lock mutex for contacts database: %1")
                                            .arg(fileName));
            d->m_database.close();
            return;
        }

        // Perform an integrity check
        if (!checkDatabase(d->m_database)) {
            setDatabaseError(error, DatabaseError::IntegrityCheckError,
                             QString::fromLatin1("Database integrity check failed: %1")
                                            .arg(d->m_database.lastError().text()));
            d->m_database.close();
            mutex->unlock();
            return;
        }

        if (!upgradeDatabase(d->m_database)) {
            setDatabaseError(error, DatabaseError::UpgradeError,
                             QString::fromLatin1("Failed to upgrade contacts database: %1")
                                            .arg(d->m_database.lastError().text()));
            d->m_database.close();
            mutex->unlock();
            return;
        }

        mutex->unlock();
    } else if (databasePreexisting && !databaseOwner) {
        // check that the version is correct.  If not, it is probably because another process
        // with an open database connection is preventing upgrade of the database schema.
        QSqlQuery versionQuery(d->m_database);
        versionQuery.prepare("PRAGMA user_version");
        if (!versionQuery.exec() || !versionQuery.next()) {
            setDatabaseError(error, DatabaseError::VersionQueryError,
                             QString::fromLatin1("Failed to query existing database schema version: %1")
                                            .arg(versionQuery.lastError().text()));
            d->m_database.close();
            return;
        }

        const int schemaVersion = versionQuery.value(0).toInt();
        if (schemaVersion != currentSchemaVersion) {
            setDatabaseError(error, DatabaseError::VersionMismatchError,
                             QString::fromLatin1("Existing database schema version is unexpected: %1 != %2. "
                                                 "Is a process preventing schema upgrade?")
                                            .arg(schemaVersion).arg(currentSchemaVersion));
            d->m_database.close();
            return;
        }
    }
}

QVector<SyncCache::Event> EventDatabase::events(int accountId, DatabaseError *error) const
{
    Q_D(const EventDatabase);

    QVector<SyncCache::Event> retn;
    if (!d->m_database.isOpen()) {
        setDatabaseError(error, DatabaseError::NotOpenError,
                         QStringLiteral("Database is not open, cannot query events"));
        return retn;
    }

    const QString queryString = QStringLiteral("SELECT accountId, eventId, eventText, eventUrl, imageUrl, imagePath, timestamp FROM Events"
                                               " WHERE accountId = :accountId"
                                               " ORDER BY timestamp DESC, eventId ASC");

    QSqlQuery selectQuery(d->m_database);
    selectQuery.setForwardOnly(true);
    if (!selectQuery.prepare(queryString)) {
        setDatabaseError(error, DatabaseError::PrepareQueryError,
                         QStringLiteral("Failed to prepare events query: %1\n%2")
                                   .arg(selectQuery.lastError().text())
                                   .arg(queryString));
        return retn;
    }

    selectQuery.bindValue(QStringLiteral(":accountId"), accountId);

    if (!selectQuery.exec()) {
        setDatabaseError(error, DatabaseError::QueryError,
                         QStringLiteral("Failed to execute events query: %1\n%2")
                                   .arg(selectQuery.lastError().text())
                                   .arg(queryString));
        return retn;
    }
    while (selectQuery.next()) {
        Event currEvent;
        int i = 0;
        currEvent.accountId = selectQuery.value(i++).toInt();
        currEvent.eventId = selectQuery.value(i++).toString();
        currEvent.eventText = selectQuery.value(i++).toString();
        currEvent.eventUrl = selectQuery.value(i++).toString();
        currEvent.imageUrl = QUrl(selectQuery.value(i++).toString());
        currEvent.imagePath = QUrl(selectQuery.value(i++).toString());
        currEvent.timestamp = QDateTime::fromString(selectQuery.value(i++).toString(), Qt::ISODate);
        retn.append(currEvent);
    }

    return retn;
}

Event EventDatabase::event(int accountId, const QString &eventId, DatabaseError *error) const
{
    Q_D(const EventDatabase);

    Event retn;
    if (!d->m_database.isOpen()) {
        setDatabaseError(error, DatabaseError::NotOpenError,
                         QStringLiteral("Database is not open, cannot query event"));
        return retn;
    }

    const QString queryString = QStringLiteral("SELECT eventText, eventUrl, imageUrl, imagePath, timestamp FROM Events"
                                               " WHERE accountId = :accountId AND eventId = :eventId");

    QSqlQuery selectQuery(d->m_database);
    selectQuery.setForwardOnly(true);
    if (!selectQuery.prepare(queryString)) {
        setDatabaseError(error, DatabaseError::PrepareQueryError,
                         QStringLiteral("Failed to prepare event query: %1\n%2")
                                   .arg(selectQuery.lastError().text())
                                   .arg(queryString));
        return retn;
    }

    selectQuery.bindValue(":accountId", accountId);
    selectQuery.bindValue(":eventId", eventId);
    if (!selectQuery.exec()) {
        setDatabaseError(error, DatabaseError::QueryError,
                         QStringLiteral("Failed to execute event query: %1\n%2")
                                   .arg(selectQuery.lastError().text())
                                   .arg(queryString));
        return retn;
    }
    if (selectQuery.next()) {
        int i = 0;
        retn.accountId = accountId;
        retn.eventId = eventId;
        retn.eventText = selectQuery.value(i++).toString();
        retn.eventUrl = selectQuery.value(i++).toString();
        retn.imageUrl = QUrl(selectQuery.value(i++).toString());
        retn.imagePath = QUrl(selectQuery.value(i++).toString());
        retn.timestamp = QDateTime::fromString(selectQuery.value(i++).toString(), Qt::ISODate);
    }

    return retn;
}

void EventDatabase::storeEvent(const Event &event, DatabaseError *error)
{
    Q_D(EventDatabase);

    if (!d->m_database.isOpen()) {
        setDatabaseError(error, DatabaseError::NotOpenError,
                         QStringLiteral("Database is not open, cannot store event"));
        return;
    }

    DatabaseError err;
    const Event existingEvent = this->event(event.accountId, event.eventId, &err);
    if (err.errorCode != DatabaseError::NoError) {
        setDatabaseError(error, err.errorCode, err.errorMessage);
        return;
    }

    const QString insertString = QStringLiteral("INSERT INTO Events (accountId, eventId, eventText, eventUrl, imageUrl, imagePath, timestamp)"
                                                " VALUES(:accountId, :eventId, :eventText, :eventUrl, :imageUrl, :imagePath, :timestamp)");
    const QString updateString = QStringLiteral("UPDATE Events SET eventText = :eventText, eventUrl = :eventUrl, imageUrl = :imageUrl, imagePath = :imagePath, timestamp = :timestamp"
                                                " WHERE accountId = :accountId AND eventId = :eventId");

    const bool insert = existingEvent.eventId.isEmpty();
    const QString queryString = insert ? insertString : updateString;

    QSqlQuery storeQuery(d->m_database);
    if (!storeQuery.prepare(queryString)) {
        setDatabaseError(error, DatabaseError::PrepareQueryError,
                         QStringLiteral("Failed to prepare store event query: %1\n%2")
                                   .arg(storeQuery.lastError().text())
                                   .arg(queryString));
        return;
    }

    storeQuery.bindValue(":accountId", event.accountId);
    storeQuery.bindValue(":eventId", event.eventId);
    storeQuery.bindValue(":eventText", event.eventText);
    storeQuery.bindValue(":eventUrl", event.eventUrl);
    storeQuery.bindValue(":imageUrl", event.imageUrl);
    storeQuery.bindValue(":imagePath", event.imagePath);
    storeQuery.bindValue(":timestamp", event.timestamp.toString(Qt::ISODate));

    const bool wasInTransaction = inTransaction();
    if (!wasInTransaction && !beginTransaction(error)) {
        return;
    }

    if (!storeQuery.exec()) {
        setDatabaseError(error, DatabaseError::QueryError,
                         QStringLiteral("Failed to execute store event query: %1\n%2")
                                   .arg(storeQuery.lastError().text())
                                   .arg(queryString));
        if (!wasInTransaction) {
            DatabaseError rollbackError;
            rollbackTransaction(&rollbackError);
        }
    } else {
        d->m_storedEvents.append(event);
        if (!wasInTransaction && !commitTransation(error)) {
            DatabaseError rollbackError;
            rollbackTransaction(&rollbackError);
        }
    }
}

void EventDatabase::deleteEvent(const Event &event, DatabaseError *error)
{
    Q_D(EventDatabase);

    if (!d->m_database.isOpen()) {
        setDatabaseError(error, DatabaseError::NotOpenError,
                         QStringLiteral("Database is not open, cannot delete event"));
        return;
    }

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

    const bool wasInTransaction = inTransaction();
    if (!wasInTransaction && !beginTransaction(error)) {
        return;
    }

    const QString queryString = QStringLiteral("DELETE FROM Events"
                                               " WHERE accountId = :accountId AND eventId = :eventId");

    QSqlQuery deleteQuery(d->m_database);
    if (!deleteQuery.prepare(queryString)) {
        setDatabaseError(error, DatabaseError::PrepareQueryError,
                         QStringLiteral("Failed to prepare delete event query: %1\n%2")
                                   .arg(deleteQuery.lastError().text())
                                   .arg(queryString));
        if (!wasInTransaction) {
            DatabaseError rollbackError;
            rollbackTransaction(&rollbackError);
        }
        return;
    }

    deleteQuery.bindValue(":accountId", event.accountId);
    deleteQuery.bindValue(":eventId", event.eventId);

    if (!deleteQuery.exec()) {
        setDatabaseError(error, DatabaseError::QueryError,
                         QStringLiteral("Failed to execute delete event query: %1\n%2")
                                   .arg(deleteQuery.lastError().text())
                                   .arg(queryString));
        if (!wasInTransaction) {
            DatabaseError rollbackError;
            rollbackTransaction(&rollbackError);
        }
    } else {
        d->m_deletedEvents.append(existingEvent);
        if (!existingEvent.imagePath.isEmpty()) {
            d->m_filesToDelete.append(existingEvent.imagePath.toString());
        }
        if (!wasInTransaction && !commitTransation(error)) {
            DatabaseError rollbackError;
            rollbackTransaction(&rollbackError);
        }
    }
}

bool EventDatabase::inTransaction() const
{
    Q_D(const EventDatabase);
    return d->m_inTransaction;
}

bool EventDatabase::beginTransaction(DatabaseError *error)
{
    Q_D(EventDatabase);

    if (inTransaction()) {
        setDatabaseError(error, DatabaseError::TransactionError,
                         QStringLiteral("Transaction error: cannot begin transaction, already in a transaction"));
        return false;
    }

    ProcessMutex *mutex(processMutex());

    // We use a cross-process mutex to ensure only one process can write
    // to the DB at once.  Without external locking, SQLite will back off
    // on write contention, and the backed-off process may never get access
    // if other processes are performing regular writes.
    if (mutex->lock()) {
        if (::beginTransaction(d->m_database)) {
            d->m_inTransaction = true;
            return true;
        }

        setDatabaseError(error, DatabaseError::TransactionError,
                         QStringLiteral("Transaction error: unable to begin transaction: %1")
                                   .arg(d->m_database.lastError().text()));
        mutex->unlock();
    } else {
        setDatabaseError(error, DatabaseError::TransactionLockError,
                         QStringLiteral("Lock error: unable to lock for transaction"));
    }

    return false;
}

bool EventDatabase::commitTransation(DatabaseError *error)
{
    Q_D(EventDatabase);

    ProcessMutex *mutex(processMutex());

    if (mutex->isLocked()) {
        if (::commitTransaction(d->m_database)) {
            d->m_inTransaction = false;

            QVector<QString> doomedFiles = d->m_filesToDelete;
            QVector<SyncCache::Event> deletedEvents = d->m_deletedEvents;
            QVector<SyncCache::Event> storedEvents = d->m_storedEvents;

            d->m_filesToDelete.clear();
            d->m_deletedEvents.clear();
            d->m_storedEvents.clear();

            mutex->unlock();

            for (const QString &doomed : doomedFiles) {
                if (QFile::exists(doomed)) {
                    QFile::remove(doomed);
                }
            }

            if (!deletedEvents.isEmpty()) {
                emit eventsDeleted(deletedEvents);
            }

            if (!storedEvents.isEmpty()) {
                emit eventsStored(storedEvents);
            }

            return true;
        }

        setDatabaseError(error, DatabaseError::TransactionError,
                         QStringLiteral("Transaction error: unable to commit transaction: %1")
                                   .arg(d->m_database.lastError().text()));
    } else {
        setDatabaseError(error, DatabaseError::TransactionLockError,
                         QStringLiteral("Lock error: no lock held on commit"));
    }

    return false;
}

bool EventDatabase::rollbackTransaction(DatabaseError *error)
{
    Q_D(EventDatabase);

    ProcessMutex *mutex(processMutex());

    const bool rv = ::rollbackTransaction(d->m_database);
    d->m_inTransaction = false;
    d->m_filesToDelete.clear();
    d->m_deletedEvents.clear();
    d->m_storedEvents.clear();

    if (mutex->isLocked()) {
        mutex->unlock();
    } else {
        setDatabaseError(error, DatabaseError::TransactionLockError,
                         QStringLiteral("Lock error: no lock held on rollback"));
    }

    return rv;
}

