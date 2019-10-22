QT += sql network

include($$PWD/common.pri)

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
