TARGET = nextcloud-backuprestore-client

include($$PWD/../backup-common.pri)

INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/nextcloudbackuprestoreclient.cpp

HEADERS += \
    $$PWD/nextcloudbackuprestoreclient.h

OTHER_FILES += \
    $$PWD/nextcloud-backuprestore.xml \
    $$PWD/nextcloud.BackupRestore.xml

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
    DEFINES += "CLASSNAME=NextcloudBackupRestoreClient"
    DEFINES += CLASSNAME_H=\\\"nextcloudbackuprestoreclient.h\\\"
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
sync.files = nextcloud.BackupRestore.xml

client.path = /etc/buteo/profiles/client
client.files = nextcloud-backuprestore.xml

INSTALLS += target sync client
