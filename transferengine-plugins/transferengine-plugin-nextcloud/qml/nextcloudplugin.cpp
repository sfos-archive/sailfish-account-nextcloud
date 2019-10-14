/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include <QQmlExtensionPlugin>
#include <QQmlEngine>
#include <QtQml>

#include <QGuiApplication>
#include <QTranslator>
#include <QLocale>

// using custom translator so it gets properly removed from qApp when engine is deleted
class AppTranslator: public QTranslator
{
    Q_OBJECT

public:
    AppTranslator(QObject *parent)
        : QTranslator(parent)
    {
        qApp->installTranslator(this);
    }

    virtual ~AppTranslator()
    {
        qApp->removeTranslator(this);
    }
};

class SailfishTransferEngineNextcloudPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "Sailfish.TransferEngine.Nextcloud")

public:
    void initializeEngine(QQmlEngine *engine, const char *uri)
    {
        Q_UNUSED(uri)
        Q_ASSERT(QLatin1String(uri) == QLatin1String("Sailfish.TransferEngine.Nextcloud"));
        AppTranslator *engineeringEnglish = new AppTranslator(engine);
        engineeringEnglish->load("sailfish_transferengine_plugin_nextcloud_eng_en", "/usr/share/translations");
        AppTranslator *translator = new AppTranslator(engine);
        translator->load(QLocale(), "sailfish_transferengine_plugin_nextcloud", "-", "/usr/share/translations");
    }

    virtual void registerTypes(const char *uri)
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("Sailfish.TransferEngine.Nextcloud"));
    }
};

#include "nextcloudplugin.moc"

