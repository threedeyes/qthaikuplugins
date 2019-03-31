TARGET = qtaudio_haiku

QT += multimedia-private

LIBS += -lbe -lmedia

HEADERS += haikuaudioplugin.h \
           haikuaudiodeviceinfo.h \
           haikuaudiooutput.h \
           haikuaudioringbuffer.h

SOURCES += haikuaudioplugin.cpp \
           haikuaudiodeviceinfo.cpp \
           haikuaudiooutput.cpp \
           haikuaudioringbuffer.cpp

OTHER_FILES += haiku_audio.json

PLUGIN_TYPE = audio
PLUGIN_CLASS_NAME = HaikuAudioPlugin
load(qt_plugin)
