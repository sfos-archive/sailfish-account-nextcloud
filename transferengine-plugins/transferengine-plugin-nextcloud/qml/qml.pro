TEMPLATE = lib
TARGET = sailfishtransferenginenextcloudplugin
TARGET = $$qtLibraryTarget($$TARGET)

MODULENAME = Sailfish/TransferEngine/Nextcloud
TARGETPATH = $$[QT_INSTALL_QML]/$$MODULENAME

QT += qml
CONFIG += plugin

TS_FILE = $$OUT_PWD/sailfish_transferengine_plugin_nextcloud.ts
EE_QM = $$OUT_PWD/sailfish_transferengine_plugin_nextcloud_eng_en.qm

ts.commands += lupdate $$PWD/.. -ts $$TS_FILE
ts.CONFIG += no_check_exist no_link
ts.output = $$TS_FILE
ts.input = ..

ts_install.files = $$TS_FILE
ts_install.path = /usr/share/translations/source
ts_install.CONFIG += no_check_exist

# should add -markuntranslated "-" when proper translations are in place (or for testing)
engineering_english.commands += lrelease -idbased $$TS_FILE -qm $$EE_QM
engineering_english.CONFIG += no_check_exist no_link
engineering_english.depends = ts
engineering_english.input = $$TS_FILE
engineering_english.output = $$EE_QM

engineering_english_install.path = /usr/share/translations
engineering_english_install.files = $$EE_QM
engineering_english_install.CONFIG += no_check_exist

QMAKE_EXTRA_TARGETS += ts engineering_english

PRE_TARGETDEPS += ts engineering_english

INSTALLS += ts_install engineering_english_install

target.path = $$TARGETPATH

import.files = qmldir NextcloudShareFile.qml
import.path = $$TARGETPATH

qml.files = NextcloudShareDialog.qml
qml.path = /usr/share/nemo-transferengine/plugins

SOURCES += nextcloudplugin.cpp
OTHER_FILES += $$import.files $$qml.files

INSTALLS += target import qml
