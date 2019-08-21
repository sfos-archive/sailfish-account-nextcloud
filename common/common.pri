QT += sql

INCLUDEPATH += $$PWD
DEPENDPATH += .

HEADERS += \
    $$PWD/imagecache.h \
    $$PWD/imagecache_p.h \
    $$PWD/processmutex_p.h

SOURCES += \
    $$PWD/imagecache.cpp \
    $$PWD/imagedatabase.cpp \
    $$PWD/processmutex_p.cpp

