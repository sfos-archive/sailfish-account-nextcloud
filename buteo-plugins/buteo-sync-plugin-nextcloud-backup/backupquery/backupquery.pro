TARGET = nextcloud-backupquery-client

include($$PWD/../backup-common.pri)

INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/nextcloudbackupqueryclient.cpp

HEADERS += \
    $$PWD/nextcloudbackupqueryclient.h

OTHER_FILES += \
    $$PWD/nextcloud-backupquery.xml \
    $$PWD/nextcloud.BackupQuery.xml

QMAKE_CXXFLAGS = -Wall -Werror

!contains (DEFINES, BUTEO_OUT_OF_PROCESS_SUPPORT) {
    TEMPLATE = lib
    CONFIG += plugin
    target.path = $$[QT_INSTALL_LIBS]/buteo-plugins-qt5
}

contains (DEFINES, BUTEO_OUT_OF_PROCESS_SUPPORT) {
    TEMPLATE = app
    target.path = $$[QT_INSTALL_LIBS]/buteo-plugins-qt5/oopp
    DEFINES += CLIENT_PLUGIN
    DEFINES += "CLASSNAME=NextcloudBackupQueryClient"
    DEFINES += CLASSNAME_H=\\\"nextcloudbackupqueryclient.h\\\"
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
sync.files = nextcloud.BackupQuery.xml

client.path = /etc/buteo/profiles/client
client.files = nextcloud-backupquery.xml

INSTALLS += target sync client
