TARGET  = qhaiku
PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QHaikuExpIntegrationPlugin
load(qt_plugin)

QT += \
	widgets-private core-private gui-private \
	eventdispatcher_support-private fontdatabase_support-private \
	theme_support-private

LIBS += -lbe -lroot -ltracker -lgame

CONFIG += link_pkgconfig
PKGCONFIG += freetype2

QMAKE_USE_PRIVATE += freetype

INCLUDEPATH += ../../../3rdparty/simplecrypt/

SOURCES =   main.cpp \
            qhaikuintegration.cpp \
            qhaikuapplication.cpp \
            qhaikuglcontext.cpp \
            qhaikuwindow.cpp \
            qhaikubackingstore.cpp \
            qhaikutheme.cpp \
            qhaikusystemtrayicon.cpp \
            qhaikuclipboard.cpp \
            qhaikuplatformdialoghelpers.cpp \
            qhaikuplatformfontdatabase.cpp \
            qhaikusystemlocale.cpp \
            qhaikuview.cpp \
            qhaikuscreen.cpp \
            qhaikuservices.cpp \
            qhaikucursor.cpp \
            qhaikunativeinterface.cpp \
            ../../../3rdparty/simplecrypt/simplecrypt.cpp

HEADERS =   qhaikuintegration.h \
			qhaikuapplication.h \
			qhaikuglcontext.h \
            qhaikuwindow.h \
            qhaikubackingstore.h \
            qhaikutheme.h \
            qhaikusystemtrayicon.h \
            qhaikuclipboard.h \
            qhaikuplatformdialoghelpers.h \
            qhaikusystemlocale.h \
            qhaikuview.h \
            qhaikuscreen.h \
            qhaikuservices.h \
            qhaikucursor.h \
            qhaikunativeinterface.h

OTHER_FILES += haiku.json
