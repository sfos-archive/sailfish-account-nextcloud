QT -= gui

include($$PWD/../../common/webdav.pri)
include($$PWD/../../common/eventcache.pri)

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
