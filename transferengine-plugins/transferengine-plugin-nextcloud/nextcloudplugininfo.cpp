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
    : TransferPluginInfo()
    , m_nextcloudShareServiceStatus(new NextcloudShareServiceStatus(this))
    , m_ready(false)
{
    m_capabilities << QLatin1String("image/*")
                   << QLatin1String("*");

    QVariantMap meta;
    meta.insert(QStringLiteral("accountProviderName"), QStringLiteral("nextcloud"));
    meta.insert(QStringLiteral("capabilities"), m_capabilities);
    setMetaData(meta);

    connect(m_nextcloudShareServiceStatus, &NextcloudShareServiceStatus::serviceReady, this, &NextcloudPluginInfo::serviceReady);
    connect(m_nextcloudShareServiceStatus, &NextcloudShareServiceStatus::serviceError, this, &NextcloudPluginInfo::infoError);
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
    m_nextcloudShareServiceStatus->queryStatus(NextcloudShareServiceStatus::PassiveMode); // don't perform sign-in.
}

bool NextcloudPluginInfo::ready() const
{
    return m_ready;
}

void NextcloudPluginInfo::serviceReady()
{
    m_info.clear();
    for (int i = 0; i < m_nextcloudShareServiceStatus->count(); ++i) {
        TransferMethodInfo info;

        info.displayName     = m_nextcloudShareServiceStatus->details(i).providerName;
        info.userName        = m_nextcloudShareServiceStatus->details(i).displayName;
        info.accountId       = m_nextcloudShareServiceStatus->details(i).accountId;

        info.methodId        = QLatin1String("Nextcloud");
        info.accountIcon     = QLatin1String("image://theme/graphic-m-service-nextcloud");
        info.shareUIPath     = QLatin1String("/usr/share/nemo-transferengine/plugins/NextcloudShareDialog.qml");
        info.capabilitities  = m_capabilities;

        m_info << info;
    }

    m_ready = true;
    emit infoReady();
}
