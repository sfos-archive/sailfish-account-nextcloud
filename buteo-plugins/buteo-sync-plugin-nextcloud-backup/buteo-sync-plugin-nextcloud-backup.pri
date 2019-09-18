QT -= gui
QT += network dbus systeminfo

CONFIG += link_pkgconfig console c++11
PKGCONFIG += buteosyncfw5 libsignon-qt5 accounts-qt5 mlite5
LIBS += -lssu

packagesExist(libsailfishkeyprovider) {
    PKGCONFIG += libsailfishkeyprovider
    DEFINES += USE_SAILFISHKEYPROVIDER
}

INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/nextcloudbackupclient.cpp \
    $$PWD/auth.cpp \
    $$PWD/replyparser.cpp \
    $$PWD/requestgenerator.cpp \
    $$PWD/syncer.cpp

HEADERS += \
    $$PWD/nextcloudbackupclient.h \
    $$PWD/auth_p.h \
    $$PWD/replyparser_p.h \
    $$PWD/requestgenerator_p.h \
    $$PWD/syncer_p.h

OTHER_FILES += \
    $$PWD/nextcloud-backup.xml \
    $$PWD/nextcloud.Backup.xml
