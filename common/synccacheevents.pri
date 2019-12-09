QT += sql network

include($$PWD/common.pri)

HEADERS += \
    $$PWD/processmutex_p.h \
    $$PWD/synccachedatabase.h \
    $$PWD/synccachedatabase_p.h \
    $$PWD/synccacheevents.h \
    $$PWD/synccacheevents_p.h \
    $$PWD/synccacheeventdownloads_p.h

SOURCES += \
    $$PWD/processmutex.cpp \
    $$PWD/synccachedatabase.cpp \
    $$PWD/synccacheevents.cpp \
    $$PWD/eventdatabase.cpp \
    $$PWD/synccacheeventdownloads.cpp
