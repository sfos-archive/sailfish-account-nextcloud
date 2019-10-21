TEMPLATE = lib
TARGET = jollaeventsviewnextcloudplugin
TARGET = $$qtLibraryTarget($$TARGET)

MODULENAME = com/jolla/eventsview/nextcloud
TARGETPATH = $$[QT_INSTALL_QML]/$$MODULENAME

QT += qml
CONFIG += plugin link_pkgconfig c++11
PKGCONFIG += libsignon-qt5 accounts-qt5 libsailfishkeyprovider

DEFINES += NEXTCLOUDEVENTCACHE
include($$PWD/../../common/common.pri)
DEFINES -= NEXTCLOUDEVENTCACHE

include ($$PWD/translations.pri)

qml.files = nextcloud-delegate.qml
qml.path = /usr/share/lipstick/eventfeed/

import.files = \
    NextcloudFeed.qml \
    NextcloudFeedItem.qml \
    qmldir

import.path = $$TARGETPATH
target.path = $$TARGETPATH

HEADERS += \
    eventmodel.h

SOURCES += \
    eventmodel.cpp \
    plugin.cpp

OTHER_FILES += import.files qml.files

INSTALLS += target import qml

