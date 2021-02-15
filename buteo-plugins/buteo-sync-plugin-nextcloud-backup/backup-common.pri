QT -= gui

include($$PWD/../buteo-common/buteo-common.pri)

INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/nextcloudbackupoperationclient.cpp \
    $$PWD/syncer.cpp

HEADERS += \
    $$PWD/nextcloudbackupoperationclient.h \
    $$PWD/syncer_p.h

