/****************************************************************************************
**
** Copyright (c) 2020 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "nextcloudbackupclient.h"
#include "syncer_p.h"

// Buteo
#include <PluginCbInterface.h>
#include <SyncProfile.h>

extern "C" NextcloudBackupClient* createPlugin(
        const QString& pluginName,
        const Buteo::SyncProfile& profile,
        Buteo::PluginCbInterface *cbInterface)
{
    return new NextcloudBackupClient(pluginName, profile, cbInterface);
}

extern "C" void destroyPlugin(NextcloudBackupClient *client)
{
    delete client;
}

NextcloudBackupClient::NextcloudBackupClient(
        const QString& pluginName,
        const Buteo::SyncProfile& profile,
        Buteo::PluginCbInterface *cbInterface)
    : NextcloudBackupOperationClient(pluginName, profile, cbInterface)
{
}

NextcloudBackupClient::~NextcloudBackupClient()
{
}

Syncer *NextcloudBackupClient::newSyncer()
{
    return new Syncer(this, &profile(), Syncer::Backup);
}

