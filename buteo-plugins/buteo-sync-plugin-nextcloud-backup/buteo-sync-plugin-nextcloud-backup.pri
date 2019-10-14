DEFINES += NEXTCLOUDWEBDAV
include($$PWD/../../common/common.pri)

PKGCONFIG += sailfishaccounts
QT -= gui

INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/nextcloudbackupclient.cpp \
    $$PWD/syncer.cpp

HEADERS += \
    $$PWD/nextcloudbackupclient.h \
    $$PWD/syncer_p.h

OTHER_FILES += \
    $$PWD/nextcloud-backup.xml \
    $$PWD/nextcloud.Backup.xml
