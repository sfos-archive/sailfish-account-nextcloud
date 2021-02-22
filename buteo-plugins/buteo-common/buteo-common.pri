INCLUDEPATH += $$PWD
DEPENDPATH += .

QT += dbus

CONFIG += link_pkgconfig
PKGCONFIG += accounts-qt5 buteosyncfw5 sailfishaccounts

LIBS += -L$$PWD -lnextcloudbuteocommon
