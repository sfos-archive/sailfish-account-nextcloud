QMAKE_CXXFLAGS = -Wall -Werror
INCLUDEPATH += $$PWD
DEPENDPATH += .

contains (DEFINES, NEXTCLOUDWEBDAV) {
    QT += sql network dbus
    CONFIG += link_pkgconfig console c++11
    PKGCONFIG += buteosyncfw5
    DEFINES += NEXTCLOUDACCOUNTAUTH

    HEADERS += \
        $$PWD/webdavsyncer_p.h \
        $$PWD/webdavrequestgenerator_p.h \
        $$PWD/xmlreplyparser_p.h

    SOURCES += \
        $$PWD/webdavsyncer.cpp \
        $$PWD/webdavrequestgenerator.cpp \
        $$PWD/xmlreplyparser.cpp
}

contains (DEFINES, NEXTCLOUDACCOUNTAUTH) {
    CONFIG += link_pkgconfig
    PKGCONFIG += libsignon-qt5 accounts-qt5
    packagesExist(libsailfishkeyprovider) {
        PKGCONFIG += libsailfishkeyprovider
        DEFINES += USE_SAILFISHKEYPROVIDER
    }
    HEADERS += $$PWD/accountauthenticator_p.h
    SOURCES += $$PWD/accountauthenticator.cpp
}

contains (DEFINES, NEXTCLOUDIMAGECACHE) {
    QT += sql network

    HEADERS += \
        $$PWD/processmutex_p.h \
        $$PWD/synccachedatabase.h \
        $$PWD/synccachedatabase_p.h \
        $$PWD/imagecache.h \
        $$PWD/imagecache_p.h

    SOURCES += \
        $$PWD/processmutex.cpp \
        $$PWD/synccachedatabase.cpp \
        $$PWD/imagecache.cpp \
        $$PWD/imagedatabase.cpp
}

contains (DEFINES, NEXTCLOUDEVENTCACHE) {
    QT += sql network

    HEADERS += \
        $$PWD/processmutex_p.h \
        $$PWD/synccachedatabase.h \
        $$PWD/synccachedatabase_p.h \
        $$PWD/eventcache.h \
        $$PWD/eventcache_p.h

    SOURCES += \
        $$PWD/processmutex.cpp \
        $$PWD/synccachedatabase.cpp \
        $$PWD/eventcache.cpp \
        $$PWD/eventdatabase.cpp
}

