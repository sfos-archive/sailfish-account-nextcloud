QT -= gui

include($$PWD/../../common/webdav.pri)

INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/nextcloudbackupoperationclient.cpp \
    $$PWD/syncer.cpp

HEADERS += \
    $$PWD/nextcloudbackupoperationclient.h \
    $$PWD/syncer_p.h

