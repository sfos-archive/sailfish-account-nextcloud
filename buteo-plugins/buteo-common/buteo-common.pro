TEMPLATE = lib

TARGET = nextcloudbuteocommon
TARGET = $$qtLibraryTarget($$TARGET)

QT -= gui
QT += network dbus
CONFIG += link_pkgconfig
PKGCONFIG += buteosyncfw5 sailfishaccounts

INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/webdavsyncer_p.h \
    $$PWD/networkrequestgenerator_p.h \
    $$PWD/networkreplyparser_p.h

SOURCES += \
    $$PWD/webdavsyncer.cpp \
    $$PWD/networkrequestgenerator.cpp \
    $$PWD/networkreplyparser.cpp

TARGETPATH = $$[QT_INSTALL_LIBS]
target.path = $$TARGETPATH

INSTALLS += target
