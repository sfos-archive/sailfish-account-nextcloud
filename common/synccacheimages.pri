QT += sql network

include($$PWD/common.pri)

HEADERS += \
    $$PWD/processmutex_p.h \
    $$PWD/synccachedatabase.h \
    $$PWD/synccachedatabase_p.h \
    $$PWD/synccacheimages.h \
    $$PWD/synccacheimages_p.h \
    $$PWD/synccacheimagedownloads_p.h

SOURCES += \
    $$PWD/processmutex.cpp \
    $$PWD/synccachedatabase.cpp \
    $$PWD/synccacheimages.cpp \
    $$PWD/synccacheimagedownloads.cpp \
    $$PWD/imagedatabase.cpp
