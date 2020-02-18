/****************************************************************************************
**
** Copyright (c) 2020 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_BACKUPQUERY_CLIENT_H
#define NEXTCLOUD_BACKUPQUERY_CLIENT_H

#include "nextcloudbackupoperationclient.h"

class Q_DECL_EXPORT NextcloudBackupRestoreClient : public NextcloudBackupOperationClient
{
    Q_OBJECT

public:
    NextcloudBackupRestoreClient(
            const QString &pluginName,
            const Buteo::SyncProfile &profile,
            Buteo::PluginCbInterface *cbInterface);
    ~NextcloudBackupRestoreClient();

protected:
    Syncer *newSyncer() override;
};

extern "C" NextcloudBackupRestoreClient* createPlugin(const QString &pluginName,
                                               const Buteo::SyncProfile &profile,
                                               Buteo::PluginCbInterface *cbInterface);

extern "C" void destroyPlugin(NextcloudBackupRestoreClient *client);

#endif // NEXTCLOUD_BACKUPQUERY_CLIENT_H
