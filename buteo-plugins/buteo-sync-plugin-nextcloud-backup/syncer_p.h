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

#ifndef NEXTCLOUD_BACKUP_SYNCER_P_H
#define NEXTCLOUD_BACKUP_SYNCER_P_H

#include "webdavsyncer_p.h"

// libaccounts-qt5
#include <Accounts/Manager>

#include <QFileInfo>
#include <QDir>

class QFile;
class WebDavRequestGenerator;
class QDBusInterface;

class Syncer : public WebDavSyncer
{
    Q_OBJECT

public:
    enum Operation {
        Backup,
        BackupQuery,
        BackupRestore
    };
    Q_ENUM(Operation)

    Syncer(QObject *parent, Buteo::SyncProfile *profile, Operation operation);
   ~Syncer();

    void purgeAccount(int accountId) override;

private Q_SLOTS:
    void cloudBackupStatusChanged(int accountId, const QString &status);
    void cloudBackupError(int accountId, const QString &error, const QString &errorString);
    void cloudRestoreStatusChanged(int accountId, const QString &status);
    void cloudRestoreError(int accountId, const QString &error, const QString &errorString);

private:
    void beginSync() override;

    bool performDirCreationRequest(const QStringList &remotePathParts, int remotePathPartsIndex);
    void handleDirCreationReply();

    bool performDirListingRequest(const QString &remoteDirPath);
    void handleDirListingReply();

    bool performUploadRequest(const QString &fileNameList);
    void handleUploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void handleUploadReply();

    bool performDownloadRequest(const QString &fileName);
    void handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void handleDownloadReply();

    void finishWithHttpError(const QString &errorMessage, int httpCode);
    void finishWithError(const QString &errorMessage);
    void finishWithSuccess();

    void cleanUp();

    QString initBackupDir();

    Accounts::Manager *m_manager = nullptr;
    QFile *m_downloadedFile = nullptr;
    QDBusInterface *m_sailfishBackup = nullptr;
    QFileInfo m_localFileInfo;
    QString m_remoteBackupDirPath;
    Operation m_operation = BackupQuery;
};

#endif // NEXTCLOUD_BACKUP_SYNCER_P_H
