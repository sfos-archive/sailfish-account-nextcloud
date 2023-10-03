/****************************************************************************************
** Copyright (c) 2019 - 2021 Open Mobile Platform LLC.
** Copyright (c) 2023 Jolla Ltd.
**
** All rights reserved.
**
** This file is part of Sailfish Nextcloud account package.
**
** You may use this file under the terms of BSD license as follows:
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** 1. Redistributions of source code must retain the above copyright notice, this
**    list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright notice,
**    this list of conditions and the following disclaimer in the documentation
**    and/or other materials provided with the distribution.
**
** 3. Neither the name of the copyright holder nor the names of its
**    contributors may be used to endorse or promote products derived from
**    this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
** OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************************/

#include "nextcloudplugininfo.h"
#include "nextcloudshareservicestatus.h"

NextcloudPluginInfo::NextcloudPluginInfo()
    : SharingPluginInfo()
    , m_nextcloudShareServiceStatus(new NextcloudShareServiceStatus(this))
{
    m_capabilities << QLatin1String("application/*")
                   << QLatin1String("audio/*")
                   << QLatin1String("image/*")
                   << QLatin1String("video/*")
                   << QLatin1String("text/x-vnote")
                   << QLatin1String("text/xml")
                   << QLatin1String("text/plain");

    connect(m_nextcloudShareServiceStatus, &NextcloudShareServiceStatus::serviceReady, this, &NextcloudPluginInfo::serviceReady);
    connect(m_nextcloudShareServiceStatus, &NextcloudShareServiceStatus::serviceError, this, &NextcloudPluginInfo::infoError);
}

NextcloudPluginInfo::~NextcloudPluginInfo()
{
}

QList<SharingMethodInfo> NextcloudPluginInfo::info() const
{
    return m_info;
}

void NextcloudPluginInfo::query()
{
    m_nextcloudShareServiceStatus->queryStatus(NextcloudShareServiceStatus::PassiveMode); // don't perform sign-in.
}

void NextcloudPluginInfo::serviceReady()
{
    m_info.clear();
    for (int i = 0; i < m_nextcloudShareServiceStatus->count(); ++i) {
        SharingMethodInfo info;

        info.setDisplayName(m_nextcloudShareServiceStatus->details(i).providerName);
        info.setSubtitle(m_nextcloudShareServiceStatus->details(i).displayName);
        info.setAccountId(m_nextcloudShareServiceStatus->details(i).accountId);

        info.setMethodId(QLatin1String("Nextcloud"));
        info.setMethodIcon(QLatin1String("image://theme/graphic-m-service-nextcloud"));
        info.setShareUIPath(QLatin1String("/usr/share/nemo-transferengine/plugins/sharing/NextcloudShareFile.qml"));
        info.setCapabilities(m_capabilities);

        m_info << info;
    }

    emit infoReady();
}
