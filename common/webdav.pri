QT += network dbus
CONFIG += link_pkgconfig
PKGCONFIG += buteosyncfw5

PKGCONFIG += libsignon-qt5 accounts-qt5 sailfishaccounts

INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/webdavsyncer_p.h \
    $$PWD/networkrequestgenerator_p.h \
    $$PWD/networkreplyparser_p.h

SOURCES += \
    $$PWD/webdavsyncer.cpp \
    $$PWD/networkrequestgenerator.cpp \
    $$PWD/networkreplyparser.cpp
