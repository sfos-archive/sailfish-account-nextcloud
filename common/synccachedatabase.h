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

#ifndef NEXTCLOUD_SYNCCACHE_DATABASE_H
#define NEXTCLOUD_SYNCCACHE_DATABASE_H

#include <QtCore/QObject>
#include <QtCore/QString>

namespace SyncCache {

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
        InvalidArgumentError,
        UnknownError = 1024
    };
    ErrorCode errorCode = NoError;
    QString errorMessage;
};

class ProcessMutex;
class DatabasePrivate;
class Database : public QObject
{
    Q_OBJECT

public:
    virtual ~Database();
    ProcessMutex *processMutex() const;
    void openDatabase(const QString &fileName, SyncCache::DatabaseError *error);

    bool inTransaction() const;
    bool beginTransaction(SyncCache::DatabaseError *error);
    bool commitTransaction(SyncCache::DatabaseError *error);
    bool rollbackTransaction(SyncCache::DatabaseError *error);

    static void setDatabaseError(DatabaseError *error, DatabaseError::ErrorCode code, const QString &message);

protected:
    explicit Database(DatabasePrivate *dptr, QObject *parent = nullptr);
    Q_DECLARE_PRIVATE(Database)
    Q_DISABLE_COPY(Database)
    QScopedPointer<DatabasePrivate> d_ptr;
};

} // namespace SyncCache

#endif // NEXTCLOUD_SYNCCACHE_DATABASE_H
