QT -= gui
QT += network dbus

CONFIG += link_pkgconfig console c++11
PKGCONFIG += buteosyncfw5 libsignon-qt5 accounts-qt5

packagesExist(libsailfishkeyprovider) {
    PKGCONFIG += libsailfishkeyprovider
    DEFINES += USE_SAILFISHKEYPROVIDER
}

INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/nextcloudimagesclient.cpp

HEADERS += \
    $$PWD/nextcloudimagesclient.h

OTHER_FILES += \
    $$PWD/nextcloud-images.xml \
    $$PWD/nextcloud.Images.xml
