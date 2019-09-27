QT += sql network dbus

INCLUDEPATH += $$PWD
DEPENDPATH += .

CONFIG += link_pkgconfig console c++11
PKGCONFIG += buteosyncfw5 libsignon-qt5 accounts-qt5 sailfishaccounts

packagesExist(libsailfishkeyprovider) {
    PKGCONFIG += libsailfishkeyprovider
    DEFINES += USE_SAILFISHKEYPROVIDER
}

HEADERS += \
    $$PWD/imagecache.h \
    $$PWD/imagecache_p.h \
    $$PWD/processmutex_p.h \
    $$PWD/accountauthenticator_p.h \
    $$PWD/xmlreplyparser_p.h \
    $$PWD/webdavrequestgenerator_p.h

SOURCES += \
    $$PWD/imagecache.cpp \
    $$PWD/imagedatabase.cpp \
    $$PWD/processmutex_p.cpp \
    $$PWD/accountauthenticator.cpp \
    $$PWD/xmlreplyparser.cpp \
    $$PWD/webdavrequestgenerator.cpp

