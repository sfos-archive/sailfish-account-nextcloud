/****************************************************************************************
**
** Copyright (C) 2013-2019 Jolla Ltd
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "imagecache.h"
#include "imagecache_p.h"

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

static const char *createUsersTable =
        "\n CREATE TABLE Users ("
        "\n accountId INTEGER,"
        "\n userId TEXT,"
        "\n thumbnailUrl TEXT,"
        "\n thumbnailPath TEXT,"
        "\n PRIMARY KEY (accountId, userId));";

static const char *createAlbumsTable =
        "\n CREATE TABLE Albums ("
        "\n accountId INTEGER,"
        "\n userId TEXT,"
        "\n albumId TEXT,"
        "\n photoCount INTEGER,"
        "\n thumbnailUrl TEXT,"
        "\n thumbnailPath TEXT,"
        "\n parentAlbumId TEXT,"
        "\n albumName TEXT,"
        "\n PRIMARY KEY (accountId, userId, albumId),"
        "\n FOREIGN KEY (accountId, userId) REFERENCES Users (accountId, userId) ON DELETE CASCADE);";

static const char *createPhotosTable =
        "\n CREATE TABLE Photos ("
        "\n accountId INTEGER,"
        "\n userId TEXT,"
        "\n albumId TEXT,"
        "\n photoId TEXT,"
        "\n fileName TEXT,"
        "\n albumPath TEXT,"
        "\n description TEXT,"
        "\n createdTimestamp TEXT,"
        "\n updatedTimestamp TEXT,"
        "\n thumbnailUrl TEXT,"
        "\n thumbnailPath TEXT,"
        "\n imageUrl TEXT,"
        "\n imagePath TEXT,"
        "\n imageWidth INTEGER,"
        "\n imageHeight INTEGER,"
        "\n PRIMARY KEY (accountId, userId, albumId, photoId),"
        "\n FOREIGN KEY (accountId, userId, albumId) REFERENCES Albums (accountId, userId, albumId) ON DELETE CASCADE);";

static const char *createStatements[] =
{
    createUsersTable,
    createAlbumsTable,
    createPhotosTable,
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
//static const char *anotherUpgradeStatement = "\n UPDATE Albums ... etc etc";
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
        qWarning() << "Upgrading Nextcloud images database from schema version" << schemaVersion;

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
            qWarning() << "Nextcloud images database schema upgrade cycle detected - aborting";
            return false;
        } else {
            schemaVersion = version;
            if (schemaVersion == currentSchemaVersion) {
                qWarning() << "Nextcloud images database upgraded to version" << schemaVersion;
            }
        }
    }

    if (schemaVersion > currentSchemaVersion) {
        qWarning() << "Nextcloud images database schema is newer than expected - this may result in failures or corruption";
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

    QString constructAlbumIdentifier(int accountId, const QString &userId, const QString &albumId) {
        return QStringLiteral("%1|%2|%3").arg(accountId).arg(userId, albumId);
    }
}


ImageDatabase::ImageDatabase(QObject *parent)
    : QObject(parent), d_ptr(new ImageDatabasePrivate)
{
    qRegisterMetaType<SyncCache::User>();
    qRegisterMetaType<SyncCache::Album>();
    qRegisterMetaType<SyncCache::Photo>();
    qRegisterMetaType<QVector<SyncCache::User> >();
    qRegisterMetaType<QVector<SyncCache::User> >();
    qRegisterMetaType<QVector<SyncCache::User> >();
}

ImageDatabase::~ImageDatabase()
{
}

ProcessMutex *ImageDatabase::processMutex() const
{
    Q_D(const ImageDatabase);

    if (!d->m_processMutex) {
        Q_ASSERT(d->m_database.isOpen());
        d->m_processMutex.reset(new ProcessMutex(d->m_database.databaseName()));
    }
    return d->m_processMutex.data();
}

void ImageDatabase::openDatabase(const QString &fileName, DatabaseError *error)
{
    Q_D(ImageDatabase);

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
                         QString::fromLatin1("Failed to configure Nextcloud images database: %1")
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

QVector<SyncCache::User> ImageDatabase::users(DatabaseError *error) const
{
    Q_D(const ImageDatabase);

    QVector<SyncCache::User> retn;
    if (!d->m_database.isOpen()) {
        setDatabaseError(error, DatabaseError::NotOpenError,
                         QStringLiteral("Database is not open, cannot query users"));
        return retn;
    }

    const QString queryString = QStringLiteral("SELECT accountId, userId, thumbnailUrl, thumbnailPath FROM USERS"
                                               " ORDER BY accountId ASC, userId ASC");

    QSqlQuery selectQuery(d->m_database);
    selectQuery.setForwardOnly(true);
    if (!selectQuery.prepare(queryString)) {
        setDatabaseError(error, DatabaseError::PrepareQueryError,
                         QStringLiteral("Failed to prepare users query: %1\n%2")
                                   .arg(selectQuery.lastError().text())
                                   .arg(queryString));
        return retn;
    }
    if (!selectQuery.exec()) {
        setDatabaseError(error, DatabaseError::QueryError,
                         QStringLiteral("Failed to execute users query: %1\n%2")
                                   .arg(selectQuery.lastError().text())
                                   .arg(queryString));
        return retn;
    }
    while (selectQuery.next()) {
        User currUser;
        currUser.accountId = selectQuery.value(0).toInt();
        currUser.userId = selectQuery.value(1).toString();
        currUser.thumbnailUrl = QUrl(selectQuery.value(2).toString());
        currUser.thumbnailPath = QUrl(selectQuery.value(3).toString());
        retn.append(currUser);
    }

    return retn;
}

QVector<SyncCache::Album> ImageDatabase::albums(int accountId, const QString &userId, DatabaseError *error) const
{
    Q_D(const ImageDatabase);

    QVector<SyncCache::Album> retn;
    if (!d->m_database.isOpen()) {
        setDatabaseError(error, DatabaseError::NotOpenError,
                         QStringLiteral("Database is not open, cannot query albums"));
        return retn;
    }

    const QString queryString = QStringLiteral("SELECT albumId, photoCount, thumbnailUrl, thumbnailPath, parentAlbumId, albumName FROM ALBUMS"
                                               " WHERE accountId = :accountId AND userId = :userId"
                                               " ORDER BY accountId ASC, userId ASC, albumId ASC");

    QSqlQuery selectQuery(d->m_database);
    selectQuery.setForwardOnly(true);
    if (!selectQuery.prepare(queryString)) {
        setDatabaseError(error, DatabaseError::PrepareQueryError,
                         QStringLiteral("Failed to prepare albums query: %1\n%2")
                                   .arg(selectQuery.lastError().text())
                                   .arg(queryString));
        return retn;
    }

    selectQuery.bindValue(":accountId", accountId);
    selectQuery.bindValue(":userId", userId);
    if (!selectQuery.exec()) {
        setDatabaseError(error, DatabaseError::QueryError,
                         QStringLiteral("Failed to execute albums query: %1\n%2")
                                   .arg(selectQuery.lastError().text())
                                   .arg(queryString));
        return retn;
    }
    while (selectQuery.next()) {
        int whichValue = 0;
        Album currAlbum;
        currAlbum.accountId = accountId;
        currAlbum.userId = userId;
        currAlbum.albumId = selectQuery.value(whichValue++).toString();
        currAlbum.photoCount = selectQuery.value(whichValue++).toInt();
        currAlbum.thumbnailUrl = QUrl(selectQuery.value(whichValue++).toString());
        currAlbum.thumbnailPath = QUrl(selectQuery.value(whichValue++).toString());
        currAlbum.parentAlbumId = selectQuery.value(whichValue++).toString();
        currAlbum.albumName = selectQuery.value(whichValue++).toString();
        retn.append(currAlbum);
    }

    return retn;
}

QVector<SyncCache::Photo> ImageDatabase::photos(int accountId, const QString &userId, const QString &albumId, DatabaseError *error) const
{
    Q_D(const ImageDatabase);

    QVector<SyncCache::Photo> retn;
    if (!d->m_database.isOpen()) {
        setDatabaseError(error, DatabaseError::NotOpenError,
                         QStringLiteral("Database is not open, cannot query photos"));
        return retn;
    }

    const QString queryString = albumId.isEmpty()
                              ? QStringLiteral("SELECT albumId, photoId, createdTimestamp, updatedTimestamp, fileName, albumPath, description,"
                                                     " thumbnailUrl, thumbnailPath, imageUrl, imagePath, imageWidth, imageHeight FROM Photos"
                                               " WHERE accountId = :accountId AND userId = :userId"
                                               " ORDER BY accountId ASC, userId ASC, albumId ASC, createdTimestamp DESC")
                              : QStringLiteral("SELECT photoId, createdTimestamp, updatedTimestamp, fileName, albumPath, description,"
                                                     " thumbnailUrl, thumbnailPath, imageUrl, imagePath, imageWidth, imageHeight FROM Photos"
                                               " WHERE accountId = :accountId AND userId = :userId AND albumId = :albumId"
                                               " ORDER BY accountId ASC, userId ASC, albumId ASC, createdTimestamp DESC");

    QSqlQuery selectQuery(d->m_database);
    selectQuery.setForwardOnly(true);
    if (!selectQuery.prepare(queryString)) {
        setDatabaseError(error, DatabaseError::PrepareQueryError,
                         QStringLiteral("Failed to prepare photos query: %1\n%2")
                                   .arg(selectQuery.lastError().text())
                                   .arg(queryString));
        return retn;
    }

    selectQuery.bindValue(":accountId", accountId);
    selectQuery.bindValue(":userId", userId);
    if (!albumId.isEmpty()) {
        selectQuery.bindValue(":albumId", albumId);
    }

    if (!selectQuery.exec()) {
        setDatabaseError(error, DatabaseError::QueryError,
                         QStringLiteral("Failed to execute photos query: %1\n%2")
                                   .arg(selectQuery.lastError().text())
                                   .arg(queryString));
        return retn;
    }
    while (selectQuery.next()) {
        int whichValue = 0;
        Photo currPhoto;
        currPhoto.accountId = accountId;
        currPhoto.userId = userId;
        currPhoto.albumId = albumId.isEmpty() ? selectQuery.value(whichValue++).toString() : albumId;
        currPhoto.photoId = selectQuery.value(whichValue++).toString();
        currPhoto.createdTimestamp = selectQuery.value(whichValue++).toString();
        currPhoto.updatedTimestamp = selectQuery.value(whichValue++).toString();
        currPhoto.fileName = selectQuery.value(whichValue++).toString();
        currPhoto.albumPath = selectQuery.value(whichValue++).toString();
        currPhoto.description = selectQuery.value(whichValue++).toString();
        currPhoto.thumbnailUrl = QUrl(selectQuery.value(whichValue++).toString());
        currPhoto.thumbnailPath = QUrl(selectQuery.value(whichValue++).toString());
        currPhoto.imageUrl = QUrl(selectQuery.value(whichValue++).toString());
        currPhoto.imagePath = QUrl(selectQuery.value(whichValue++).toString());
        retn.append(currPhoto);
    }

    return retn;
}

User ImageDatabase::user(int accountId, const QString &userId, DatabaseError *error) const
{
    Q_D(const ImageDatabase);

    User retn;
    if (!d->m_database.isOpen()) {
        setDatabaseError(error, DatabaseError::NotOpenError,
                         QStringLiteral("Database is not open, cannot query user"));
        return retn;
    }

    const QString queryString = QStringLiteral("SELECT thumbnailUrl, thumbnailPath FROM USERS"
                                               " WHERE accountId = :accountId AND userId = :userId");

    QSqlQuery selectQuery(d->m_database);
    selectQuery.setForwardOnly(true);
    if (!selectQuery.prepare(queryString)) {
        setDatabaseError(error, DatabaseError::PrepareQueryError,
                         QStringLiteral("Failed to prepare user query: %1\n%2")
                                   .arg(selectQuery.lastError().text())
                                   .arg(queryString));
        return retn;
    }

    selectQuery.bindValue(":accountId", accountId);
    selectQuery.bindValue(":userId", userId);
    if (!selectQuery.exec()) {
        setDatabaseError(error, DatabaseError::QueryError,
                         QStringLiteral("Failed to execute user query: %1\n%2")
                                   .arg(selectQuery.lastError().text())
                                   .arg(queryString));
        return retn;
    }
    if (selectQuery.next()) {
        retn.accountId = accountId;
        retn.userId = userId;
        retn.thumbnailUrl = QUrl(selectQuery.value(0).toString());
        retn.thumbnailPath = QUrl(selectQuery.value(1).toString());
    }

    return retn;
}

Album ImageDatabase::album(int accountId, const QString &userId, const QString &albumId, DatabaseError *error) const
{
    Q_D(const ImageDatabase);

    Album retn;
    if (!d->m_database.isOpen()) {
        setDatabaseError(error, DatabaseError::NotOpenError,
                         QStringLiteral("Database is not open, cannot query album"));
        return retn;
    }

    const QString queryString = QStringLiteral("SELECT photoCount, thumbnailUrl, thumbnailPath, parentAlbumId, albumName FROM ALBUMS"
                                               " WHERE accountId = :accountId AND userId = :userId AND albumId = :albumId");

    QSqlQuery selectQuery(d->m_database);
    selectQuery.setForwardOnly(true);
    if (!selectQuery.prepare(queryString)) {
        setDatabaseError(error, DatabaseError::PrepareQueryError,
                         QStringLiteral("Failed to prepare album query: %1\n%2")
                                   .arg(selectQuery.lastError().text())
                                   .arg(queryString));
        return retn;
    }

    selectQuery.bindValue(":accountId", accountId);
    selectQuery.bindValue(":userId", userId);
    selectQuery.bindValue(":albumId", albumId);
    if (!selectQuery.exec()) {
        setDatabaseError(error, DatabaseError::QueryError,
                         QStringLiteral("Failed to execute album query: %1\n%2")
                                   .arg(selectQuery.lastError().text())
                                   .arg(queryString));
        return retn;
    }
    if (selectQuery.next()) {
        int whichValue = 0;
        retn.accountId = accountId;
        retn.userId = userId;
        retn.albumId = albumId;
        retn.photoCount = selectQuery.value(whichValue++).toInt();
        retn.thumbnailUrl = QUrl(selectQuery.value(whichValue++).toString());
        retn.thumbnailPath = QUrl(selectQuery.value(whichValue++).toString());
        retn.parentAlbumId = selectQuery.value(whichValue++).toString();
        retn.albumName = selectQuery.value(whichValue++).toString();
    }

    return retn;
}

Photo ImageDatabase::photo(int accountId, const QString &userId, const QString &albumId, const QString &photoId, DatabaseError *error) const
{
    Q_D(const ImageDatabase);

    Photo retn;
    if (!d->m_database.isOpen()) {
        setDatabaseError(error, DatabaseError::NotOpenError,
                         QStringLiteral("Database is not open, cannot query photo"));
        return retn;
    }

    const QString queryString = QStringLiteral("SELECT createdTimestamp, updatedTimestamp, fileName, albumPath, description,"
                                               " thumbnailUrl, thumbnailPath, imageUrl, imagePath, imageWidth, imageHeight FROM Photos"
                                               " WHERE accountId = :accountId AND userId = :userId AND albumId = :albumId AND photoId = :photoId");

    QSqlQuery selectQuery(d->m_database);
    selectQuery.setForwardOnly(true);
    if (!selectQuery.prepare(queryString)) {
        setDatabaseError(error, DatabaseError::PrepareQueryError,
                         QStringLiteral("Failed to prepare photo query: %1\n%2")
                                   .arg(selectQuery.lastError().text())
                                   .arg(queryString));
        return retn;
    }

    selectQuery.bindValue(":accountId", accountId);
    selectQuery.bindValue(":userId", userId);
    selectQuery.bindValue(":albumId", albumId);
    selectQuery.bindValue(":photoId", photoId);
    if (!selectQuery.exec()) {
        setDatabaseError(error, DatabaseError::QueryError,
                         QStringLiteral("Failed to execute photo query: %1\n%2")
                                   .arg(selectQuery.lastError().text())
                                   .arg(queryString));
        return retn;
    }
    while (selectQuery.next()) {
        int whichValue = 0;
        retn.accountId = accountId;
        retn.userId = userId;
        retn.albumId = albumId;
        retn.photoId = photoId;
        retn.createdTimestamp = selectQuery.value(whichValue++).toString();
        retn.updatedTimestamp = selectQuery.value(whichValue++).toString();
        retn.fileName = selectQuery.value(whichValue++).toString();
        retn.albumPath = selectQuery.value(whichValue++).toString();
        retn.description = selectQuery.value(whichValue++).toString();
        retn.thumbnailUrl = QUrl(selectQuery.value(whichValue++).toString());
        retn.thumbnailPath = QUrl(selectQuery.value(whichValue++).toString());
        retn.imageUrl = QUrl(selectQuery.value(whichValue++).toString());
        retn.imagePath = QUrl(selectQuery.value(whichValue++).toString());
    }

    return retn;
}

void ImageDatabase::storeUser(const User &user, DatabaseError *error)
{
    Q_D(ImageDatabase);

    if (!d->m_database.isOpen()) {
        setDatabaseError(error, DatabaseError::NotOpenError,
                         QStringLiteral("Database is not open, cannot store user"));
        return;
    }

    DatabaseError err;
    const User existingUser = this->user(user.accountId, user.userId, &err);
    if (err.errorCode != DatabaseError::NoError) {
        setDatabaseError(error, err.errorCode, err.errorMessage);
        return;
    }

    const QString insertString = QStringLiteral("INSERT INTO Users (accountId, userId, thumbnailUrl, thumbnailPath)"
                                                " VALUES(:accountId, :userId, :thumbnailUrl, :thumbnailPath)");
    const QString updateString = QStringLiteral("UPDATE Users SET thumbnailUrl = :thumbnailUrl, thumbnailPath = :thumbnailPath"
                                                " WHERE accountId = :accountId AND userId = :userId");

    const bool insert = existingUser.userId.isEmpty();
    const QString queryString = insert ? insertString : updateString;

    QSqlQuery storeQuery(d->m_database);
    if (!storeQuery.prepare(queryString)) {
        setDatabaseError(error, DatabaseError::PrepareQueryError,
                         QStringLiteral("Failed to prepare store user query: %1\n%2")
                                   .arg(storeQuery.lastError().text())
                                   .arg(queryString));
        return;
    }

    storeQuery.bindValue(":accountId", user.accountId);
    storeQuery.bindValue(":userId", user.userId);
    storeQuery.bindValue(":thumbnailUrl", user.thumbnailUrl);
    storeQuery.bindValue(":thumbnailPath", user.thumbnailPath);

    const bool wasInTransaction = inTransaction();
    if (!wasInTransaction && !beginTransaction(error)) {
        return;
    }

    if (!storeQuery.exec()) {
        setDatabaseError(error, DatabaseError::QueryError,
                         QStringLiteral("Failed to execute store user query: %1\n%2")
                                   .arg(storeQuery.lastError().text())
                                   .arg(queryString));
        if (!wasInTransaction) {
            DatabaseError rollbackError;
            rollbackTransaction(&rollbackError);
        }
    } else {
        d->m_storedUsers.append(user);
        if (!wasInTransaction && !commitTransation(error)) {
            DatabaseError rollbackError;
            rollbackTransaction(&rollbackError);
        }
    }
}

void ImageDatabase::storeAlbum(const Album &album, DatabaseError *error)
{
    Q_D(ImageDatabase);

    if (!d->m_database.isOpen()) {
        setDatabaseError(error, DatabaseError::NotOpenError,
                         QStringLiteral("Database is not open, cannot store album"));
        return;
    }

    DatabaseError err;
    const Album existingAlbum = this->album(album.accountId, album.userId, album.albumId, &err);
    if (err.errorCode != DatabaseError::NoError) {
        setDatabaseError(error, err.errorCode, err.errorMessage);
        return;
    }

    const QString insertString = QStringLiteral("INSERT INTO Albums (accountId, userId, albumId, photoCount, thumbnailUrl, thumbnailPath, parentAlbumId, albumName)"
                                                " VALUES(:accountId, :userId, :albumId, :photoCount, :thumbnailUrl, :thumbnailPath, :parentAlbumId, :albumName)");
    const QString updateString = QStringLiteral("UPDATE Albums SET photoCount = :photoCount, thumbnailUrl = :thumbnailUrl, thumbnailPath = :thumbnailPath, "
                                                                  "parentAlbumId = :parentAlbumId, albumName = :albumName"
                                                " WHERE accountId = :accountId AND userId = :userId AND albumId = :albumId");

    const bool insert = existingAlbum.albumId.isEmpty();
    const QString queryString = insert ? insertString : updateString;

    QSqlQuery storeQuery(d->m_database);
    if (!storeQuery.prepare(queryString)) {
        setDatabaseError(error, DatabaseError::PrepareQueryError,
                         QStringLiteral("Failed to prepare store album query: %1\n%2")
                                   .arg(storeQuery.lastError().text())
                                   .arg(queryString));
        return;
    }

    storeQuery.bindValue(":accountId", album.accountId);
    storeQuery.bindValue(":userId", album.userId);
    storeQuery.bindValue(":albumId", album.albumId);
    storeQuery.bindValue(":photoCount", album.photoCount);
    storeQuery.bindValue(":thumbnailUrl", album.thumbnailUrl);
    storeQuery.bindValue(":thumbnailPath", album.thumbnailPath);
    storeQuery.bindValue(":parentAlbumId", album.parentAlbumId);
    storeQuery.bindValue(":albumName", album.albumName);

    const bool wasInTransaction = inTransaction();
    if (!wasInTransaction && !beginTransaction(error)) {
        return;
    }

    if (!storeQuery.exec()) {
        setDatabaseError(error, DatabaseError::QueryError,
                         QStringLiteral("Failed to execute store album query: %1\n%2")
                                   .arg(storeQuery.lastError().text())
                                   .arg(queryString));
        if (!wasInTransaction) {
            DatabaseError rollbackError;
            rollbackTransaction(&rollbackError);
        }
    } else {
        d->m_storedAlbums.append(album);
        if (!wasInTransaction && !commitTransation(error)) {
            DatabaseError rollbackError;
            rollbackTransaction(&rollbackError);
        }
    }
}

void ImageDatabase::storePhoto(const Photo &photo, DatabaseError *error)
{
    Q_D(ImageDatabase);

    if (!d->m_database.isOpen()) {
        setDatabaseError(error, DatabaseError::NotOpenError,
                         QStringLiteral("Database is not open, cannot store photo"));
        return;
    }

    DatabaseError err;
    const Photo existingPhoto = this->photo(photo.accountId, photo.userId, photo.albumId, photo.photoId, &err);
    if (err.errorCode != DatabaseError::NoError) {
        setDatabaseError(error, err.errorCode, err.errorMessage);
        return;
    }

    const QString insertString = QStringLiteral("INSERT INTO Photos (accountId, userId, albumId, photoId, createdTimestamp, updatedTimestamp, "
                                                                    "fileName, albumPath, description, thumbnailUrl, thumbnailPath, "
                                                                    "imageUrl, imagePath, imageWidth, imageHeight)"
                                                " VALUES(:accountId, :userId, :albumId, :photoId, :createdTimestamp, :updatedTimestamp, "
                                                        ":fileName, :albumPath, :description, :thumbnailUrl, :thumbnailPath, :imageUrl, :imagePath, :imageWidth, :imageHeight)");
    const QString updateString = QStringLiteral("UPDATE Photos SET createdTimestamp = :createdTimestamp, updatedTimestamp = :updatedTimestamp, "
                                                                  "fileName = :fileName, albumPath = :albumPath, description = :description, "
                                                                  "thumbnailUrl = :thumbnailUrl, thumbnailPath = :thumbnailPath, "
                                                                  "imageUrl = :imageUrl, imagePath = :imagePath, imageWidth = :imageWidth, imageHeight = :imageHeight"
                                                " WHERE accountId = :accountId AND userId = :userId AND albumId = :albumId AND photoId = :photoId");

    const bool insert = existingPhoto.photoId.isEmpty();
    const QString queryString = insert ? insertString : updateString;

    QSqlQuery storeQuery(d->m_database);
    if (!storeQuery.prepare(queryString)) {
        setDatabaseError(error, DatabaseError::PrepareQueryError,
                         QStringLiteral("Failed to prepare store photo query: %1\n%2")
                                   .arg(storeQuery.lastError().text())
                                   .arg(queryString));
        return;
    }

    storeQuery.bindValue(":accountId", photo.accountId);
    storeQuery.bindValue(":userId", photo.userId);
    storeQuery.bindValue(":albumId", photo.albumId);
    storeQuery.bindValue(":photoId", photo.photoId);
    storeQuery.bindValue(":createdTimestamp", photo.createdTimestamp);
    storeQuery.bindValue(":updatedTimestamp", photo.updatedTimestamp);
    storeQuery.bindValue(":fileName", photo.fileName);
    storeQuery.bindValue(":albumPath", photo.albumPath);
    storeQuery.bindValue(":description", photo.description);
    storeQuery.bindValue(":thumbnailUrl", photo.thumbnailUrl);
    storeQuery.bindValue(":thumbnailPath", photo.thumbnailPath);
    storeQuery.bindValue(":imageUrl", photo.imageUrl);
    storeQuery.bindValue(":imagePath", photo.imagePath);
    storeQuery.bindValue(":imageWidth", photo.imageWidth);
    storeQuery.bindValue(":imageHeight", photo.imageHeight);

    const bool wasInTransaction = inTransaction();
    if (!wasInTransaction && !beginTransaction(error)) {
        return;
    }

    if (!storeQuery.exec()) {
        setDatabaseError(error, DatabaseError::QueryError,
                         QStringLiteral("Failed to execute store photo query: %1\n%2")
                                   .arg(storeQuery.lastError().text())
                                   .arg(queryString));
        if (!wasInTransaction) {
            DatabaseError rollbackError;
            rollbackTransaction(&rollbackError);
        }
    } else {
        d->m_storedPhotos.append(photo);
        if (!insert) {
            if (!existingPhoto.imagePath.isEmpty()
                    && existingPhoto.imagePath != photo.imagePath) {
                d->m_filesToDelete.append(existingPhoto.imagePath.toString());
            }
            if (!existingPhoto.thumbnailPath.isEmpty()
                    && existingPhoto.thumbnailPath != photo.thumbnailPath) {
                d->m_filesToDelete.append(existingPhoto.thumbnailPath.toString());
            }
        }
        if (!wasInTransaction && !commitTransation(error)) {
            DatabaseError rollbackError;
            rollbackTransaction(&rollbackError);
        }
    }
}

void ImageDatabase::deleteUser(const User &user, DatabaseError *error)
{
    Q_D(ImageDatabase);

    if (!d->m_database.isOpen()) {
        setDatabaseError(error, DatabaseError::NotOpenError,
                         QStringLiteral("Database is not open, cannot delete user"));
        return;
    }

    DatabaseError err;
    const User existingUser = this->user(user.accountId, user.userId, &err);
    if (err.errorCode != DatabaseError::NoError) {
        setDatabaseError(error, err.errorCode,
                         QStringLiteral("Error while querying existing user %1 for delete: %2")
                                   .arg(user.userId, err.errorMessage));
        return;
    }

    if (existingUser.userId.isEmpty()) {
        // does not exist in database, already deleted.
        return;
    }

    // delete all albums for this user, to ensure artifacts (e.g. downloaded files) are also deleted.
    QVector<SyncCache::Album> userAlbums = this->albums(user.accountId, user.userId, &err);
    if (err.errorCode != DatabaseError::NoError) {
        setDatabaseError(error, err.errorCode,
                         QStringLiteral("Error while querying albums for user %1 for delete: %2")
                                   .arg(user.userId, err.errorMessage));
        return;
    }

    const bool wasInTransaction = inTransaction();
    if (!wasInTransaction && !beginTransaction(error)) {
        return;
    }

    for (const Album &doomed : userAlbums) {
        deleteAlbum(doomed, &err);
        if (err.errorCode != DatabaseError::NoError) {
            setDatabaseError(error, err.errorCode,
                             QStringLiteral("Error while deleting album %1 for user %2: %3")
                                       .arg(doomed.albumId, doomed.userId, err.errorMessage));
            if (!wasInTransaction) {
                DatabaseError rollbackError;
                rollbackTransaction(&rollbackError);
            }
            return;
        }
    }

    const QString queryString = QStringLiteral("DELETE FROM Users"
                                               " WHERE accountId = :accountId AND userId = :userId");

    QSqlQuery deleteQuery(d->m_database);
    if (!deleteQuery.prepare(queryString)) {
        setDatabaseError(error, DatabaseError::PrepareQueryError,
                         QStringLiteral("Failed to prepare delete photo query: %1\n%2")
                                   .arg(deleteQuery.lastError().text())
                                   .arg(queryString));
        if (!wasInTransaction) {
            DatabaseError rollbackError;
            rollbackTransaction(&rollbackError);
        }
        return;
    }

    deleteQuery.bindValue(":accountId", user.accountId);
    deleteQuery.bindValue(":userId", user.userId);

    if (!deleteQuery.exec()) {
        setDatabaseError(error, DatabaseError::QueryError,
                         QStringLiteral("Failed to execute delete user query: %1\n%2")
                                   .arg(deleteQuery.lastError().text())
                                   .arg(queryString));
        if (!wasInTransaction) {
            DatabaseError rollbackError;
            rollbackTransaction(&rollbackError);
        }
    } else {
        d->m_deletedUsers.append(existingUser);
        if (!existingUser.thumbnailPath.isEmpty()) {
            d->m_filesToDelete.append(existingUser.thumbnailPath.toString());
        }
        if (!wasInTransaction && !commitTransation(error)) {
            DatabaseError rollbackError;
            rollbackTransaction(&rollbackError);
        }
    }
}

void ImageDatabase::deleteAlbum(const Album &album, DatabaseError *error)
{
    Q_D(ImageDatabase);

    if (!d->m_database.isOpen()) {
        setDatabaseError(error, DatabaseError::NotOpenError,
                         QStringLiteral("Database is not open, cannot delete album"));
        return;
    }

    DatabaseError err;
    const Album existingAlbum = this->album(album.accountId, album.userId, album.albumId, &err);
    if (err.errorCode != DatabaseError::NoError) {
        setDatabaseError(error, err.errorCode,
                         QStringLiteral("Error while querying existing album %1 for delete: %2")
                                   .arg(album.albumId, err.errorMessage));
        return;
    }

    if (existingAlbum.albumId.isEmpty()) {
        // does not exist in database, already deleted.
        return;
    }

    // delete all photos in this album, to ensure artifacts (e.g. downloaded files) are also deleted.
    QVector<SyncCache::Photo> albumPhotos = this->photos(album.accountId, album.userId, album.albumId, &err);
    if (err.errorCode != DatabaseError::NoError) {
        setDatabaseError(error, err.errorCode,
                         QStringLiteral("Error while querying photos from album %1 for delete: %2")
                                   .arg(album.albumId, err.errorMessage));
        return;
    }

    const bool wasInTransaction = inTransaction();
    if (!wasInTransaction && !beginTransaction(error)) {
        return;
    }

    for (const Photo &doomed : albumPhotos) {
        deletePhoto(doomed, &err);
        if (err.errorCode != DatabaseError::NoError) {
            setDatabaseError(error, err.errorCode,
                             QStringLiteral("Error while deleting photo %1 from album %2: %3")
                                       .arg(doomed.photoId, album.albumId, err.errorMessage));
            if (!wasInTransaction) {
                DatabaseError rollbackError;
                rollbackTransaction(&rollbackError);
            }
            return;
        }
    }

    const QString queryString = QStringLiteral("DELETE FROM Albums"
                                               " WHERE accountId = :accountId AND userId = :userId AND albumId = :albumId");

    QSqlQuery deleteQuery(d->m_database);
    if (!deleteQuery.prepare(queryString)) {
        setDatabaseError(error, DatabaseError::PrepareQueryError,
                         QStringLiteral("Failed to prepare delete photo query: %1\n%2")
                                   .arg(deleteQuery.lastError().text())
                                   .arg(queryString));
        if (!wasInTransaction) {
            DatabaseError rollbackError;
            rollbackTransaction(&rollbackError);
        }
        return;
    }

    deleteQuery.bindValue(":accountId", album.accountId);
    deleteQuery.bindValue(":userId", album.userId);
    deleteQuery.bindValue(":albumId", album.albumId);

    if (!deleteQuery.exec()) {
        setDatabaseError(error, DatabaseError::QueryError,
                         QStringLiteral("Failed to execute delete album query: %1\n%2")
                                   .arg(deleteQuery.lastError().text())
                                   .arg(queryString));
        if (!wasInTransaction) {
            DatabaseError rollbackError;
            rollbackTransaction(&rollbackError);
        }
    } else {
        d->m_deletedAlbums.append(existingAlbum);
        if (!existingAlbum.thumbnailUrl.isEmpty() && !existingAlbum.thumbnailPath.isEmpty()) {
            // only delete the thumbnail if it was downloaded especially from url,
            // but not if it is from a photo (in that case, thumbnailUrl will be empty).
            d->m_filesToDelete.append(existingAlbum.thumbnailPath.toString());
        }
        if (!wasInTransaction && !commitTransation(error)) {
            DatabaseError rollbackError;
            rollbackTransaction(&rollbackError);
        }
    }
}

void ImageDatabase::deletePhoto(const Photo &photo, DatabaseError *error)
{
    Q_D(ImageDatabase);

    if (!d->m_database.isOpen()) {
        setDatabaseError(error, DatabaseError::NotOpenError,
                         QStringLiteral("Database is not open, cannot delete photo"));
        return;
    }

    DatabaseError err;
    const Photo existingPhoto = this->photo(photo.accountId, photo.userId, photo.albumId, photo.photoId, &err);
    if (err.errorCode != DatabaseError::NoError) {
        setDatabaseError(error, err.errorCode,
                         QStringLiteral("Error while querying existing photo %1 for delete: %2")
                                   .arg(photo.photoId, err.errorMessage));
        return;
    }

    if (existingPhoto.photoId.isEmpty()) {
        // does not exist in database, already deleted.
        return;
    }

    const bool wasInTransaction = inTransaction();
    if (!wasInTransaction && !beginTransaction(error)) {
        return;
    }

    const QString queryString = QStringLiteral("DELETE FROM Photos"
                                               " WHERE accountId = :accountId AND userId = :userId AND albumId = :albumId AND photoId = :photoId");

    QSqlQuery deleteQuery(d->m_database);
    if (!deleteQuery.prepare(queryString)) {
        setDatabaseError(error, DatabaseError::PrepareQueryError,
                         QStringLiteral("Failed to prepare delete photo query: %1\n%2")
                                   .arg(deleteQuery.lastError().text())
                                   .arg(queryString));
        if (!wasInTransaction) {
            DatabaseError rollbackError;
            rollbackTransaction(&rollbackError);
        }
        return;
    }

    deleteQuery.bindValue(":accountId", photo.accountId);
    deleteQuery.bindValue(":userId", photo.userId);
    deleteQuery.bindValue(":albumId", photo.albumId);
    deleteQuery.bindValue(":photoId", photo.photoId);

    if (!deleteQuery.exec()) {
        setDatabaseError(error, DatabaseError::QueryError,
                         QStringLiteral("Failed to execute delete photo query: %1\n%2")
                                   .arg(deleteQuery.lastError().text())
                                   .arg(queryString));
        if (!wasInTransaction) {
            DatabaseError rollbackError;
            rollbackTransaction(&rollbackError);
        }
    } else {
        d->m_deletedPhotos.append(existingPhoto);
        if (!existingPhoto.thumbnailPath.isEmpty()) {
            d->m_filesToDelete.append(existingPhoto.thumbnailPath.toString());
        }
        if (!existingPhoto.imagePath.isEmpty()) {
            d->m_filesToDelete.append(existingPhoto.imagePath.toString());
        }
        if (!wasInTransaction && !commitTransation(error)) {
            DatabaseError rollbackError;
            rollbackTransaction(&rollbackError);
        }
    }
}

bool ImageDatabase::inTransaction() const
{
    Q_D(const ImageDatabase);
    return d->m_inTransaction;
}

bool ImageDatabase::beginTransaction(DatabaseError *error)
{
    Q_D(ImageDatabase);

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

bool ImageDatabase::commitTransation(DatabaseError *error)
{
    Q_D(ImageDatabase);

    ProcessMutex *mutex(processMutex());

    if (mutex->isLocked()) {
        // Fixup album thumbnails if required.
        if (!d->m_deletedPhotos.isEmpty()
                || !d->m_storedPhotos.isEmpty()) {
            // potentially need to update album thumbnails.
            QSet<QString> doomedAlbums;
            Q_FOREACH (const SyncCache::Album &doomed, d->m_deletedAlbums) {
                doomedAlbums.insert(constructAlbumIdentifier(doomed.accountId, doomed.userId, doomed.albumId));
            }
            SyncCache::DatabaseError thumbnailError;
            QHash<QString, SyncCache::Album> thumbnailAlbums;
            Q_FOREACH (const SyncCache::Photo &photo, d->m_deletedPhotos) {
                const QString albumIdentifier = constructAlbumIdentifier(photo.accountId, photo.userId, photo.albumId);
                if (!thumbnailAlbums.contains(albumIdentifier) && !doomedAlbums.contains(albumIdentifier)) {
                    SyncCache::Album album = this->album(photo.accountId, photo.userId, photo.albumId, &thumbnailError);
                    if (thumbnailError.errorCode == SyncCache::DatabaseError::NoError && !album.albumId.isEmpty()) {
                        thumbnailAlbums.insert(albumIdentifier, album);
                    }
                }
            }
            thumbnailError.errorCode = SyncCache::DatabaseError::NoError;
            Q_FOREACH (const SyncCache::Photo &photo, d->m_storedPhotos) {
                const QString albumIdentifier = constructAlbumIdentifier(photo.accountId, photo.userId, photo.albumId);
                if (!thumbnailAlbums.contains(albumIdentifier)) {
                    SyncCache::Album album = this->album(photo.accountId, photo.userId, photo.albumId, &thumbnailError);
                    if (thumbnailError.errorCode == SyncCache::DatabaseError::NoError && !album.albumId.isEmpty()) {
                        thumbnailAlbums.insert(albumIdentifier, album);
                    }
                }
            }
            thumbnailError.errorCode = SyncCache::DatabaseError::NoError;
            Q_FOREACH (const SyncCache::Album &album, thumbnailAlbums) {
                // if the album uses a photo image as its thumbnail
                // and if that photo image is set to be deleted,
                // use another photo's image.
                if (album.thumbnailUrl.isEmpty()
                        && (album.thumbnailPath.isEmpty()
                              || d->m_filesToDelete.contains(album.thumbnailPath.toString()))) {
                    QVector<SyncCache::Photo> photos = this->photos(album.accountId, album.userId, album.albumId, &thumbnailError);
                    Q_FOREACH (const SyncCache::Photo &photo, photos) {
                        if (!photo.imagePath.isEmpty()) {
                            SyncCache::Album updateAlbum = album;
                            updateAlbum.thumbnailPath = photo.imagePath;
                            this->storeAlbum(updateAlbum, &thumbnailError);
                            break;
                        }
                    }
                }
            }
        }

        if (::commitTransaction(d->m_database)) {
            d->m_inTransaction = false;

            QVector<QString> doomedFiles = d->m_filesToDelete;
            QVector<SyncCache::User> deletedUsers = d->m_deletedUsers;
            QVector<SyncCache::Album> deletedAlbums = d->m_deletedAlbums;
            QVector<SyncCache::Photo> deletedPhotos = d->m_deletedPhotos;
            QVector<SyncCache::User> storedUsers = d->m_storedUsers;
            QVector<SyncCache::Album> storedAlbums = d->m_storedAlbums;
            QVector<SyncCache::Photo> storedPhotos = d->m_storedPhotos;

            d->m_filesToDelete.clear();
            d->m_deletedUsers.clear();
            d->m_deletedAlbums.clear();
            d->m_deletedPhotos.clear();
            d->m_storedUsers.clear();
            d->m_storedAlbums.clear();
            d->m_storedPhotos.clear();

            mutex->unlock();

            for (const QString &doomed : doomedFiles) {
                if (QFile::exists(doomed)) {
                    QFile::remove(doomed);
                }
            }

            if (!deletedUsers.isEmpty()) {
                emit usersDeleted(deletedUsers);
            }
            if (!deletedAlbums.isEmpty()) {
                emit albumsDeleted(deletedAlbums);
            }
            if (!deletedPhotos.isEmpty()) {
                emit photosDeleted(deletedPhotos);
            }

            if (!storedUsers.isEmpty()) {
                emit usersStored(storedUsers);
            }
            if (!storedAlbums.isEmpty()) {
                emit albumsStored(storedAlbums);
            }
            if (!storedPhotos.isEmpty()) {
                emit photosStored(storedPhotos);
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

bool ImageDatabase::rollbackTransaction(DatabaseError *error)
{
    Q_D(ImageDatabase);

    ProcessMutex *mutex(processMutex());

    const bool rv = ::rollbackTransaction(d->m_database);
    d->m_inTransaction = false;
    d->m_filesToDelete.clear();
    d->m_deletedUsers.clear();
    d->m_deletedAlbums.clear();
    d->m_deletedPhotos.clear();
    d->m_storedUsers.clear();
    d->m_storedAlbums.clear();
    d->m_storedPhotos.clear();

    if (mutex->isLocked()) {
        mutex->unlock();
    } else {
        setDatabaseError(error, DatabaseError::TransactionLockError,
                         QStringLiteral("Lock error: no lock held on rollback"));
    }

    return rv;
}

