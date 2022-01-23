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

SOURCES =	main.cpp \
			qhaikuapplication.cpp \
			qhaikubackingstore.cpp \
			qhaikuclipboard.cpp \
			qhaikucursor.cpp \
			qhaikuglcontext.cpp \
			qhaikuintegration.cpp \
			qhaikunativeinterface.cpp \
			qhaikuoffscreensurface.cpp \
			qhaikuplatformdialoghelpers.cpp \
			qhaikuplatformfontdatabase.cpp \
			qhaikuscreen.cpp \
			qhaikuservices.cpp \
			qhaikusystemlocale.cpp \
			qhaikusystemtrayicon.cpp \
			qhaikutheme.cpp \
			qhaikuview.cpp \
			qhaikuwindow.cpp \
			../../../3rdparty/simplecrypt/simplecrypt.cpp

HEADERS =	qhaikuapplication.h \
			qhaikubackingstore.h \
			qhaikuclipboard.h \
			qhaikucursor.h \
			qhaikuglcontext.h \
			qhaikuintegration.h \
			qhaikunativeinterface.h \
			qhaikuoffscreensurface.h \
			qhaikuplatformdialoghelpers.h \
			qhaikuplatformfontdatabase.h \
			qhaikuscreen.h \
			qhaikuservices.h \
			qhaikusystemlocale.h \
			qhaikusystemtrayicon.h \
			qhaikutheme.h \
			qhaikuview.h \
			qhaikuwindow.h

OTHER_FILES += haiku.json
