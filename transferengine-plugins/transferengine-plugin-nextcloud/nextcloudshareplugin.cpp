/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "nextcloudshareplugin.h"
#include "nextclouduploader.h"
#include "nextcloudplugininfo.h"

#include <QtPlugin>

NextcloudSharePlugin::NextcloudSharePlugin()
{
}

NextcloudSharePlugin::~NextcloudSharePlugin()
{
}

MediaTransferInterface * NextcloudSharePlugin::transferObject()
{
    return new NextcloudUploader;
}

TransferPluginInfo *NextcloudSharePlugin::infoObject()
{
    return new NextcloudPluginInfo;
}

QString NextcloudSharePlugin::pluginId() const
{
    return "Nextcloud";
}

bool NextcloudSharePlugin::enabled() const
{
    return true;
}
