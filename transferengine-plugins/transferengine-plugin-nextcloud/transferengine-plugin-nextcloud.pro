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
           nextcloudplugininfo.h

SOURCES += nextcloudshareplugin.cpp \
           nextclouduploader.cpp \
           nextcloudplugininfo.cpp \
           nextcloudshareservicestatus.cpp \
           nextcloudapi.cpp

OTHER_FILES += *.qml

shareui.files = NextcloudShareImage.qml
shareui.path = /usr/share/nemo-transferengine/plugins

target.path = /usr/lib/nemo-transferengine/plugins

INSTALLS += target shareui

include(translations/translations.pri)
