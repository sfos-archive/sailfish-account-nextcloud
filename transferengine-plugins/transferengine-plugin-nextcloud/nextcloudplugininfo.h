/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUDPLUGININFO_H
#define NEXTCLOUDPLUGININFO_H

#include <transferplugininfo.h>
#include <QStringList>

class NextcloudShareServiceStatus;
class NextcloudPluginInfo : public TransferPluginInfo
{
    Q_OBJECT

public:
    NextcloudPluginInfo();
    ~NextcloudPluginInfo();

    QList<TransferMethodInfo> info() const;
    void query();
    bool ready() const;


private Q_SLOTS:
    void serviceReady();

private:
    NextcloudShareServiceStatus *m_nextcloudShareServiceStatus;
    QList<TransferMethodInfo> m_info;
    QStringList m_capabilities;
    bool m_ready;
};

#endif // NEXTCLOUDPLUGININFO_H
