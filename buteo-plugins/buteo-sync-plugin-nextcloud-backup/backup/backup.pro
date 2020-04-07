TARGET = nextcloud-backup-client

include(backup.pri)

QMAKE_CXXFLAGS = -Wall -Werror

!contains (DEFINES, BUTEO_OUT_OF_PROCESS_SUPPORT) {
    TEMPLATE = lib
    CONFIG += plugin
    target.path = /usr/lib/buteo-plugins-qt5
}

contains (DEFINES, BUTEO_OUT_OF_PROCESS_SUPPORT) {
    TEMPLATE = app
    target.path = /usr/lib/buteo-plugins-qt5/oopp
    DEFINES += CLIENT_PLUGIN
    DEFINES += "CLASSNAME=NextcloudBackupClient"
    DEFINES += CLASSNAME_H=\\\"nextcloudbackupclient.h\\\"
    INCLUDE_DIR = $$system(pkg-config --cflags buteosyncfw5|cut -f2 -d'I')

    HEADERS += $$INCLUDE_DIR/ButeoPluginIfaceAdaptor.h   \
               $$INCLUDE_DIR/PluginCbImpl.h              \
               $$INCLUDE_DIR/PluginServiceObj.h

    SOURCES += $$INCLUDE_DIR/ButeoPluginIfaceAdaptor.cpp \
               $$INCLUDE_DIR/PluginCbImpl.cpp            \
               $$INCLUDE_DIR/PluginServiceObj.cpp        \
               $$INCLUDE_DIR/plugin_main.cpp
}

sync.path = /etc/buteo/profiles/sync
sync.files = nextcloud.Backup.xml

client.path = /etc/buteo/profiles/client
client.files = nextcloud-backup.xml

INSTALLS += target sync client
