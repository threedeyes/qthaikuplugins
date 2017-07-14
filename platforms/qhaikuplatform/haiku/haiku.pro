TARGET  = qhaiku
PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QHaikuExpIntegrationPlugin
load(qt_plugin)

QT += \
	widgets-private core-private gui-private \
	eventdispatcher_support-private fontdatabase_support-private \
	theme_support-private

LIBS += -lbe -lroot -ltracker

INCLUDEPATH += "/boot/system/develop/headers/freetype2"
INCLUDEPATH += "/boot/system/develop/headers/freetype2/freetype"

QMAKE_USE_PRIVATE += freetype

SOURCES =   main.cpp \
            qhaikuintegration.cpp \
            qhaikuintegration_haiku.cpp \
            qhaikuwindow.cpp \
            qhaikucommon.cpp \
            qhaikuplatformmenu.cpp \
            qhaikuplatformmenuitem.cpp \
            qhaikutheme.cpp \
            qhaikusystemsettings.cpp \
            qhaikusystemtrayicon.cpp \
            qhaikuclipboard.cpp \
            qhaikunativeiconmenuitem.cpp \
            qhaikuplatformfontdatabase.cpp \
            qhaikuplatformdialoghelpers.cpp \
            qhaikuview.cpp \
            qhaikuservices.cpp \
            qhaikucursor.cpp

HEADERS =   qhaikuintegration.h \
            qhaikuwindow.h \
            qhaikucommon.h \
            qhaikutheme.h \
            qhaikuplatformmenu.h \
            qhaikuplatformmenuitem.h \
            qhaikusystemsettings.h \
            qhaikusystemtrayicon.h \
            qhaikuclipboard.h \
            qhaikunativeiconmenuitem.h \
            qhaikuplatformfontdatabase.h \
            qhaikuplatformdialoghelpers.h \
            qhaikuview.h \
            qhaikuservices.h \
            qhaikucursor.h

OTHER_FILES += haiku.json
