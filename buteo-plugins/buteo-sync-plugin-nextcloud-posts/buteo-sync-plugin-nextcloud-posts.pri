QT -= gui
QT += systeminfo
LIBS += -lssu

DEFINES += NEXTCLOUDEVENTCACHE NEXTCLOUDWEBDAV
include($$PWD/../../common/common.pri)

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
