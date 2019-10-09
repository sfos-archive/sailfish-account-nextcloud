QT -= gui

packagesExist(libsailfishkeyprovider) {
    PKGCONFIG += libsailfishkeyprovider
    DEFINES += USE_SAILFISHKEYPROVIDER
}

DEFINES += NEXTCLOUDIMAGECACHE
include($$PWD/../../common/common.pri)
DEFINES -= NEXTCLOUDIMAGECACHE

INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/nextcloudimagesclient.cpp \
    $$PWD/replyparser.cpp \
    $$PWD/syncer.cpp

HEADERS += \
    $$PWD/nextcloudimagesclient.h \
    $$PWD/replyparser_p.h \
    $$PWD/syncer_p.h

OTHER_FILES += \
    $$PWD/nextcloud-images.xml \
    $$PWD/nextcloud.Images.xml
