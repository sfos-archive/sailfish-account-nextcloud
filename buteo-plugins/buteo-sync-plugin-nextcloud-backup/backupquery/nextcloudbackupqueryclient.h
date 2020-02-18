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

class Q_DECL_EXPORT NextcloudBackupQueryClient : public NextcloudBackupOperationClient
{
    Q_OBJECT

public:
    NextcloudBackupQueryClient(
            const QString &pluginName,
            const Buteo::SyncProfile &profile,
            Buteo::PluginCbInterface *cbInterface);
    ~NextcloudBackupQueryClient();

protected:
    Syncer *newSyncer() override;
};

extern "C" NextcloudBackupQueryClient* createPlugin(const QString &pluginName,
                                               const Buteo::SyncProfile &profile,
                                               Buteo::PluginCbInterface *cbInterface);

extern "C" void destroyPlugin(NextcloudBackupQueryClient *client);

#endif // NEXTCLOUD_BACKUPQUERY_CLIENT_H
