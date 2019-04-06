TARGET = qtaudio_haiku

QT += multimedia-private

CONFIG += link_pkgconfig

PKGCONFIG += libswresample
PKGCONFIG += libavutil

LIBS += -lbe -lmedia -lswresample -lavutil

HEADERS += haikuaudioplugin.h \
           haikuaudiodeviceinfo.h \
           haikuaudiooutput.h \
           haikuaudioringbuffer.h \
           haikuaudioinput.h

SOURCES += haikuaudioplugin.cpp \
           haikuaudiodeviceinfo.cpp \
           haikuaudiooutput.cpp \
           haikuaudioringbuffer.cpp \
           haikuaudioinput.cpp

OTHER_FILES += haiku_audio.json

PLUGIN_TYPE = audio
PLUGIN_CLASS_NAME = HaikuAudioPlugin
load(qt_plugin)
