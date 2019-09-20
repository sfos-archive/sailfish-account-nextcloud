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
#include <QNetworkAccessManager>

NextcloudSharePlugin::NextcloudSharePlugin()
    : QObject(), TransferPluginInterface()
    , m_qnam(new QNetworkAccessManager(this))
{
}

NextcloudSharePlugin::~NextcloudSharePlugin()
{
}

MediaTransferInterface * NextcloudSharePlugin::transferObject()
{
    return new NextcloudUploader(m_qnam, this);
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
