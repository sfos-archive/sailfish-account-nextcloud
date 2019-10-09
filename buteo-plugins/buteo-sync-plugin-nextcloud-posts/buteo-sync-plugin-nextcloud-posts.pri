QT -= gui
QT += network dbus systeminfo

CONFIG += link_pkgconfig console c++11
PKGCONFIG += buteosyncfw5 libsignon-qt5 accounts-qt5 mlite5
LIBS += -lssu

packagesExist(libsailfishkeyprovider) {
    PKGCONFIG += libsailfishkeyprovider
    DEFINES += USE_SAILFISHKEYPROVIDER
}

DEFINES += NEXTCLOUDEVENTCACHE
include($$PWD/../../common/common.pri)
DEFINES -= NEXTCLOUDEVENTCACHE

INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/nextcloudpostsclient.cpp \
    $$PWD/syncer.cpp

HEADERS += \
    $$PWD/nextcloudpostsclient.h \
    $$PWD/syncer_p.h

OTHER_FILES += \
    $$PWD/nextcloud-posts.xml \
    $$PWD/nextcloud.Posts.xml
