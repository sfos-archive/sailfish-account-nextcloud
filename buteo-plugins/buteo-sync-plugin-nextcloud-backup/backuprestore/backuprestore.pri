include($$PWD/../backup-common.pri)

INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/nextcloudbackuprestoreclient.cpp

HEADERS += \
    $$PWD/nextcloudbackuprestoreclient.h

OTHER_FILES += \
    $$PWD/nextcloud-backuprestore.xml \
    $$PWD/nextcloud.BackupRestore.xml
