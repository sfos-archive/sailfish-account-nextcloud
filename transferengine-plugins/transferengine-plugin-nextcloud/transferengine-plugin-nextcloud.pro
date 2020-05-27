TEMPLATE = lib
TARGET = $$qtLibraryTarget(nextcloudshareplugin)
CONFIG += plugin
DEPENDPATH += .

QT += network

CONFIG += link_pkgconfig
PKGCONFIG += nemotransferengine-qt5 accounts-qt5

include($$PWD/../../common/auth.pri)

HEADERS += nextcloudshareplugin.h \
           nextclouduploader.h \
           nextcloudshareservicestatus.h \
           nextcloudapi.h \
           nextcloudplugininfo.h

SOURCES += nextcloudshareplugin.cpp \
           nextclouduploader.cpp \
           nextcloudplugininfo.cpp \
           nextcloudshareservicestatus.cpp \
           nextcloudapi.cpp

target.path = /usr/lib/nemo-transferengine/plugins

INSTALLS += target
