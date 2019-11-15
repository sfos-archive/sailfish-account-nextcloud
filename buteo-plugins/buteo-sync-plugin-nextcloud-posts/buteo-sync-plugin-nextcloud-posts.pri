QT -= gui

include($$PWD/../../common/webdav.pri)
include($$PWD/../../common/synccacheevents.pri)

CONFIG += link_pkgconfig
PKGCONFIG += mlite5

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
