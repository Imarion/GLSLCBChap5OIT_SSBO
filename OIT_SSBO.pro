QT += gui core

CONFIG += c++11

TARGET = HDRToneMap
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    OIT_SSBO.cpp \
    vbosphere.cpp \
    vbocube.cpp

HEADERS += \
    OIT_SSBO.h \
    vbosphere.h \
    vbocube.h

OTHER_FILES += \
    fshader.txt \
    vshader.txt

RESOURCES += \
    shaders.qrc

DISTFILES += \
    fshader.txt \
    vshader.txt
