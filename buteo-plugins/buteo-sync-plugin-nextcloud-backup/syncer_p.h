/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_BACKUP_SYNCER_P_H
#define NEXTCLOUD_BACKUP_SYNCER_P_H

#include "webdavsyncer_p.h"

// sailfishaccounts
#include <accountsyncmanager.h>

class QFile;

class Syncer : public WebDavSyncer
{
    Q_OBJECT

public:
    Syncer(QObject *parent, Buteo::SyncProfile *profile);
   ~Syncer();

    void purgeAccount(int accountId) Q_DECL_OVERRIDE;

private:
    void beginSync() Q_DECL_OVERRIDE;

    bool performDirCreationRequest(const QStringList &remotePathParts, int remotePathPartsIndex);
    void handleDirCreationReply();

    bool performDirListingRequest(const QString &remoteDirPath);
    void handleDirListingReply();

    bool performUploadRequest(const QStringList &fileNameList);
    void handleUploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void handleUploadReply();

    bool performDownloadRequest(const QStringList &fileNameList);
    void handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void handleDownloadReply();

    void finishWithHttpError(const QString &errorMessage, int httpCode);
    void finishWithError(const QString &errorMessage);
    void finishWithSuccess();

    bool loadConfig();

    QFile *m_downloadedFile = nullptr;
    AccountSyncManager m_accountSyncManager;
    AccountSyncManager::BackupRestoreOptions m_backupRestoreOptions;
    QStringList m_backupFileNames;
};

#endif // NEXTCLOUD_BACKUP_SYNCER_P_H
