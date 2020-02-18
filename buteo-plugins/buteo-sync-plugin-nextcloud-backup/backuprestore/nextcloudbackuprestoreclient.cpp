/****************************************************************************************
**
** Copyright (c) 2020 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "nextcloudbackuprestoreclient.h"
#include "syncer_p.h"

// Buteo
#include <PluginCbInterface.h>
#include <SyncProfile.h>

extern "C" NextcloudBackupRestoreClient* createPlugin(
        const QString& pluginName,
        const Buteo::SyncProfile& profile,
        Buteo::PluginCbInterface *cbInterface)
{
    return new NextcloudBackupRestoreClient(pluginName, profile, cbInterface);
}

extern "C" void destroyPlugin(NextcloudBackupRestoreClient *client)
{
    delete client;
}

NextcloudBackupRestoreClient::NextcloudBackupRestoreClient(
        const QString& pluginName,
        const Buteo::SyncProfile& profile,
        Buteo::PluginCbInterface *cbInterface)
    : NextcloudBackupOperationClient(pluginName, profile, cbInterface)
{
}

NextcloudBackupRestoreClient::~NextcloudBackupRestoreClient()
{
}

Syncer *NextcloudBackupRestoreClient::newSyncer()
{
    return new Syncer(this, &profile(), Syncer::BackupRestore);
}


