TEMPLATE = lib
TARGET = sailfishtransferenginenextcloudplugin
TARGET = $$qtLibraryTarget($$TARGET)

MODULENAME = Sailfish/TransferEngine/Nextcloud
TARGETPATH = $$[QT_INSTALL_QML]/$$MODULENAME

QT += qml
CONFIG += plugin

include ($$PWD/translations/translations.pri)

target.path = $$TARGETPATH

import.files = qmldir NextcloudShareFile.qml
import.path = $$TARGETPATH

qml.files = NextcloudShareDialog.qml
qml.path = /usr/share/nemo-transferengine/plugins

SOURCES += nextcloudplugin.cpp
OTHER_FILES += import.files qml.files

INSTALLS += target import qml
