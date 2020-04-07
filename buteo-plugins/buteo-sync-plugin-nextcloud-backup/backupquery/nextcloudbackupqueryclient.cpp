/****************************************************************************************
**
** Copyright (c) 2020 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "nextcloudbackupqueryclient.h"
#include "syncer_p.h"

// Buteo
#include <PluginCbInterface.h>
#include <SyncProfile.h>

extern "C" NextcloudBackupQueryClient* createPlugin(
        const QString& pluginName,
        const Buteo::SyncProfile& profile,
        Buteo::PluginCbInterface *cbInterface)
{
    return new NextcloudBackupQueryClient(pluginName, profile, cbInterface);
}

extern "C" void destroyPlugin(NextcloudBackupQueryClient *client)
{
    delete client;
}

NextcloudBackupQueryClient::NextcloudBackupQueryClient(
        const QString& pluginName,
        const Buteo::SyncProfile& profile,
        Buteo::PluginCbInterface *cbInterface)
    : NextcloudBackupOperationClient(pluginName, profile, cbInterface)
{
}

NextcloudBackupQueryClient::~NextcloudBackupQueryClient()
{
}

Syncer *NextcloudBackupQueryClient::newSyncer()
{
    return new Syncer(this, &profile(), Syncer::BackupQuery);
}
