/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "nextcloudplugininfo.h"
#include "nextcloudshareservicestatus.h"

NextcloudPluginInfo::NextcloudPluginInfo()
    : m_nextcloudShareServiceStatus(new NextcloudShareServiceStatus(this))
{
    m_ready = false;
    connect(m_nextcloudShareServiceStatus, SIGNAL(serviceReady()), this, SLOT(serviceReady()));
    connect(m_nextcloudShareServiceStatus, SIGNAL(error(QString)), this, SIGNAL(infoError(QString)));
}

NextcloudPluginInfo::~NextcloudPluginInfo()
{
}

QList<TransferMethodInfo> NextcloudPluginInfo::info() const
{
    return m_info;
}

void NextcloudPluginInfo::query()
{
    m_nextcloudShareServiceStatus->queryStatus(ServiceStatus::PassiveMode); // don't perform sign-in.
}

bool NextcloudPluginInfo::ready() const
{
    return m_ready;
}

void NextcloudPluginInfo::serviceReady()
{
    for (int i=0; i < m_nextcloudShareServiceStatus->count(); i++) {
        TransferMethodInfo info;

        QStringList capabilities;
        capabilities << QLatin1String("image/*")
                     << QLatin1String("video/*");

        info.displayName     = m_nextcloudShareServiceStatus->details(i).value(QLatin1String("providerName")).toString();
        info.userName        = m_nextcloudShareServiceStatus->details(i).value(QLatin1String("accountName")).toString();
        info.accountId       = m_nextcloudShareServiceStatus->details(i).value(QLatin1String("identifier")).toInt();

        info.methodId        = QLatin1String("Nextcloud");
        info.accountIcon     = QLatin1String("image://theme/icon-m-service-nextcloud");
        info.shareUIPath     = QLatin1String("/usr/share/nemo-transferengine/plugins/NextcloudShareImage.qml");
        info.capabilitities  = capabilities;
        m_info << info;
    }

    m_ready = true;
    emit infoReady();
}
