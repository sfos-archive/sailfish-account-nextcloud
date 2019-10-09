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
    $$PWD/accountauthenticator_p.h \
    $$PWD/xmlreplyparser_p.h \
    $$PWD/webdavrequestgenerator_p.h \
    $$PWD/processmutex_p.h

SOURCES += \
    $$PWD/accountauthenticator.cpp \
    $$PWD/xmlreplyparser.cpp \
    $$PWD/webdavrequestgenerator.cpp \
    $$PWD/processmutex_p.cpp

contains (DEFINES, NEXTCLOUDIMAGECACHE) {
    HEADERS += \
        $$PWD/imagecache.h \
        $$PWD/imagecache_p.h

    SOURCES += \
        $$PWD/imagecache.cpp \
        $$PWD/imagedatabase.cpp
}

contains (DEFINES, NEXTCLOUDEVENTCACHE) {
    HEADERS += \
        $$PWD/eventcache.h \
        $$PWD/eventcache_p.h

    SOURCES += \
        $$PWD/eventcache.cpp \
        $$PWD/eventdatabase.cpp
}

