TEMPLATE = lib
TARGET = $$qtLibraryTarget(nextcloudshareplugin)
CONFIG += plugin
DEPENDPATH += .

QT += network

include(../common/common.pri)

CONFIG += link_pkgconfig
PKGCONFIG += accounts-qt5 libsignon-qt5
PKGCONFIG += nemotransferengine-qt5
PKGCONFIG += libsailfishkeyprovider

HEADERS += nextcloudshareplugin.h \
           nextclouduploader.h \
           nextcloudshareservicestatus.h \
           nextcloudapi.h \
           nextcloudplugininfo.h \
           auth_p.h

SOURCES += nextcloudshareplugin.cpp \
           nextclouduploader.cpp \
           nextcloudplugininfo.cpp \
           nextcloudshareservicestatus.cpp \
           nextcloudapi.cpp \
           auth.cpp

target.path = /usr/lib/nemo-transferengine/plugins

INSTALLS += target
