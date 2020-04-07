/****************************************************************************************
**
** Copyright (c) 2020 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_BACKUP_CLIENT_H
#define NEXTCLOUD_BACKUP_CLIENT_H

#include "nextcloudbackupoperationclient.h"

class Q_DECL_EXPORT NextcloudBackupClient : public NextcloudBackupOperationClient
{
    Q_OBJECT

public:
    NextcloudBackupClient(
            const QString &pluginName,
            const Buteo::SyncProfile &profile,
            Buteo::PluginCbInterface *cbInterface);
    ~NextcloudBackupClient();

protected:
    Syncer *newSyncer() override;
};

extern "C" NextcloudBackupClient* createPlugin(const QString &pluginName,
                                               const Buteo::SyncProfile &profile,
                                               Buteo::PluginCbInterface *cbInterface);

extern "C" void destroyPlugin(NextcloudBackupClient *client);

#endif // NEXTCLOUD_BACKUP_CLIENT_H
