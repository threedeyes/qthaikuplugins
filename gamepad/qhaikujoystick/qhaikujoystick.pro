TARGET = haikujoystick
QT += gamepad gamepad-private

PLUGIN_TYPE = gamepads
PLUGIN_CLASS_NAME = QHaikuJoystickBackendPlugin
load(qt_plugin)

LIBS += -lbe -ldevice
HEADERS += qhaikujoystickbackend_p.h
SOURCES += \
    qhaikujoystickbackend.cpp \
    main.cpp

OTHER_FILES += \
    qhaikujoystick.json

