TEMPLATE = lib
TARGET = $$qtLibraryTarget(nextcloudshareplugin)
CONFIG += plugin
DEPENDPATH += .

QT += network

CONFIG += link_pkgconfig
PKGCONFIG += nemotransferengine-qt5 accounts-qt5 sailfishaccounts

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

target.path = $$[QT_INSTALL_LIBS]/nemo-transferengine/plugins

OTHER_FILES += *.qml

shareui.files = NextcloudShareFile.qml
shareui.path = /usr/share/nemo-transferengine/plugins

INSTALLS += target shareui
