TARGET  = qhaikustyle
PLUGIN_TYPE = styles
PLUGIN_CLASS_NAME = QCleanlooksStylePlugin
load(qt_plugin)

QT = core gui widgets
LIBS += -lbe

HEADERS += qhaikustyle.h
SOURCES += qhaikustyle.cpp
SOURCES += plugin.cpp

include(../shared/shared.pri)

OTHER_FILES += haiku.json
