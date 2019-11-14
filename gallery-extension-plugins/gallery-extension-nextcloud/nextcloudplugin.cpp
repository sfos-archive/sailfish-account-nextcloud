#include <QQmlExtensionPlugin>
#include <QQmlEngine>
#include <QtQml>

#include <QGuiApplication>
#include <QTranslator>
#include <QLocale>

#include "imagemodels.h"

static QObject *imagecache_api_factory(QQmlEngine *, QJSEngine *)
{
    return new NextcloudImageCache;
}

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

class JollaGalleryNextcloudPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.jolla.gallery.nextcloud")

public:
    void initializeEngine(QQmlEngine *engine, const char *uri)
    {
        Q_UNUSED(uri)
        Q_ASSERT(QLatin1String(uri) == QLatin1String("com.jolla.gallery.nextcloud"));
        AppTranslator *engineeringEnglish = new AppTranslator(engine);
        engineeringEnglish->load("gallery-extension-nextcloud_eng_en", "/usr/share/translations");
        AppTranslator *translator = new AppTranslator(engine);
        translator->load(QLocale(), "gallery-extension-nextcloud", "-", "/usr/share/translations");
    }

    virtual void registerTypes(const char *uri)
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("com.jolla.gallery.nextcloud"));

        qmlRegisterSingletonType<NextcloudImageCache>(uri, 1, 0, "NextcloudImageCache", imagecache_api_factory);
        qmlRegisterType<NextcloudUsersModel> (uri, 1, 0, "NextcloudUsersModel");
        qmlRegisterType<NextcloudAlbumsModel>(uri, 1, 0, "NextcloudAlbumsModel");
        qmlRegisterType<NextcloudPhotosModel>(uri, 1, 0, "NextcloudPhotosModel");
        qmlRegisterType<NextcloudPhotoCounter>(uri, 1, 0, "NextcloudPhotoCounter");
        qmlRegisterType<NextcloudImageDownloader>(uri, 1, 0, "NextcloudImageDownloader");
    }
};

#include "nextcloudplugin.moc"

