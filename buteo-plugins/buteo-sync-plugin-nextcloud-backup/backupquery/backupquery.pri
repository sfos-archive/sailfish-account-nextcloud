include($$PWD/../backup-common.pri)

INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/nextcloudbackupqueryclient.cpp

HEADERS += \
    $$PWD/nextcloudbackupqueryclient.h

OTHER_FILES += \
    $$PWD/nextcloud-backupquery.xml \
    $$PWD/nextcloud.BackupQuery.xml
