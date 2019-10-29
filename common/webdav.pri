QT += network dbus
CONFIG += link_pkgconfig
PKGCONFIG += buteosyncfw5

include($$PWD/common.pri)
include($$PWD/auth.pri)

HEADERS += \
    $$PWD/webdavsyncer_p.h \
    $$PWD/networkrequestgenerator_p.h \
    $$PWD/networkreplyparser_p.h

SOURCES += \
    $$PWD/webdavsyncer.cpp \
    $$PWD/networkrequestgenerator.cpp \
    $$PWD/networkreplyparser.cpp
