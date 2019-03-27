QT       += core gui

LIBS += -lbe -lroot -ltranslation -ltracker

CONFIG += plugin

TARGET = qhvif

TEMPLATE = lib

SOURCES += qhvifplugin.cpp \
    qhvifhandler.cpp

HEADERS += qhvifplugin.h \
    qhvifhandler.h

OTHER_FILES += qhvifplugin.json
