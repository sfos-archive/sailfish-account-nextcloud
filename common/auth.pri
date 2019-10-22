CONFIG += link_pkgconfig
PKGCONFIG += libsignon-qt5 accounts-qt5

packagesExist(libsailfishkeyprovider) {
    PKGCONFIG += libsailfishkeyprovider
    DEFINES += USE_SAILFISHKEYPROVIDER
}

include($$PWD/common.pri)

HEADERS += $$PWD/accountauthenticator_p.h
SOURCES += $$PWD/accountauthenticator.cpp
