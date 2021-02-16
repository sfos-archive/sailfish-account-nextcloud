TARGET = nextcloud-images-client

include($$PWD/../buteo-common/buteo-common.pri)
include($$PWD/../../common/common.pri)

CONFIG += link_pkgconfig
PKGCONFIG += accounts-qt5 buteosyncfw5

INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/nextcloudimagesclient.cpp \
    $$PWD/replyparser.cpp \
    $$PWD/syncer.cpp

HEADERS += \
    $$PWD/nextcloudimagesclient.h \
    $$PWD/replyparser_p.h \
    $$PWD/syncer_p.h

OTHER_FILES += \
    $$PWD/nextcloud-images.xml \
    $$PWD/nextcloud.Images.xml

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
    DEFINES += "CLASSNAME=NextcloudImagesClient"
    DEFINES += CLASSNAME_H=\\\"nextcloudimagesclient.h\\\"
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
sync.files = nextcloud.Images.xml

client.path = /etc/buteo/profiles/client
client.files = nextcloud-images.xml

INSTALLS += target sync client
