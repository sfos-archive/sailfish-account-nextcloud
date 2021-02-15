include($$PWD/../buteo-common/buteo-common.pri)
include($$PWD/../../common/common.pri)

CONFIG += link_pkgconfig
PKGCONFIG += accounts-qt5 buteosyncfw5

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
