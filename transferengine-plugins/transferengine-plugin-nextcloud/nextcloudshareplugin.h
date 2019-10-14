/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUDSHAREPLUGIN_H
#define NEXTCLOUDSHAREPLUGIN_H

#include <QtCore/QObject>

#include <transferplugininterface.h>

class QNetworkAccessManager;
class Q_DECL_EXPORT NextcloudSharePlugin : public QObject, public TransferPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.sailfishos.transfer.plugin.nextcloud")
    Q_INTERFACES(TransferPluginInterface)

public:
    NextcloudSharePlugin();
    ~NextcloudSharePlugin();

    MediaTransferInterface * transferObject();
    TransferPluginInfo *infoObject();
    QString pluginId() const;

    bool enabled() const;

private:
    QNetworkAccessManager *m_qnam;
};

#endif // NEXTCLOUDSHAREPLUGIN_H
