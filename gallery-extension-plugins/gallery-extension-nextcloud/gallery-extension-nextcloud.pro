TEMPLATE = lib
TARGET = jollagallerynextcloudplugin
TARGET = $$qtLibraryTarget($$TARGET)

MODULENAME = com/jolla/gallery/nextcloud
TARGETPATH = $$[QT_INSTALL_QML]/$$MODULENAME

QT += qml
CONFIG += plugin link_pkgconfig c++11
PKGCONFIG += libsignon-qt5 accounts-qt5 libsailfishkeyprovider

include($$PWD/../../common/common.pri)
include ($$PWD/translations.pri)

import.files = \
    NextcloudGalleryIcon.qml \
    NextcloudUsersPage.qml \
    NextcloudAlbumsPage.qml \
    NextcloudAlbumDelegate.qml \
    NextcloudPhotoGridPage.qml \
    NextcloudFullscreenPhotoPage.qml \
    qmldir

import.path = $$TARGETPATH
target.path = $$TARGETPATH

qml.files = NextcloudCacheMediaSource.qml
qml.path = /usr/share/jolla-gallery/mediasources/

HEADERS += imagemodels.h
SOURCES += imagemodels.cpp nextcloudplugin.cpp
OTHER_FILES += import.files qml.files

INSTALLS += target import qml
