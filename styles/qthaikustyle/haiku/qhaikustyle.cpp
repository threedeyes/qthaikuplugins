/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2017-2020 Gerasim Troeglazov,
** Contact: 3dEyes@gmail.com
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qhaikustyle.h"

#include <qcombobox.h>
#include <qpushbutton.h>
#include <qpainter.h>
#include <qdir.h>
#include <qhash.h>
#include <qstyleoption.h>
#include <qapplication.h>
#include <qmainwindow.h>
#include <qwindow.h>
#include <qfont.h>
#include <qgroupbox.h>
#include <qprocess.h>
#include <qpixmapcache.h>
#include <qdialogbuttonbox.h>
#include <qpa/qplatformfontdatabase.h>
#include <qpa/qplatformtheme.h>
#include <qscrollbar.h>
#include <qspinbox.h>
#include <qslider.h>
#include <qsplitter.h>
#include <qprogressbar.h>
#include <qtoolbar.h>
#include <qwizard.h>
#include <qlibrary.h>
#include <qstylefactory.h>
#include <qmdisubwindow.h>
#include <qlabel.h>
#include <qdial.h>
#include <qsizegrip.h>
#include <qmenu.h>

#include <QDebug>

#include <AppKit.h>
#include <StorageKit.h>
#include <InterfaceKit.h>
#include <NodeInfo.h>
#include <Bitmap.h>
#include <View.h>
#include <Window.h>
#include <ControlLook.h>
#include <ScrollBar.h>
#include <Bitmap.h>
#include <IconUtils.h>

#include "../../../platforms/qhaikuplatform/haiku/qhaikuwindow.h"
#include "../../../platforms/qhaikuplatform/haiku/qhaikusettings.h"

#include "qstylehelper_p.h"

#define _QS_HAIKU_TAB_FIX_WIDTH_

QT_BEGIN_NAMESPACE

using namespace QStyleHelper;

enum Direction {
	TopDown,
	FromLeft,
	BottomUp,
	FromRight
};

typedef enum {
	ARROW_LEFT = 0,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	ARROW_NONE
} arrow_direction;

// Haiku BBitmap surface
class TemporarySurface
{
public:
	TemporarySurface(const BRect& bounds, QImage::Format format = QImage::Format_ARGB32)
		: mBitmap(bounds, B_RGBA32, true)
		, mView(bounds, "Qt temporary surface", B_FOLLOW_ALL, B_WILL_DRAW)
		, mImage(reinterpret_cast<const uchar*>(mBitmap.Bits()),
			bounds.IntegerWidth() + 1, bounds.IntegerHeight() + 1,
			mBitmap.BytesPerRow(), format)
	{
		mBitmap.Lock();
		mBitmap.AddChild(&mView);
	}

	~TemporarySurface()
	{
		mBitmap.RemoveChild(&mView);
		mBitmap.Unlock();
	}

	BView* view()
	{
		return &mView;
	}

	QImage& image()
	{
		if(mView.Window())
			mView.Sync();
		return mImage;
	}

private:
	BBitmap		mBitmap;
	BView		mView;
	QImage		mImage;
};

// convert Haiku rgb_color to QColor
static QColor mkQColor(rgb_color rgb)
{
	return QColor(rgb.red, rgb.green, rgb.blue);
}

static rgb_color mkHaikuColor(QColor color)
{
	rgb_color hcolor;

	hcolor.red = color.red();
	hcolor.green = color.green();
	hcolor.blue = color.blue();
	hcolor.alpha = color.alpha();

	return hcolor;
}

// from windows style
static const int windowsItemFrame        =  2; // menu item frame width
static const int windowsItemHMargin      =  3; // menu item hor text margin
static const int windowsItemVMargin      =  8; // menu item ver text margin
static const int windowsRightBorder      = 15; // right border on windows
static const int progressAnimationFps    = 25;
static const int mdiTabWidthMin			 = 100; //minimal tab size for mid window
static const int mdiTabWidthFix			 = 220; //tab size for mid window (fixed size mode _QS_HAIKU_TAB_FIX_WIDTH_)
static const int mdiTabTextMarginLeft	 =  32;
static const int mdiTabTextMarginRight	 =  24;


/* XPM */
static const char * const dock_widget_close_xpm[] = {
	"11 13 7 1",
	" 	c None",
	".	c #D5CFCB",
	"+	c #8F8B88",
	"@	c #6C6A67",
	"#	c #ABA6A3",
	"$	c #B5B0AC",
	"%	c #A4A09D",
	"           ",
	" +@@@@@@@+ ",
	"+#       #+",
	"@ $@   @$ @",
	"@ @@@ @@@ @",
	"@  @@@@@  @",
	"@   @@@   @",
	"@  @@@@@  @",
	"@ @@@ @@@ @",
	"@ $@   @$ @",
	"+%       #+",
	" +@@@@@@@+ ",
	"           "};

static const char * const qt_haiku_arrow_down_xpm[] = {
	"11 7 2 1",
	" 	c None",
	"x	c #000000",
	"           ",
	"           ",
	"  xxxxxxx  ",
	"   xxxxx   ",
	"    xxx    ",
	"     x     ",
	"           "};

static const char * const qt_haiku_arrow_up_xpm[] = {
	"11 7 2 1",
	" 	c None",
	"x	c #000000",
	"           ",
	"     x     ",
	"    xxx    ",
	"   xxxxx   ",
	"  xxxxxxx  ",
	"           ",
	"           "};

static const char * const dock_widget_restore_xpm[] = {
	"11 13 7 1",
	" 	c None",
	".	c #D5CFCB",
	"+	c #8F8B88",
	"@	c #6C6A67",
	"#	c #ABA6A3",
	"$	c #B5B0AC",
	"%	c #A4A09D",
	"           ",
	" +@@@@@@@+ ",
	"+#       #+",
	"@   #@@@# @",
	"@   @   @ @",
	"@ #@@@# @ @",
	"@ @   @ @ @",
	"@ @   @@@ @",
	"@ @   @   @",
	"@ #@@@#   @",
	"+%       #+",
	" +@@@@@@@+ ",
	"           "};

static const char * const workspace_minimize[] = {
	"11 13 7 1",
	" 	c None",
	".	c #D5CFCB",
	"+	c #8F8B88",
	"@	c #6C6A67",
	"#	c #ABA6A3",
	"$	c #B5B0AC",
	"%	c #A4A09D",
	"           ",
	" +@@@@@@@+ ",
	"+#       #+",
	"@         @",
	"@         @",
	"@         @",
	"@ @@@@@@@ @",
	"@ @@@@@@@ @",
	"@         @",
	"@         @",
	"+%       #+",
	" +@@@@@@@+ ",
	"           "};


static const unsigned char kShiftBits[] = {
	0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x14,
	0x1d,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x17,0x04,0x04,0x17,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x17,0x17,0x04,0x1a,0x04,0x1a,0x04,0x1a,0x04,0x1a,0x04,0x04,0x04,0x1a,0x04,0x04,0x04,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x17,0x04,0x04,0x1a,0x1a,0x04,0x1a,0x04,0x1a,0x04,0x1a,0x04,0x1a,0x1a,0x1a,0x1a,0x04,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x1a,0x1a,0x1a,0x04,0x1a,0x04,0x04,0x04,0x1a,0x04,0x1a,0x04,0x04,0x1a,0x1a,0x1a,0x04,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x17,0x17,0x04,0x1a,0x04,0x1a,0x04,0x1a,0x04,0x1a,0x04,0x1a,0x1a,0x1a,0x1a,0x04,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x17,0x04,0x04,0x17,0x1a,0x04,0x1a,0x04,0x1a,0x04,0x1a,0x04,0x1a,0x1a,0x1a,0x1a,0x04,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x14,
	0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14
};


static const unsigned char kCtrlBits[] = {
	0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x14,
	0x1d,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x13,0x04,0x04,0x13,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x1a,0x1a,0x04,0x1a,0x04,0x04,0x04,0x1a,0x04,0x04,0x04,0x1a,0x04,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x1a,0x1a,0x1a,0x1a,0x1a,0x04,0x1a,0x1a,0x04,0x1a,0x04,0x1a,0x04,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x1a,0x1a,0x1a,0x1a,0x1a,0x04,0x1a,0x1a,0x04,0x04,0x04,0x1a,0x04,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x1a,0x1a,0x04,0x1a,0x1a,0x04,0x1a,0x1a,0x04,0x04,0x1a,0x1a,0x04,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x13,0x04,0x04,0x13,0x1a,0x1a,0x04,0x1a,0x1a,0x04,0x13,0x04,0x1a,0x04,0x04,0x04,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x14,
	0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14
};


static const unsigned char kOptBits[] = {
	0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x14,
	0x1d,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x13,0x04,0x04,0x13,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x1a,0x1a,0x04,0x1a,0x04,0x04,0x1a,0x04,0x04,0x04,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x1a,0x1a,0x04,0x1a,0x04,0x1a,0x04,0x1a,0x04,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x1a,0x1a,0x04,0x1a,0x04,0x04,0x1a,0x1a,0x04,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x1a,0x1a,0x04,0x1a,0x04,0x1a,0x1a,0x1a,0x04,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x13,0x04,0x04,0x13,0x1a,0x04,0x1a,0x1a,0x1a,0x04,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x14,
	0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14
};


static const unsigned char kAltBits[] = {
	0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x14,
	0x1d,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x13,0x04,0x04,0x13,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x1a,0x1a,0x04,0x1a,0x04,0x1a,0x1a,0x04,0x04,0x04,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x1a,0x1a,0x04,0x1a,0x04,0x1a,0x1a,0x1a,0x04,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x04,0x04,0x04,0x1a,0x04,0x1a,0x1a,0x1a,0x04,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x1a,0x1a,0x04,0x1a,0x04,0x1a,0x1a,0x1a,0x04,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x1a,0x1a,0x04,0x1a,0x04,0x04,0x04,0x1a,0x04,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x14,
	0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14
};


static const unsigned char kMenuBits[] = {
	0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x14,
	0x1d,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x04,0x1a,0x04,0x04,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x04,0x04,0x04,0x04,0x1a,0x04,0x04,0x1a,0x04,0x1a,0x1a,0x04,0x1a,0x04,0x1a,0x04,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x1a,0x04,0x1a,0x04,0x1a,0x04,0x1a,0x1a,0x04,0x04,0x1a,0x04,0x1a,0x04,0x1a,0x04,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x1a,0x1a,0x1a,0x04,0x1a,0x04,0x04,0x1a,0x04,0x04,0x04,0x04,0x1a,0x04,0x1a,0x04,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x1a,0x1a,0x1a,0x04,0x1a,0x04,0x1a,0x1a,0x04,0x1a,0x04,0x04,0x1a,0x04,0x1a,0x04,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x04,0x1a,0x1a,0x1a,0x04,0x1a,0x04,0x04,0x1a,0x04,0x1a,0x1a,0x04,0x1a,0x04,0x04,0x04,0x1a,0x17,0x14,
	0x1d,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x17,0x14,
	0x1d,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x14,
	0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14
};

static QImage get_haiku_alert_icon(uint32 fType, int32 iconSize)
{
	QImage image;

	if (fType == B_EMPTY_ALERT)
		return image;

	BBitmap* icon = NULL;
	BPath path;
	status_t status = find_directory(B_BEOS_SERVERS_DIRECTORY, &path);
	if (status != B_OK)
		return image;

	path.Append("app_server");
	BFile file;
	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status != B_OK)
		return image;

	BResources resources;
	status = resources.SetTo(&file);
	if (status != B_OK)
		return image;

	const char* iconName;
	switch (fType) {
		case B_INFO_ALERT:
			iconName = "info";
			break;
		case B_IDEA_ALERT:
			iconName = "idea";
			break;
		case B_WARNING_ALERT:
			iconName = "warn";
			break;
		case B_STOP_ALERT:
			iconName = "stop";
			break;
		default:
			return image;
	}

	icon = new(std::nothrow) BBitmap(BRect(0, 0, iconSize - 1, iconSize - 1), 0, B_RGBA32);
	if (icon == NULL || icon->InitCheck() < B_OK) {
		delete icon;
		return image;
	}

	size_t size = 0;
	const uint8* rawIcon;

	rawIcon = (const uint8*)resources.LoadResource(B_VECTOR_ICON_TYPE, iconName, &size);
	if (rawIcon != NULL	&& BIconUtils::GetVectorIcon(rawIcon, size, icon) == B_OK) {
		image = QImage((unsigned char *) icon->Bits(),
			icon->Bounds().Width() + 1, icon->Bounds().Height() + 1, QImage::Format_ARGB32);
		delete icon;
		return image;
	}

	rawIcon = (const uint8*)resources.LoadResource(B_LARGE_ICON_TYPE, iconName, &size);
	if (rawIcon == NULL) {
		delete icon;
		return image;
	}

	if (icon->ColorSpace() != B_CMAP8)
		BIconUtils::ConvertFromCMAP8(rawIcon, iconSize, iconSize, iconSize, icon);

	image = QImage((unsigned char *) icon->Bits(), icon->Bounds().Width() + 1,
		icon->Bounds().Height() + 1, QImage::Format_ARGB32);
	delete icon;
	return image;
}

static void qt_haiku_draw_windows_frame(QPainter *painter, const QRect &qrect, color_which bcolor, uint32 borders = BControlLook::B_ALL_BORDERS, bool sizer = true)
{
	QColor frameColorActive(mkQColor(ui_color(bcolor)));
	QColor bevelShadow1(mkQColor(tint_color(ui_color(bcolor), 1.07)));
	QColor bevelShadow2(mkQColor(tint_color(ui_color(bcolor), B_DARKEN_2_TINT)));
	QColor bevelShadow3(mkQColor(tint_color(ui_color(bcolor), B_DARKEN_3_TINT)));
	QColor bevelLight(mkQColor(tint_color(ui_color(bcolor), B_LIGHTEN_2_TINT)));

	QRect rect= qrect;
	painter->setPen(bevelShadow2);
	if ((borders & BControlLook::B_LEFT_BORDER) != 0)
		painter->drawLine(rect.topLeft(), rect.bottomLeft());
	if ((borders & BControlLook::B_TOP_BORDER) != 0)
		painter->drawLine(rect.topLeft(), rect.topRight());
	painter->setPen(bevelShadow3);
	if ((borders & BControlLook::B_RIGHT_BORDER) != 0)
		painter->drawLine(rect.topRight(), rect.bottomRight());
	if ((borders & BControlLook::B_BOTTOM_BORDER) != 0)
		painter->drawLine(rect.bottomRight(), rect.bottomLeft());
	rect.adjust(1,1,-1,-1);
	painter->setPen(bevelLight);
	if ((borders & BControlLook::B_LEFT_BORDER) != 0)
		painter->drawLine(rect.topLeft(), rect.bottomLeft());
	if ((borders & BControlLook::B_TOP_BORDER) != 0)
		painter->drawLine(rect.topLeft(), rect.topRight());
	painter->setPen(bevelShadow1);
	if ((borders & BControlLook::B_RIGHT_BORDER) != 0)
		painter->drawLine(rect.topRight(), rect.bottomRight());
	if ((borders & BControlLook::B_BOTTOM_BORDER) != 0)
		painter->drawLine(rect.bottomRight(), rect.bottomLeft());
	rect.adjust(1,1,-1,-1);
	painter->setPen(frameColorActive);
	if ((borders & BControlLook::B_LEFT_BORDER) != 0)
		painter->drawLine(rect.topLeft(), rect.bottomLeft());
	if ((borders & BControlLook::B_TOP_BORDER) != 0)
		painter->drawLine(rect.topLeft(), rect.topRight());
	if ((borders & BControlLook::B_RIGHT_BORDER) != 0)
		painter->drawLine(rect.topRight(), rect.bottomRight());
	if ((borders & BControlLook::B_BOTTOM_BORDER) != 0)
		painter->drawLine(rect.bottomRight(), rect.bottomLeft());
	rect.adjust(1,1,-1,-1);
	painter->setPen(bevelShadow1);
	if ((borders & BControlLook::B_LEFT_BORDER) != 0)
		painter->drawLine(rect.topLeft(), rect.bottomLeft());
	if ((borders & BControlLook::B_TOP_BORDER) != 0)
		painter->drawLine(rect.topLeft(), rect.topRight());
	painter->setPen(bevelLight);
	if ((borders & BControlLook::B_RIGHT_BORDER) != 0)
		painter->drawLine(rect.topRight(), rect.bottomRight());
	if ((borders & BControlLook::B_BOTTOM_BORDER) != 0)
		painter->drawLine(rect.bottomRight(), rect.bottomLeft());
	rect.adjust(1,1,-1,-1);
	painter->setPen(bevelShadow2);
	if ((borders & BControlLook::B_LEFT_BORDER) != 0)
		painter->drawLine(rect.topLeft(), rect.bottomLeft());
	if ((borders & BControlLook::B_TOP_BORDER) != 0)
		painter->drawLine(rect.topLeft(), rect.topRight());
	if ((borders & BControlLook::B_RIGHT_BORDER) != 0)
		painter->drawLine(rect.topRight(), rect.bottomRight());
	if ((borders & BControlLook::B_BOTTOM_BORDER) != 0)
		painter->drawLine(rect.bottomRight(), rect.bottomLeft());
	if( (borders & BControlLook::B_RIGHT_BORDER) != 0 &&
		(borders & BControlLook::B_BOTTOM_BORDER) != 0 && sizer) {
		painter->setPen(bevelShadow2);
		painter->drawLine(rect.bottomRight() + QPoint(0, -18), rect.bottomRight() + QPoint(5,-18));
		painter->drawLine(rect.bottomRight() + QPoint(-18, 0), rect.bottomRight() + QPoint(-18, 5));
	}
}

static void qt_haiku_draw_scroll_arrow(BView *view, int32 direction, BRect rect,
	const BRect& updateRect, bool enabled, bool down, orientation orient)
{
	uint32 flags = 0;
	if (!enabled)
		flags |= BControlLook::B_DISABLED;

	if (down)
		flags |= BControlLook::B_ACTIVATED;

	rgb_color baseColor = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_LIGHTEN_1_TINT);

	be_control_look->DrawButtonBackground(view, rect, updateRect, baseColor,
		flags, BControlLook::B_ALL_BORDERS, orient);

	rect.InsetBy(-0.5f, -0.5f);
	be_control_look->DrawArrowShape(view, rect, updateRect,
		baseColor, direction, flags, B_DARKEN_MAX_TINT);
}

static void qt_haiku_draw_disabled_background(BView *view, BRect area, const rgb_color& light,
	const rgb_color& dark, const rgb_color& fill, orientation orient )
{
	if (!area.IsValid())
		return;

	if (orient == B_VERTICAL) {
		int32 height = area.IntegerHeight();
		if (height == 0) {
			view->SetHighColor(dark);
			view->StrokeLine(area.LeftTop(), area.RightTop());
		} else if (height == 1) {
			view->SetHighColor(dark);
			view->FillRect(area);
		} else {
			view->BeginLineArray(4);
				view->AddLine(BPoint(area.left, area.top),
						BPoint(area.right, area.top), dark);
				view->AddLine(BPoint(area.left, area.bottom - 1),
						BPoint(area.left, area.top + 1), light);
				view->AddLine(BPoint(area.left + 1, area.top + 1),
						BPoint(area.right, area.top + 1), light);
				view->AddLine(BPoint(area.right, area.bottom),
						BPoint(area.left, area.bottom), dark);
			view->EndLineArray();
			area.left++;
			area.top += 2;
			area.bottom--;
			if (area.IsValid()) {
				view->SetHighColor(fill);
				view->FillRect(area);
			}
		}
	} else {
		int32 width = area.IntegerWidth();
		if (width == 0) {
			view->SetHighColor(dark);
			view->StrokeLine(area.LeftBottom(), area.LeftTop());
		} else if (width == 1) {
			view->SetHighColor(dark);
			view->FillRect(area);
		} else {
			view->BeginLineArray(4);
				view->AddLine(BPoint(area.left, area.bottom),
						BPoint(area.left, area.top), dark);
				view->AddLine(BPoint(area.left + 1, area.bottom),
						BPoint(area.left + 1, area.top + 1), light);
				view->AddLine(BPoint(area.left + 1, area.top),
						BPoint(area.right - 1, area.top), light);
				view->AddLine(BPoint(area.right, area.top),
						BPoint(area.right, area.bottom), dark);
			view->EndLineArray();
			area.left += 2;
			area.top ++;
			area.right--;
			if (area.IsValid()) {
				view->SetHighColor(fill);
				view->FillRect(area);
			}
		}
	}
}

/*!
	Constructs a QHaikuStyle object.
*/
QHaikuStyle::QHaikuStyle() : QProxyStyle(QStyleFactory::create(QLatin1String("Fusion"))), animateStep(0), animateTimer(0)
{
	setObjectName(QLatin1String("Haiku"));
	QSettings settings(QT_SETTINGS_FILENAME, QSettings::NativeFormat);
	settings.beginGroup("Style");
	smallIconsSizeSettings = settings.value("icons_small_size", 16).toInt();
	largeIconsSizeSettings = settings.value("icons_large_size", 32).toInt();
	toolbarIconsSizeSettings = settings.value("icons_toolbar_icon_size", 24).toInt();
	toolbarIconMode = settings.value("icons_toolbar_mode", 0).toInt();
	showMenuIcon = settings.value("icons_menu_icons", true).toBool();
	settings.endGroup();

	sMenuItemShift = new BBitmap(BRect(0.0f, 0.0f, 23.0f, 10.0f), B_CMAP8);
	sMenuItemControl = new BBitmap(BRect(0.0f, 0.0f, 21.0f, 10.0f), B_CMAP8);
	sMenuItemOption = new BBitmap(BRect(0.0f, 0.0f, 16.0f, 10.0f), B_CMAP8);
	sMenuItemAlt = new BBitmap(BRect(0.0f, 0.0f, 16.0f, 10.0f), B_CMAP8);
	sMenuItemMenu = new BBitmap(BRect(0.0f, 0.0f, 22.0f, 10.0f), B_CMAP8);

	sMenuItemShift->ImportBits(kShiftBits, sizeof(kShiftBits), 24, 0, B_CMAP8);
	sMenuItemControl->ImportBits(kCtrlBits, sizeof(kCtrlBits), 22, 0, B_CMAP8);
	sMenuItemOption->ImportBits(kOptBits, sizeof(kOptBits), 17, 0, B_CMAP8);
	sMenuItemAlt->ImportBits(kAltBits, sizeof(kAltBits), 17, 0, B_CMAP8);
	sMenuItemMenu->ImportBits(kMenuBits, sizeof(kMenuBits), 23, 0, B_CMAP8);
}

/*!
	Destroys the QHaikuStyle object.
*/
QHaikuStyle::~QHaikuStyle()
{
	delete sMenuItemShift;
	delete sMenuItemControl;
	delete sMenuItemOption;
	delete sMenuItemAlt;
	delete sMenuItemMenu;
}

/*!
	\fn void QHaikuStyle::drawItemText(QPainter *painter, const QRect &rectangle, int alignment, const QPalette &palette,
									bool enabled, const QString& text, QPalette::ColorRole textRole) const

	Draws the given \a text in the specified \a rectangle using the
	provided \a painter and \a palette.

	Text is drawn using the painter's pen. If an explicit \a textRole
	is specified, then the text is drawn using the \a palette's color
	for the specified role.  The \a enabled value indicates whether or
	not the item is enabled; when reimplementing, this value should
	influence how the item is drawn.

	The text is aligned and wrapped according to the specified \a
	alignment.

	\sa Qt::Alignment
*/
void QHaikuStyle::drawItemText(QPainter *painter, const QRect &rect, int alignment, const QPalette &pal,
									bool enabled, const QString& text, QPalette::ColorRole textRole) const
{
	if (text.isEmpty())
		return;

	QPen savedPen = painter->pen();
	if (textRole != QPalette::NoRole) {
		painter->setPen(QPen(pal.brush(textRole), savedPen.widthF()));
	}
	if (!enabled) {
		QPen pen = painter->pen();
		painter->setPen(pen);
	}
	painter->drawText(rect, alignment, text);
	painter->setPen(savedPen);
}

static QColor mergedColors(const QColor &colorA, const QColor &colorB, int factor = 50)
{
	const int maxFactor = 100;
	QColor tmp = colorA;
	tmp.setRed((tmp.red() * factor) / maxFactor + (colorB.red() * (maxFactor - factor)) / maxFactor);
	tmp.setGreen((tmp.green() * factor) / maxFactor + (colorB.green() * (maxFactor - factor)) / maxFactor);
	tmp.setBlue((tmp.blue() * factor) / maxFactor + (colorB.blue() * (maxFactor - factor)) / maxFactor);
	return tmp;
}

/*!
	\reimp
*/
void QHaikuStyle::drawPrimitive(PrimitiveElement elem,
						const QStyleOption *option,
						QPainter *painter, const QWidget *widget) const
{
	Q_ASSERT(option);
	QRect rect = option->rect;
	int state = option->state;
	QColor button = option->palette.button().color();
	QColor buttonShadow = option->palette.button().color().darker(110);
	QColor buttonShadowAlpha = buttonShadow;
	buttonShadowAlpha.setAlpha(128);
	QColor darkOutline;
	QColor dark;
	darkOutline.setHsv(button.hue(),
				qMin(255, (int)(button.saturation()*3.0)),
				qMin(255, (int)(button.value()*0.6)));
	dark.setHsv(button.hue(),
				qMin(255, (int)(button.saturation()*1.9)),
				qMin(255, (int)(button.value()*0.7)));

	switch (elem) {
	case PE_IndicatorItemViewItemCheck:
		{
			QStyleOptionButton button;
			button.QStyleOption::operator=(*option);
			button.state &= ~State_MouseOver;
			proxy()->drawPrimitive(PE_IndicatorCheckBox, &button, painter, widget);
		}
		return;
	case PE_IndicatorHeaderArrow:
		if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
			QRect r = header->rect;
			QImage arrow;
			if (header->sortIndicator & QStyleOptionHeader::SortUp)
				arrow = QImage(qt_haiku_arrow_up_xpm);
			else if (header->sortIndicator & QStyleOptionHeader::SortDown)
				arrow = QImage(qt_haiku_arrow_down_xpm);
			if (!arrow.isNull()) {
				r.setSize(arrow.size());
				r.moveCenter(header->rect.center());
				arrow.setColor(1, header->palette.dark().color().rgba());
				painter->drawImage(r, arrow);
			}
		}
		break;
	case PE_FrameTabBarBase:
		if (const QStyleOptionTabBarBase *tbb
				= qstyleoption_cast<const QStyleOptionTabBarBase *>(option)) {
			QRect tabRect = tbb->rect;
			painter->save();
			rgb_color base = mkHaikuColor(option->palette.color( QPalette::Normal, QPalette::Window));
			QColor backgroundColor(option->palette.color( QPalette::Normal, QPalette::Window));
			QColor frameColor(mkQColor(tint_color(base, 1.30)));
			painter->setPen(frameColor);

			switch (tbb->shape) {
				case QTabBar::RoundedNorth:
				case QTabBar::TriangularNorth:
					if (be_control_look != NULL) {
						if (tabRect.height() <= 1)
							tabRect.setTop(0);
						else
	            			tabRect.moveTop(0);

	            		QRect rect = tabRect.adjusted(-PM_TabBarTabHSpace, 0, PM_TabBarTabHSpace, 0);
	            		BRect bRect(0.0f, 0.0f, rect.width() - 1, rect.height() - 1);
	            		BRect bRect1(bRect.left, bRect.top,	bRect.right, bRect.bottom + 1);
	            		TemporarySurface surface(bRect);
	            		be_control_look->DrawInactiveTab(surface.view(), bRect1, bRect, base, 0, BControlLook::B_ALL_BORDERS, BControlLook::B_TOP_BORDER);
	            		painter->drawImage(rect, surface.image());
	            	}
	                break;
	            case QTabBar::RoundedSouth:
	            case QTabBar::TriangularSouth:
	            	if (be_control_look != NULL) {
	            		if (tabRect.height() <= 1)
							tabRect.setTop(0);
						else
	            			tabRect.moveTop(0);

	            		QRect rect = tabRect.adjusted(-PM_TabBarTabHSpace, 0, PM_TabBarTabHSpace, 0);
	            		BRect bRect(0.0f, 0.0f, rect.width() - 1, rect.height() - 1);
	            		BRect bRect1(bRect.left, bRect.top - 1,	bRect.right, bRect.bottom);
	            		TemporarySurface surface(bRect);
	            		be_control_look->DrawInactiveTab(surface.view(), bRect1, bRect, base, 0, BControlLook::B_ALL_BORDERS, BControlLook::B_BOTTOM_BORDER);
	            		painter->drawImage(rect, surface.image());
	            	}
	                break;
	            case QTabBar::RoundedWest:
	            case QTabBar::TriangularWest:
	                painter->drawLine(tabRect.left(), tabRect.top(), tabRect.left(), tabRect.bottom());
	                break;

	            case QTabBar::RoundedEast:
	            case QTabBar::TriangularEast:
	                painter->drawLine(tabRect.topRight(), tabRect.bottomRight());
	                break;
	        }
			painter->restore();
		}
		return;
	case PE_IndicatorButtonDropDown:
		proxy()->drawPrimitive(PE_PanelButtonCommand, option, painter, widget);
		break;
	case PE_IndicatorToolBarSeparator:
		{
			QRect rect = option->rect;
			const int margin = 6;
			if (option->state & State_Horizontal) {
				const int offset = rect.width()/2;
				painter->setPen(QPen(option->palette.window().color().darker(110)));
				painter->drawLine(rect.bottomLeft().x() + offset,
							rect.bottomLeft().y() - margin,
							rect.topLeft().x() + offset,
							rect.topLeft().y() + margin);
				painter->setPen(QPen(option->palette.window().color().lighter(110)));
				painter->drawLine(rect.bottomLeft().x() + offset + 1,
							rect.bottomLeft().y() - margin,
							rect.topLeft().x() + offset + 1,
							rect.topLeft().y() + margin);
			} else { //Draw vertical separator
				const int offset = rect.height()/2;
				painter->setPen(QPen(option->palette.window().color().darker(110)));
				painter->drawLine(rect.topLeft().x() + margin ,
							rect.topLeft().y() + offset,
							rect.topRight().x() - margin,
							rect.topRight().y() + offset);
				painter->setPen(QPen(option->palette.window().color().lighter(110)));
				painter->drawLine(rect.topLeft().x() + margin ,
							rect.topLeft().y() + offset + 1,
							rect.topRight().x() - margin,
							rect.topRight().y() + offset + 1);
			}
		}
		break;
	case PE_Frame:
		painter->save();
		painter->setPen(dark.lighter(108));
		painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
		painter->restore();
		break;
	case PE_FrameGroupBox:
		painter->save();
	    {
	        QColor backgroundColor(mkQColor(ui_color(B_PANEL_BACKGROUND_COLOR)));
	        QColor frameColor(mkQColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), 1.30)));
	        QColor bevelLight(mkQColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), 0.8)));
	        QColor bevelShadow(mkQColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), 1.03)));

	        QRect frame = option->rect;
	        painter->setPen(bevelShadow);
	        painter->drawLine(frame.topLeft(), frame.bottomLeft());
	        painter->drawLine(frame.topLeft(), frame.topRight());
	        painter->setPen(bevelLight);
	        painter->drawLine(frame.topRight(), frame.bottomRight());
	        painter->drawLine(frame.bottomLeft(), frame.bottomRight());

			frame.adjust(1, 1, -1, -1);
	        painter->setPen(frameColor);
	        painter->drawLine(frame.topLeft(), frame.bottomLeft());
	        painter->drawLine(frame.topLeft(), frame.topRight());
	        painter->drawLine(frame.topRight(), frame.bottomRight());
	        painter->drawLine(frame.bottomLeft(), frame.bottomRight());

			frame.adjust(1, 1, -1, -1);
	        painter->setPen(bevelLight);
	        painter->drawLine(frame.topLeft(), frame.bottomLeft());
	        painter->drawLine(frame.topLeft(), frame.topRight());
	        painter->setPen(bevelShadow);
	        painter->drawLine(frame.topRight(), frame.bottomRight());
	        painter->drawLine(frame.bottomLeft(), frame.bottomRight());
	    }
		painter->restore();
		break;
	case PE_FrameMenu:
		painter->save();
		{
			painter->setPen(QPen(darkOutline, 1));
			painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
			QColor frameLight = option->palette.window().color().lighter(160);
			QColor frameShadow = option->palette.window().color().darker(110);

			//paint beveleffect
			QRect frame = option->rect.adjusted(1, 1, -1, -1);
			painter->setPen(frameLight);
			painter->drawLine(frame.topLeft(), frame.bottomLeft());
			painter->drawLine(frame.topLeft(), frame.topRight());

			painter->setPen(frameShadow);
			painter->drawLine(frame.topRight(), frame.bottomRight());
			painter->drawLine(frame.bottomLeft(), frame.bottomRight());
		}
		painter->restore();
		break;
	case PE_FrameDockWidget:

		painter->save();
		{
			QColor softshadow = option->palette.window().color().darker(120);

			QRect rect= option->rect;
			painter->setPen(softshadow);
			painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
			painter->setPen(QPen(option->palette.light(), 0));
			painter->drawLine(QPoint(rect.left() + 1, rect.top() + 1), QPoint(rect.left() + 1, rect.bottom() - 1));
			painter->setPen(QPen(option->palette.window().color().darker(120), 0));
			painter->drawLine(QPoint(rect.left() + 1, rect.bottom() - 1), QPoint(rect.right() - 2, rect.bottom() - 1));
			painter->drawLine(QPoint(rect.right() - 1, rect.top() + 1), QPoint(rect.right() - 1, rect.bottom() - 1));

		}
		painter->restore();
		break;
	case PE_PanelButtonTool:
		painter->save();
		if ((option->state & State_Enabled || option->state & State_On) || !(option->state & State_AutoRaise)) {
			QPen oldPen = painter->pen();

			if (widget && widget->inherits("QDockWidgetTitleButton")) {
				   if (option->state & State_MouseOver)
					   proxy()->drawPrimitive(PE_PanelButtonCommand, option, painter, widget);
			} else {
				proxy()->drawPrimitive(PE_PanelButtonCommand, option, painter, widget);
			}
		}
		painter->restore();
		break;
	case PE_IndicatorDockWidgetResizeHandle:
		{
			QStyleOption dockWidgetHandle = *option;
			bool horizontal = option->state & State_Horizontal;
			if (horizontal)
				dockWidgetHandle.state &= ~State_Horizontal;
			else
				dockWidgetHandle.state |= State_Horizontal;
			proxy()->drawControl(CE_Splitter, &dockWidgetHandle, painter, widget);
		}
		break;
	case PE_FrameWindow:
		painter->save();
		{
			bool active = (option->state & State_Active);
			if (widget && widget->inherits("QMdiSubWindow")) {
				qt_haiku_draw_windows_frame(painter, option->rect, active ? B_WINDOW_BORDER_COLOR : B_WINDOW_INACTIVE_BORDER_COLOR,
					BControlLook::B_LEFT_BORDER | BControlLook::B_RIGHT_BORDER | BControlLook::B_BOTTOM_BORDER);
			} else {
				QColor menuBackground = option->palette.window().color().lighter(104);
				painter->fillRect(option->rect, menuBackground);
				proxy()->drawPrimitive(PE_FrameMenu, option, painter, widget);
			}
		}
		painter->restore();
		break;
	case PE_FrameLineEdit:
	case PE_PanelLineEdit:
		if (const QStyleOptionFrame *lineEdit = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
	        painter->save();
	        {
				QRect r = lineEdit->rect;
			    BRect bRect(0.0f, 0.0f, r.width() - 1, r.height() - 1);
				TemporarySurface surface(bRect);

				rgb_color base = mkHaikuColor(option->palette.color( QPalette::Normal, QPalette::Window));
				surface.view()->SetViewColor(base);
				surface.view()->SetHighColor(ui_color(B_DOCUMENT_BACKGROUND_COLOR));
				surface.view()->SetLowColor(ui_color(B_DOCUMENT_BACKGROUND_COLOR));
				surface.view()->FillRect(bRect);

				uint32 flags = 0;
				if (!(lineEdit->state & State_Enabled))
					flags |= BControlLook::B_DISABLED;
				if (lineEdit->state & State_HasFocus)
					flags |= BControlLook::B_FOCUSED;

				bRect.InsetBy(-1, -1);
				if (widget) {
					if (qobject_cast<const QComboBox *>(widget->parentWidget()))
						bRect.InsetBy(-1, -1);
					if (qobject_cast<const QAbstractSpinBox *>(widget->parentWidget())) {
						painter->restore();
						break;
					}
				}
				if (lineEdit->lineWidth != 0)
					be_control_look->DrawTextControlBorder(surface.view(), bRect, bRect, base, flags);
				painter->drawImage(r, surface.image());
	        }
	        painter->restore();
		}
		break;
	case PE_IndicatorCheckBox:
		painter->save();
		if (const QStyleOptionButton *checkbox = qstyleoption_cast<const QStyleOptionButton*>(option)) {
			BRect bRect(0.0f, 0.0f, rect.width() - 1, rect.height() - 1);
			TemporarySurface surface(bRect);
			rgb_color base = mkHaikuColor(option->palette.color( QPalette::Normal, QPalette::Window));
			surface.view()->SetViewColor(base);
			surface.view()->SetHighColor(ui_color(B_DOCUMENT_BACKGROUND_COLOR));
			surface.view()->SetLowColor(ui_color(B_DOCUMENT_BACKGROUND_COLOR));
			surface.view()->FillRect(bRect);

			uint32 flags = 0;

			if (!(state & State_Enabled))
				flags |= BControlLook::B_DISABLED;
			if (checkbox->state & State_On)
				flags |= BControlLook::B_ACTIVATED;
			if (checkbox->state & State_HasFocus)
				flags |= BControlLook::B_FOCUSED;
			if (checkbox->state & State_Sunken)
				flags |= BControlLook::B_CLICKED;
			if (checkbox->state & State_NoChange)
				flags |= BControlLook::B_DISABLED | BControlLook::B_ACTIVATED;

			bRect.InsetBy(-1, -1);
			be_control_look->DrawCheckBox(surface.view(), bRect, bRect, base, flags);
			painter->drawImage(rect, surface.image());
		}
		painter->restore();
		break;
#if 0
	case PE_IndicatorRadioButton:
		painter->save();
		if (const QStyleOptionButton *radiobutton = qstyleoption_cast<const QStyleOptionButton*>(option)) {
			rect = rect.adjusted(1, -1, 3, 1);
			BRect bRect(0.0f, 0.0f, rect.width() - 1, rect.height() - 1);
			TemporarySurface surface(bRect);
			rgb_color base = mkHaikuColor(backgroundColor(option->palette, widget));
			surface.view()->SetHighColor(base);
			surface.view()->SetLowColor(base);
			surface.view()->SetViewColor(base);
			//surface.view()->FillRect(bRect);

			uint32 flags = 0;

			if (!(state & State_Enabled))
				flags |= BControlLook::B_DISABLED;
			if (radiobutton->state & State_On)
				flags |= BControlLook::B_ACTIVATED;
			if (radiobutton->state & State_HasFocus)
				flags |= BControlLook::B_FOCUSED;
			if (radiobutton->state & State_Sunken)
				flags |= BControlLook::B_CLICKED;
			if (radiobutton->state & State_NoChange)
				flags |= BControlLook::B_DISABLED | BControlLook::B_ACTIVATED;

			be_control_look->DrawRadioButton(surface.view(), bRect, bRect, base, flags);
			painter->drawImage(rect, surface.image());
		}
		painter->restore();
		break;
#endif
	case PE_IndicatorRadioButton:
		painter->save();
		if (const QStyleOptionButton *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
			painter->setRenderHint(QPainter::Antialiasing);

			bool on = button->state & State_On;
			bool sunken = button->state & State_Sunken;
			bool enabled = button->state & State_Enabled;
			bool focused = button->state & State_HasFocus;

			QColor base = backgroundColor(option->palette, widget);
			QColor borderColor;
			QColor bevelLight;
			QColor bevelShadow;
			QColor navigationColor = mkQColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));

			if (!enabled) {
				borderColor = mkQColor(tint_color(mkHaikuColor(base), 1.15));
				bevelLight = base;
				bevelShadow = base;
			} else if (sunken) {
				borderColor = mkQColor(tint_color(mkHaikuColor(base), 1.50));
				bevelLight = borderColor;
				bevelShadow = borderColor;
			} else {
				borderColor = mkQColor(tint_color(mkHaikuColor(base), 1.45));
				bevelLight = mkQColor(tint_color(mkHaikuColor(base), 0.55));
				bevelShadow = mkQColor(tint_color(mkHaikuColor(base), 1.11));
			}

			if (focused)
				borderColor = navigationColor;

			QLinearGradient bevelGradient(option->rect.topLeft(), option->rect.bottomRight());
			bevelGradient.setColorAt(0, bevelShadow);
			bevelGradient.setColorAt(1, bevelLight);

			QLinearGradient borderGradient(option->rect.topLeft(), option->rect.bottomRight());
			borderGradient.setColorAt(0, borderColor);
			borderGradient.setColorAt(1, mkQColor(tint_color(mkHaikuColor(borderColor), 0.8)));

			painter->setPen(Qt::NoPen);
			//TODO: invalid blending bevel in LibreOffice
			//painter->setBrush(bevelGradient);
			//painter->drawEllipse(rect);
			rect = rect.adjusted(1, 1, -1, -1);

			painter->setBrush(borderGradient);
			painter->drawEllipse(rect);
			rect = rect.adjusted(1, 1, -1, -1);

			float topTint;
			float bottomTint;
			if (!enabled) {
				topTint = 0.4;
				bottomTint = 0.2;
			} else {
				topTint = 0.15;
				bottomTint = 0.0;
			}

			QLinearGradient bgGradient(option->rect.topLeft(), option->rect.bottomRight());
			bgGradient.setColorAt(0, mkQColor(tint_color(mkHaikuColor(base), topTint)));
			bgGradient.setColorAt(1, mkQColor(tint_color(mkHaikuColor(base), bottomTint)));

			painter->setBrush(bgGradient);
			painter->drawEllipse(rect);
			rect = rect.adjusted(3, 3, -3, -3);

			rgb_color color = ui_color(B_CONTROL_MARK_COLOR);
			float mix = 1.0;
			if (!enabled) {
				mix = 0.4;
			} else if (sunken) {
				if (on) {
					mix = 0.7;
				} else {
					mix = 0.3;
				}
			}

			rgb_color hbase = mkHaikuColor(base);
			color.red = uint8(color.red * mix + hbase.red * (1.0 - mix));
			color.green = uint8(color.green * mix + hbase.green * (1.0 - mix));
			color.blue = uint8(color.blue * mix + hbase.blue * (1.0 - mix));

			if (on || (enabled && sunken)) {
				painter->setPen(Qt::NoPen);
				painter->setBrush(mkQColor(color));
				painter->drawEllipse(rect);
			}
		}
		painter->restore();
		break;
	case PE_IndicatorToolBarHandle:
		painter->save();
		{
			rgb_color base = mkHaikuColor(backgroundColor(option->palette, widget));
			QColor vdark = mkQColor(tint_color(base, B_DARKEN_3_TINT));
			QColor light = mkQColor(tint_color(base, B_LIGHTEN_2_TINT));

	        if (option->state & State_Horizontal) {
				int x = rect.center().x();
				int y = rect.y() + 4;

				while (y < rect.bottom() - 3) {
					painter->setPen(QPen(vdark, 0));
					painter->drawLine(x, y, x, y);
					painter->setPen(QPen(light, 0));
					painter->drawLine(x + 1, y + 1, x + 1, y + 1);
					y += 3;
				}
	        }
	        else { //vertical toolbar
				int x = rect.x() + 4;
				int y = rect.center().y();

				while (x <= rect.right() - 3) {
					painter->setPen(QPen(vdark, 0));
					painter->drawLine(x, y, x, y);
					painter->setPen(QPen(light, 0));
					painter->drawLine(x + 1, y + 1, x + 1, y + 1);
					x += 3;
				}
	        }
		}
		painter->restore();
		break;
	case PE_FrameDefaultButton:
	case PE_FrameFocusRect:
		break;
	case PE_PanelButtonCommand:
		painter->save();
		{
			bool isDefault = false;
			bool isFlat = false;
			bool isDown = (option->state & State_Sunken) || (option->state & State_On);
			bool isTool = (elem == PE_PanelButtonTool);
			bool isMouseOver = (option->state & State_MouseOver) && (option->state & State_Enabled);

			if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton*>(option)) {
				isDefault = btn->features & QStyleOptionButton::DefaultButton;
				isFlat = (btn->features & QStyleOptionButton::Flat);
			}

			if (isTool && !(option->state & State_Enabled || option->state & State_On) && (option->state & State_AutoRaise))
				break;

			bool hasFocus = option->state & State_HasFocus;
			bool isEnabled = option->state & State_Enabled;

			rgb_color background = mkHaikuColor(option->palette.color( QPalette::Normal, QPalette::Window));
			rgb_color base = mkHaikuColor(option->palette.color( QPalette::Normal, QPalette::Button));
			base = tint_color(base, 1.12); //FIX: strange correction

			QRect rect = option->rect;

			uint32 flags = BControlLook::B_IS_CONTROL;

			if (isDown)
				flags |= BControlLook::B_ACTIVATED;
			if (hasFocus)
				flags |= BControlLook::B_FOCUSED;
			if (isFlat)
				flags |= BControlLook::B_FLAT;
			if (isMouseOver)
				flags |= BControlLook::B_HOVER;
			if (!isEnabled)
				flags |= BControlLook::B_DISABLED;
			if (isDefault) {
				flags |= BControlLook::B_DEFAULT_BUTTON;
				rect = rect.adjusted(-3,-3,3,3);
			}

			BRect bRect(0.0f, 0.0f, rect.width() - 1, rect.height() - 1);

			TemporarySurface surface(bRect);

			surface.view()->SetViewColor(background);
			surface.view()->SetHighColor(background);
			surface.view()->SetLowColor(base);
			surface.view()->FillRect(bRect);

			be_control_look->DrawButtonFrame(surface.view(), bRect, bRect, base, background, flags);
			be_control_look->DrawButtonBackground(surface.view(), bRect, bRect, base, flags);

			painter->drawImage(rect, surface.image());
	     	painter->restore();
		}
		break;
	case PE_FrameTabWidget:
		painter->save();
		if (const QStyleOptionTabWidgetFrame *twf = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(option)) {
			QRect barRect = subElementRect(SE_TabWidgetTabBar, option, widget);

			rgb_color bgColor = mkHaikuColor(option->palette.color( QPalette::Normal, QPalette::Window));
	        QColor backgroundColor(mkQColor(bgColor));
	        QColor frameColor(mkQColor(tint_color(bgColor, 1.30)));
	        QColor bevelLight(mkQColor(tint_color(bgColor, 0.8)));
	        QColor bevelShadow(mkQColor(tint_color(bgColor, 1.03)));

	        QRect frame = option->rect;

	        switch(twf->shape) {
				case QTabBar::RoundedNorth:
				case QTabBar::TriangularNorth:
				case QTabBar::RoundedWest:
				case QTabBar::TriangularWest:
					break;
				case QTabBar::RoundedSouth:
				case QTabBar::TriangularSouth:
					frame.adjust(0,0,0,2);
					break;
				case QTabBar::RoundedEast:
				case QTabBar::TriangularEast:
					frame.adjust(0,0,2,0);
					break;
			}

	        painter->setPen(bevelShadow);
	        painter->drawLine(frame.topLeft(), frame.bottomLeft());
	        painter->drawLine(frame.topLeft(), frame.topRight());
	        painter->setPen(bevelLight);
	        painter->drawLine(frame.topRight(), frame.bottomRight());
	        painter->drawLine(frame.bottomLeft(), frame.bottomRight());

			frame.adjust(1, 1, -1, -1);
	        painter->setPen(frameColor);
	        painter->drawLine(frame.topLeft(), frame.bottomLeft());
	        painter->drawLine(frame.topLeft(), frame.topRight());
	        painter->drawLine(frame.topRight(), frame.bottomRight());
	        painter->drawLine(frame.bottomLeft(), frame.bottomRight());

			frame.adjust(1, 1, -1, -1);
	        painter->setPen(bevelLight);
	        painter->drawLine(frame.topLeft(), frame.bottomLeft());
	        painter->drawLine(frame.topLeft(), frame.topRight());
	        painter->setPen(bevelShadow);
	        painter->drawLine(frame.topRight(), frame.bottomRight());
	        painter->drawLine(frame.bottomLeft(), frame.bottomRight());

	        frame = option->rect;

			if (be_control_look != NULL && (twf->shape == QTabBar::RoundedNorth || twf->shape == QTabBar::RoundedSouth)) {
				QRect r = twf->shape == QTabBar::RoundedNorth ?
					QRect(QPoint(frame.left(), barRect.top()), QPoint(frame.right(), barRect.bottom())) :
					QRect(QPoint(frame.left(), frame.bottom()), QPoint(frame.right(), frame.bottom() + barRect.height() - 1));
				uint32 side = twf->shape == QTabBar::RoundedNorth ? BControlLook::B_TOP_BORDER : BControlLook::B_BOTTOM_BORDER;
		        BRect bRect(0.0f, 0.0f, r.width() - 1, r.height() - 1);
		        BRect bRect1 = bRect;
		        bRect1.bottom++;
				TemporarySurface surface(bRect);
				be_control_look->DrawInactiveTab(surface.view(), bRect1, bRect, bgColor, 0, BControlLook::B_ALL_BORDERS, side);
				painter->drawImage(r, surface.image());
			}
		}
		painter->restore();
		break ;
	case PE_FrameStatusBarItem:
		break;
	case PE_IndicatorTabClose:
		{
			static QIcon tabBarcloseButtonIcon;
			if (tabBarcloseButtonIcon.isNull())
				tabBarcloseButtonIcon = standardIcon(SP_DialogCloseButton, option, widget);
			if ((option->state & State_Enabled) && (option->state & State_MouseOver))
				proxy()->drawPrimitive(PE_PanelButtonCommand, option, painter, widget);
			QPixmap pixmap = tabBarcloseButtonIcon.pixmap(QSize(16, 16), QIcon::Normal, QIcon::On);
			proxy()->drawItemPixmap(painter, option->rect, Qt::AlignCenter, pixmap);
		}
		break;
	default:
		QProxyStyle::drawPrimitive(elem, option, painter, widget);
		break;
	}
}

/*!
  \reimp
*/
void QHaikuStyle::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter,
								   const QWidget *widget) const
{
	QColor button = option->palette.button().color();
	QColor dark;
	dark.setHsv(button.hue(),
				qMin(255, (int)(button.saturation()*1.9)),
				qMin(255, (int)(button.value()*0.7)));
	QColor darkOutline;
	darkOutline.setHsv(button.hue(),
				qMin(255, (int)(button.saturation()*2.0)),
				qMin(255, (int)(button.value()*0.6)));
	QRect rect = option->rect;
	QColor shadow = mergedColors(option->palette.window().color().darker(120),
								 dark.lighter(130), 60);

	/*QColor highlight = option->palette.highlight().color();*/

	switch (element) {
	 case CE_ShapedFrame:
		painter->save();
		{
			if (const QStyleOptionFrame *frame = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
				QRect rect = option->rect;
				border_style border = B_NO_BORDER;

				if (frame->frameShape == QFrame::Box)
					border = B_PLAIN_BORDER;
				else if (frame->frameShape == QFrame::Panel ||
					frame->frameShape == QFrame::StyledPanel ||
					frame->frameShape == QFrame::WinPanel ||
					frame->frameShape == QFrame::HLine ||
					frame->frameShape == QFrame::VLine)
					border = B_FANCY_BORDER;
				if (border == B_NO_BORDER) {
					painter->restore();
					break;
				}

				BRect bRect(0.0f, 0.0f, rect.width() - 1, rect.height() - 1);
				TemporarySurface surface(bRect);

				uint32 flags = 0;
				if (option->state & State_HasFocus)
					flags |= BControlLook::B_FOCUSED;

				rgb_color base = mkHaikuColor(option->palette.color( QPalette::Normal, QPalette::Window));
				surface.view()->SetViewColor(base);
				surface.view()->SetHighColor(base);
				surface.view()->SetLowColor(base);
				surface.view()->FillRect(bRect);
				be_control_look->DrawScrollViewFrame(surface.view(), bRect, bRect,
					BRect(), BRect(), base, border, flags, BControlLook::B_ALL_BORDERS);
				painter->setClipping(true);
				QRegion allRegion(rect, QRegion::Rectangle);
				QRegion excludeRegion(rect.adjusted(4, 4, -4, -4), QRegion::Rectangle);
				QRegion region=allRegion.subtracted(excludeRegion);
				painter->setClipRegion(region);
				painter->drawImage(rect, surface.image());
			}
		}
		painter->restore();
		break;
	 case CE_RadioButton: //fall through
	 case CE_CheckBox:
		if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
			QStyleOptionButton copy = *btn;
			//copy.rect.adjust(1, -1, 3, 2);
			QProxyStyle::drawControl(element, &copy, painter, widget);
		}
		break;
	case CE_SizeGrip:
		break;
	case CE_Splitter:
		painter->save();
		{
			orientation orient = (option->state & State_Horizontal)?B_HORIZONTAL:B_VERTICAL;

			if (be_control_look != NULL) {
				QRect r = option->rect;
				rgb_color base = mkHaikuColor(option->palette.color( QPalette::Normal, QPalette::Window));
				uint32 flags = 0;
		        BRect bRect(0.0f, 0.0f, r.width() - 1, r.height() - 1);
				TemporarySurface surface(bRect);
				surface.view()->SetHighColor(base);
				surface.view()->SetLowColor(base);
				surface.view()->FillRect(bRect);
				bRect.InsetBy(-1, -1);
				be_control_look->DrawSplitter(surface.view(), bRect, bRect, base, orient, flags);
				painter->drawImage(r, surface.image());
			}
		}
		painter->restore();
		break;
	case CE_ToolBar:
		// Reserve the beveled appearance only for mainwindow toolbars
		if (!(widget && qobject_cast<const QMainWindow*> (widget->parentWidget())))
			break;

		painter->save();
		if (const QStyleOptionToolBar *toolbar = qstyleoption_cast<const QStyleOptionToolBar *>(option)) {
			QRect rect = option->rect;

			bool paintTopBorder = false;
			bool paintLeftBorder = true;
			bool paintRightBorder = true;
			bool paintBottomBorder = true;

			switch (toolbar->toolBarArea) {
			case Qt::BottomToolBarArea:
				switch (toolbar->positionOfLine) {
				case QStyleOptionToolBar::Beginning:
				case QStyleOptionToolBar::OnlyOne:
					paintBottomBorder = false;
					break;
				default:
					break;
				}
				break;
			case Qt::TopToolBarArea:
				switch (toolbar->positionWithinLine) {
				case QStyleOptionToolBar::Beginning:
					paintLeftBorder = false;
					break;
				case QStyleOptionToolBar::End:
					paintRightBorder = false;
					break;
				case QStyleOptionToolBar::OnlyOne:
					paintRightBorder = false;
					paintLeftBorder = false;
					break;
				default:
					break;
				}
				if (toolbar->direction == Qt::RightToLeft) { //reverse layout changes the order of Beginning/end
					bool tmp = paintLeftBorder;
					paintRightBorder=paintLeftBorder;
					paintLeftBorder=tmp;
				}
				break;
			case Qt::RightToolBarArea:
				switch (toolbar->positionOfLine) {
				case QStyleOptionToolBar::Beginning:
				case QStyleOptionToolBar::OnlyOne:
					paintRightBorder = false;
					break;
				default:
					break;
				}
				break;
			case Qt::LeftToolBarArea:
				switch (toolbar->positionOfLine) {
				case QStyleOptionToolBar::Beginning:
				case QStyleOptionToolBar::OnlyOne:
					paintLeftBorder = false;
					break;
				default:
					break;
				}
				break;
			default:
				break;
			}

			QColor light = option->palette.window().color().lighter(110);

			//draw borders
			if (paintTopBorder) {
				painter->setPen(QPen(light));
				painter->drawLine(rect.topLeft().x(),
							rect.topLeft().y(),
							rect.topRight().x(),
							rect.topRight().y());
			}

			if (paintLeftBorder) {
				painter->setPen(QPen(light));
				painter->drawLine(rect.topLeft().x(),
							rect.topLeft().y(),
							rect.bottomLeft().x(),
							rect.bottomLeft().y());
			}

			if (paintRightBorder) {
				painter->setPen(QPen(shadow));
				painter->drawLine(rect.topRight().x(),
							rect.topRight().y(),
							rect.bottomRight().x(),
							rect.bottomRight().y());
			}

			if (paintBottomBorder) {
				painter->setPen(QPen(shadow));
				painter->drawLine(rect.bottomLeft().x(),
							rect.bottomLeft().y(),
							rect.bottomRight().x(),
							rect.bottomRight().y());
			}
		}
		painter->restore();
		break;
	case CE_DockWidgetTitle:
		if (const QStyleOptionDockWidget *dockWidget = qstyleoption_cast<const QStyleOptionDockWidget *>(option)) {
			painter->save();

			bool verticalTitleBar = dockWidget->verticalTitleBar;

			rgb_color bgColor = mkHaikuColor(option->palette.color( QPalette::Normal, QPalette::Window));
			QColor backgroundColor(mkQColor(tint_color(bgColor, 1.1)));
			QColor frameColor(mkQColor(tint_color(bgColor, 1.3)));

			QRect titleRect = subElementRect(SE_DockWidgetTitleBarText, option, widget);
			QRect rect = dockWidget->rect;

			if (verticalTitleBar) {
				QRect r = rect;
				QSize s = r.size();
				s.transpose();
				r.setSize(s);

				titleRect = QRect(r.left() + rect.bottom()
									- titleRect.bottom(),
								r.top() + titleRect.left() - rect.left(),
								titleRect.height(), titleRect.width());

				painter->translate(r.left(), r.top() + r.width());
				painter->rotate(-90);
				painter->translate(-r.left(), -r.top());

				rect = r;
			}

			painter->setPen(frameColor);
			painter->fillRect(rect.adjusted(-1, 3, 1, -3), backgroundColor);
			painter->drawRect(rect.adjusted(-1, 3, 1, -3));

			QString title = painter->fontMetrics().elidedText(dockWidget->title,
											Qt::ElideRight, titleRect.width());

			QFont font = painter->font();
			font.setPointSize(font.pointSize() - 1);
			painter->setFont(font);
			painter->setPen(dockWidget->palette.windowText().color());
			painter->drawText(titleRect,
							  int(Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextShowMnemonic),
							  title);

			painter->restore();
		}
		break;
	case CE_HeaderSection:
		painter->save();
		if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
			BRect drawRect;
			QRect rect = header->rect;
			rgb_color background = mkHaikuColor(option->palette.color( QPalette::Normal, QPalette::Window));
			rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
			BRect bRect(0.0f, 0.0f, rect.width() - 1, rect.height() - 1);

			TemporarySurface surface(bRect);

			surface.view()->SetViewColor(background);
			surface.view()->SetHighColor(background);
			surface.view()->SetLowColor(base);
			surface.view()->FillRect(bRect);

			BRect bgRect = bRect;
			surface.view()->SetHighColor(tint_color(base, B_DARKEN_2_TINT));
			surface.view()->StrokeLine(bgRect.LeftBottom(), bgRect.RightBottom());

			bgRect.bottom--;
			bgRect.right--;

			uint32 flags = BControlLook::B_IS_CONTROL;

			if (option->state & State_Sunken) {
				base = tint_color(base, B_DARKEN_1_TINT);
				flags |= BControlLook::B_ACTIVATED;
			}

			be_control_look->DrawButtonBackground(surface.view(), bgRect, bgRect, base, 0,
				BControlLook::B_TOP_BORDER | BControlLook::B_BOTTOM_BORDER | flags);

			surface.view()->SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_2_TINT));
			surface.view()->StrokeLine(bRect.RightTop(), bRect.RightBottom());
			painter->drawImage(rect, surface.image());
		}
		painter->restore();
		break;
	case CE_ProgressBarGroove:
		break;
	case CE_ProgressBarContents:
		painter->save();
		if (const QStyleOptionProgressBar *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
			QRect rect = bar->rect;
			bool vertical = (bar->orientation == Qt::Vertical);
			bool inverted = bar->invertedAppearance;
			bool indeterminate = (bar->minimum == 0 && bar->maximum == 0);

			rgb_color base = mkHaikuColor(backgroundColor(option->palette, widget));
			rgb_color highlight = ui_color(B_STATUS_BAR_COLOR);

			int maxWidth = vertical?rect.height():rect.width();

			BRect bRect;
			qreal progress = qMax(bar->progress, bar->minimum);
			int progressBarWidth = (progress - bar->minimum) * qreal(maxWidth) / qMax(qreal(1.0), qreal(bar->maximum) - bar->minimum);

			if (vertical)
				bRect = BRect(0.0f, 0.0f, rect.height() - 1, rect.width() - 1);
			else
				bRect = BRect(0.0f, 0.0f, rect.width() - 1, rect.height() - 1);

			if (indeterminate)
				progressBarWidth = maxWidth;

			TemporarySurface surface(bRect, QImage::Format_RGB32);
			bRect.InsetBy(-1, -1);
			be_control_look->DrawStatusBar(surface.view(), bRect, bRect, base, highlight, progressBarWidth);

			if (vertical) {
				QImage dstImg = surface.image().mirrored(false, inverted);
				painter->drawImage(rect, dstImg);
			} else {
				if (indeterminate) {
					float fStripeWidth = (bRect.Width() / 4) + 5;
					if (fStripeWidth > 200)
						fStripeWidth = 200;
					BPoint stripePoints[4];
					stripePoints[0].Set(fStripeWidth * 0.5, 0.0);
					stripePoints[1].Set(fStripeWidth * 1.5, 0.0);
					stripePoints[2].Set(fStripeWidth, bRect.Height());
					stripePoints[3].Set(0.0, bRect.Height());
					BPolygon fStripe = BPolygon(stripePoints, 4);

					rgb_color fColors[2];
					rgb_color otherColor = tint_color(ui_color(B_STATUS_BAR_COLOR), 1.3);
					otherColor.alpha = 50;
					fColors[0] = otherColor;
					fColors[1] = B_TRANSPARENT_COLOR;

					uint32 fNumColors = 2;
					uint32 fNumStripes = (int32)ceilf((bRect.Width()) / fStripeWidth) + 1 + fNumColors;

					float fScrollOffset = ((animateStep % progressAnimationFps) * (fStripeWidth * fNumColors) ) / progressAnimationFps;
					float position = -fStripeWidth * (fNumColors + 0.5) + fScrollOffset;

					surface.view()->SetDrawingMode(B_OP_ALPHA);
					uint32 colorIndex = 0;
					for (uint32 i = 0; i < fNumStripes; i++) {
						surface.view()->SetHighColor(fColors[colorIndex]);
						colorIndex++;
						if (colorIndex >= fNumColors)
							colorIndex = 0;
						BRect stripeFrame = fStripe.Frame();
						fStripe.MapTo(stripeFrame, stripeFrame.OffsetToCopy(position, 0.0));
						surface.view()->FillPolygon(&fStripe);
						position += fStripeWidth;
					}
					surface.view()->SetDrawingMode(B_OP_COPY);
				}
				be_control_look->DrawBorder(surface.view(), bRect, bRect, ui_color(B_PANEL_BACKGROUND_COLOR), B_PLAIN_BORDER);
				painter->drawImage(rect, inverted?surface.image().mirrored(true, false):surface.image());
			}
		}
		painter->restore();
		break;
	case CE_MenuBarItem:
		painter->save();
		if (const QStyleOptionMenuItem *mbi = qstyleoption_cast<const QStyleOptionMenuItem *>(option))
		{
			QStyleOptionMenuItem item = *mbi;
			item.rect = mbi->rect;

			bool act = mbi->state & State_Selected && mbi->state & State_Sunken;
			bool dis = !(mbi->state & State_Enabled);

			uint32 flags = 0;
			if (act)
				flags |= BControlLook::B_ACTIVATED;
			if (dis)
				flags |= BControlLook::B_DISABLED;

			if (be_control_look != NULL) {
				rgb_color base = ui_color(B_MENU_BACKGROUND_COLOR);
				rgb_color textColor = ui_color(B_MENU_ITEM_TEXT_COLOR);

				if (!dis && act)
					textColor = ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR);
				else if (!dis)
					textColor = ui_color(B_MENU_ITEM_TEXT_COLOR);
				else {
					if (base.red + base.green + base.blue > 128 * 3)
					textColor = tint_color(base, B_DISABLED_LABEL_TINT);
				else
					textColor = tint_color(base, B_LIGHTEN_2_TINT);
				}

		        BRect bRect(0.0f, 0.0f, option->rect.width() - 1, option->rect.height() - 1);
				TemporarySurface surface(bRect);

				if (act) {
					base = ui_color(B_MENU_SELECTED_BACKGROUND_COLOR);
					surface.view()->SetLowColor(base);
					be_control_look->DrawMenuItemBackground(surface.view(), bRect, bRect, base, flags, BControlLook::B_ALL_BORDERS);
				} else {
					be_control_look->DrawMenuBarBackground(surface.view(), bRect, bRect, base, flags, 8);
				}

				painter->drawImage(option->rect, surface.image());

				if (!mbi->text.isEmpty()) {
					QColor textQColor(mkQColor(textColor));
					painter->setPen(textQColor);
				    painter->drawText(item.rect, Qt::AlignCenter  | Qt::TextHideMnemonic | Qt::TextDontClip | Qt::TextSingleLine, mbi->text);
				}
			}
		}
		painter->restore();
		break;
	case CE_MenuItem:
		painter->save();
		// Draws one item in a popup menu.
		if (const QStyleOptionMenuItem *menuItem = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
			bool dis = !(menuItem->state & State_Enabled);
			bool act = menuItem->state & State_Selected;
			bool selected = menuItem->state & State_Selected && menuItem->state & State_Enabled;
			bool ignoreCheckMark = (qobject_cast<const QComboBox*>(widget) != NULL);
			bool checkable = menuItem->checkType != QStyleOptionMenuItem::NotCheckable;
			bool checked = menuItem->checked;
			bool sunken = menuItem->state & State_Sunken;
#if 0
			bool enabled = menuItem->state & State_Enabled;
#endif
			int checkcol = qMax(menuItem->maxIconWidth, 20);

			QColor discol;
			if (dis) {
				rgb_color bgColor = ui_color(B_MENU_BACKGROUND_COLOR);
				if (bgColor.red + bgColor.green + bgColor.blue > 128 * 3)
					discol = mkQColor(tint_color(bgColor, B_DISABLED_LABEL_TINT));
				else
					discol = mkQColor(tint_color(bgColor, B_LIGHTEN_2_TINT));
			}

			bool sub_exist = false;
			if (const QMenu *menu = qobject_cast<const QMenu *>(widget)){
				foreach (QAction *action, menu->actions()) {
					if (action->menu()) {
						sub_exist = true;
						break;
					}
				}
			}

			BRect itemBRect(0.0f, 0.0f, option->rect.width() - 1, option->rect.height() - 1);
			TemporarySurface itemSurface(itemBRect);
			// MenuItem background
			if (selected) {
				BRect bgBRect = itemBRect;
				itemSurface.view()->SetLowColor(ui_color(B_MENU_SELECTED_BACKGROUND_COLOR));
				be_control_look->DrawMenuItemBackground(itemSurface.view(), bgBRect, bgBRect,
					ui_color(B_MENU_SELECTED_BACKGROUND_COLOR), BControlLook::B_ACTIVATED);
			} else {
				itemSurface.view()->SetHighColor(ui_color(B_MENU_BACKGROUND_COLOR));
				itemSurface.view()->FillRect(itemBRect);
				rgb_color background = tint_color(ui_color(B_MENU_BACKGROUND_COLOR), 0.75);
				rgb_color bevelLightColor = tint_color(background, 0.6);
				rgb_color bevelShadowColor = tint_color(background, 1.12);
				itemSurface.view()->SetHighColor(bevelLightColor);
				itemSurface.view()->StrokeLine(BPoint(itemBRect.left, itemBRect.top), BPoint(itemBRect.left, itemBRect.bottom));
				itemSurface.view()->SetHighColor(bevelShadowColor);
				itemSurface.view()->StrokeLine(BPoint(itemBRect.right, itemBRect.top), BPoint(itemBRect.right, itemBRect.bottom));
				if (qobject_cast<const QMenu *>(widget)) {
					itemSurface.view()->SetHighColor(bevelLightColor);
					if (menuItem->rect.top() < 2)
						itemSurface.view()->StrokeLine(BPoint(itemBRect.left, itemBRect.top), BPoint(itemBRect.right, itemBRect.top));
					itemSurface.view()->SetHighColor(bevelShadowColor);
					if (fabs(widget->rect().bottom() - menuItem->rect.bottom()) < 2)
						itemSurface.view()->StrokeLine(BPoint(itemBRect.left, itemBRect.bottom), BPoint(itemBRect.right, itemBRect.bottom));
				}
			}
			// Separator item
			if (menuItem->menuItemType == QStyleOptionMenuItem::Separator) {
				const float startTop = itemBRect.top + (floor(itemBRect.Height())) / 2;
				itemSurface.view()->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_1_TINT));
				itemSurface.view()->StrokeLine(BPoint(itemBRect.left + 1.0f, startTop),
					BPoint(itemBRect.right - 1.0f, startTop));
				itemSurface.view()->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_LIGHTEN_2_TINT));
				itemSurface.view()->StrokeLine(BPoint(itemBRect.left + 1.0f, startTop + 1.0f),
					BPoint(itemBRect.right - 1.0f, startTop + 1.0f));

				painter->drawImage(option->rect, itemSurface.image());
				painter->restore();
				break;
			}
			// Checkmarks
			if (checkable && !ignoreCheckMark) {
				float symbolSize = roundf(itemBRect.Height() * 2 / 3) - 1;
				BRect boundRect(itemBRect);
				boundRect.left = boundRect.left + 2;
				BRect symbolRect(0, 0, symbolSize, symbolSize);
				symbolRect.OffsetTo(BPoint(boundRect.left, itemBRect.top + (itemBRect.Height() - symbolSize) / 2));

				BPoint center(floorf((symbolRect.left + symbolRect.right) / 2.0),
				floorf((symbolRect.top + symbolRect.bottom) / 2.0));

				rgb_color markColor = ui_color(selected?B_MENU_SELECTED_ITEM_TEXT_COLOR:B_MENU_ITEM_TEXT_COLOR);
				if (dis)
					markColor = mkHaikuColor(discol);
				itemSurface.view()->SetHighColor(markColor);

				if (menuItem->checkType & QStyleOptionMenuItem::Exclusive) {
					if (checked || sunken) {
						float r = (std::min(symbolRect.Height() - 2, symbolRect.Width())) / 5.0;
						itemSurface.view()->SetDrawingMode(B_OP_OVER);
						itemSurface.view()->SetPenSize(1.0);
						itemSurface.view()->SetHighColor(tint_color(markColor, 0.75f));
						itemSurface.view()->FillEllipse(center, r, r);
					}
				} else {
					if (checked || sunken) {
						float size = std::min(symbolRect.Height() - 2, symbolRect.Width());
						symbolRect.top = floorf(center.y - size / 2 + 0.5);
						symbolRect.bottom = floorf(center.y + size / 2 + 0.5);
						symbolRect.left = floorf(center.x - size / 2 + 0.5);
						symbolRect.right = floorf(center.x + size / 2 + 0.5);

						BShape arrowShape;
						center.x += 0.5;
						center.y += 0.5;
						size *= 0.3;
						arrowShape.MoveTo(BPoint(center.x - size, center.y - size * 0.25));
						arrowShape.LineTo(BPoint(center.x - size * 0.25, center.y + size));
						arrowShape.LineTo(BPoint(center.x + size, center.y - size));

						itemSurface.view()->SetHighColor(tint_color(markColor, 0.75f));
						itemSurface.view()->SetDrawingMode(B_OP_OVER);
						itemSurface.view()->SetPenSize(2.0);
						itemSurface.view()->MovePenTo(B_ORIGIN);
						itemSurface.view()->StrokeShape(&arrowShape);
					}
				}
			}

			float leftMargin = roundf(itemBRect.Height() * 2 / 3) - 3;
			if (menuItem->icon.isNull() || !showMenuIcon)
				checkcol = leftMargin;
			else
				checkcol = leftMargin + menuItem->maxIconWidth;
			// Submenu arrow
			if (menuItem->menuItemType == QStyleOptionMenuItem::SubMenu) {
				float symbolSize = roundf(itemBRect.Height() * 2 / 3);
				BRect boundRect(itemBRect);
				boundRect.left = boundRect.right - symbolSize;
				BRect symbolRect(0, 0, symbolSize, symbolSize);
				symbolRect.OffsetTo(BPoint(boundRect.left, itemBRect.top + (itemBRect.Height() - symbolSize) / 2));
				rgb_color arrowColor = ui_color(selected?B_MENU_SELECTED_ITEM_TEXT_COLOR:B_MENU_ITEM_TEXT_COLOR);
				if (dis)
					arrowColor = mkHaikuColor(discol);
				be_control_look->DrawArrowShape(itemSurface.view(), symbolRect, symbolRect,
					arrowColor, BControlLook::B_RIGHT_ARROW, 0, 0.75f);
			}
			// Shotcuts
			int tabIdx = menuItem->text.indexOf(QLatin1Char('\t'));
			if (tabIdx > 0) {
				QString shotcutText = menuItem->text.mid(tabIdx + 1);
				QStringList shotcutList = shotcutText.split('+');
				if (shotcutText.right(1).contains('+')) {
					shotcutList.removeAll(QString(""));
					shotcutList << "+";
				}

				shotcutList.last().replace("Return","⏎");
				shotcutList.last().replace("Left","←");
				shotcutList.last().replace("Right","→");
				shotcutList.last().replace("Up","↑");
				shotcutList.last().replace("Down","↓");
				shotcutList.last().replace("Tab","↹");

				if (shotcutList.last().size() > 1 && shotcutList.count() > 1)
					shotcutList.clear();

				BPoint where(0, 0);
				float symbolSize = roundf(itemBRect.Height() * 2 / 3);
				where.x = (itemBRect.right - 8) - menuItem->font.pixelSize();
				if (sub_exist)
					where.x -= symbolSize;
				else
					where.x -= itemBRect.Height() / 2;

				BFont bFont(be_plain_font);
				if (shotcutList.count() > 0) {
					font_height fHeight;
					bFont.GetHeight(&fHeight);
					where.y = (itemBRect.Height() - (fHeight.ascent + fHeight.descent)) / 2 + fHeight.ascent;
		            if (selected)
						itemSurface.view()->SetHighColor(ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR));
					else
						itemSurface.view()->SetHighColor(ui_color(B_MENU_ITEM_TEXT_COLOR));
					if (dis)
						itemSurface.view()->SetHighColor(mkHaikuColor(discol));
					if (shotcutList.last().size() > 1)
						where.x -= (bFont.StringWidth(shotcutList.last().toUtf8().data()) - bFont.StringWidth("_"));
					itemSurface.view()->DrawString(shotcutList.last().toUtf8().data(), where);
					where.x -= 4;
				}

				uint32 ctrlKey;
				get_modifier_key(B_LEFT_CONTROL_KEY, &ctrlKey);
				BBitmap *ctrlBitmap = ctrlKey==0x5c ? sMenuItemAlt : sMenuItemControl;
				BBitmap *altBitmap = ctrlKey==0x5c ? sMenuItemControl : sMenuItemAlt;

				if (shotcutList.contains("Ctrl")) {
					const BRect &rect = ctrlBitmap->Bounds();
					where.y = roundf(itemBRect.Height() - rect.Height()) / 2;
					where.x -= rect.Width() + 1;
					itemSurface.view()->DrawBitmap(ctrlBitmap, where);
				}
				if (shotcutList.contains("Alt")) {
					const BRect &rect = altBitmap->Bounds();
					where.y = roundf(itemBRect.Height() - rect.Height()) / 2;
					where.x -= rect.Width() + 1;
					itemSurface.view()->DrawBitmap(altBitmap, where);
				}
				if (shotcutList.contains("Meta")) {
					const BRect &rect = sMenuItemOption->Bounds();
					where.y = roundf(itemBRect.Height() - rect.Height()) / 2;
					where.x -= rect.Width() + 1;
					itemSurface.view()->DrawBitmap(sMenuItemOption, where);
				}
				if (shotcutList.contains("Shift")) {
					const BRect &rect = sMenuItemShift->Bounds();
					where.y = roundf(itemBRect.Height() - rect.Height()) / 2;
					where.x -= rect.Width() + 1;
					itemSurface.view()->DrawBitmap(sMenuItemShift, where);
				}
			}

			painter->drawImage(option->rect, itemSurface.image());

			// Text and icon, ripped from windows style
			const QStyleOption *opt = option;
			const QStyleOptionMenuItem *menuitem = menuItem;

			QPainter *p = painter;
			QRect vCheckRect = visualRect(opt->direction, menuitem->rect,
										  QRect(menuitem->rect.x(), menuitem->rect.y(),
												checkcol, menuitem->rect.height()));
			if (!menuItem->icon.isNull() && showMenuIcon) {
				QIcon::Mode mode = dis ? QIcon::Disabled : QIcon::Normal;
				if (act && !dis)
					mode = QIcon::Active;
				QPixmap pixmap;

				int smallIconSize = proxy()->pixelMetric(PM_SmallIconSize, option, widget);
				QSize iconSize(smallIconSize, smallIconSize);
				if (const QComboBox *combo = qobject_cast<const QComboBox*>(widget))
					iconSize = combo->iconSize();
				if (checked)
					pixmap = menuItem->icon.pixmap(iconSize, mode, QIcon::On);
				else
					pixmap = menuItem->icon.pixmap(iconSize, mode);

				QRect pmr(0, 0, pixmap.width(), pixmap.height());
				pmr.moveCenter(vCheckRect.center());
				pmr.moveRight(checkcol);
				painter->setPen(menuItem->palette.text().color());
				painter->drawPixmap(pmr.topLeft(), pixmap);
			}
			if (selected) {
				painter->setPen(mkQColor(ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR)));
			} else {
				painter->setPen(mkQColor(ui_color(B_MENU_ITEM_TEXT_COLOR)));
			}
			if (dis)
				p->setPen(discol);

			int x, y, w, h;
			menuitem->rect.getRect(&x, &y, &w, &h);
			int xm = windowsItemFrame + checkcol + windowsItemHMargin;
			int xpos = menuitem->rect.x() + xm;

			QRect textRect(xpos, y + windowsItemVMargin, w - xm - windowsRightBorder, h - 2 * windowsItemVMargin);
			QRect vTextRect = visualRect(opt->direction, menuitem->rect, textRect);
			if (!menuitem->text.isEmpty()) {
				p->save();
				int tabIdx = menuitem->text.indexOf(QLatin1Char('\t'));
				int text_flags = Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
				if (!styleHint(SH_UnderlineShortcut, menuitem, widget))
					text_flags |= Qt::TextHideMnemonic;
				text_flags |= Qt::AlignLeft;

				QFont font = menuitem->font;
				font.setPointSizeF(QFontInfo(menuItem->font).pointSizeF());

				if (menuitem->menuItemType == QStyleOptionMenuItem::DefaultItem)
					font.setBold(true);

				p->setFont(font);
				p->drawText(vTextRect, text_flags, menuitem->text.left(tabIdx));
				p->restore();
			}
		}
		painter->restore();
		break;
	case CE_MenuHMargin:
	case CE_MenuVMargin:
		break;
	case CE_MenuEmptyArea:
		break;
	case CE_PushButtonLabel:
		if (const QStyleOptionButton *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
			QRect ir = button->rect;
			uint tf = Qt::AlignVCenter;
			if (styleHint(SH_UnderlineShortcut, button, widget))
				tf |= Qt::TextShowMnemonic;
			else
			   tf |= Qt::TextHideMnemonic;

			bool isDown = (option->state & State_Sunken) || (option->state & State_On);

			if (!button->icon.isNull()) {
				//Center both icon and text
				QPoint point;

				QIcon::Mode mode = button->state & State_Enabled ? QIcon::Normal
															  : QIcon::Disabled;
				if (mode == QIcon::Normal && button->state & State_HasFocus)
					mode = QIcon::Active;
				QIcon::State state = QIcon::Off;
				if (button->state & State_On)
					state = QIcon::On;

				QPixmap pixmap = button->icon.pixmap(button->iconSize, mode, state);
				int w = pixmap.width();
				int h = pixmap.height();

				if (!button->text.isEmpty())
					w += button->fontMetrics.boundingRect(option->rect, tf, button->text).width() + 2;

				point = QPoint(ir.x() + ir.width() / 2 - w / 2,
							   ir.y() + ir.height() / 2 - h / 2);

				if (button->direction == Qt::RightToLeft)
					point.rx() += pixmap.width();

				painter->drawPixmap(visualPos(button->direction, button->rect, point), pixmap);

				if (button->direction == Qt::RightToLeft)
					ir.translate(-point.x() - 2, 0);
				else
					ir.translate(point.x() + pixmap.width(), 0);

				// left-align text if there is
				if (!button->text.isEmpty())
					tf |= Qt::AlignLeft;

			} else {
				tf |= Qt::AlignHCenter;
			}

			if (button->features & QStyleOptionButton::HasMenu)
				ir = ir.adjusted(0, 0, -proxy()->pixelMetric(PM_MenuButtonIndicator, button, widget), 0);
			if (isDown)
				ir.adjust(1, 1, 1, 1);
			proxy()->drawItemText(painter, ir, tf, button->palette, (button->state & State_Enabled),
						 button->text, QPalette::ButtonText);
		}
		break;
	case CE_MenuBarEmptyArea:
		painter->save();
		{
			if (be_control_look != NULL) {
				QRect r = rect.adjusted(0,0,0,-1);
				rgb_color base = ui_color(B_MENU_BACKGROUND_COLOR);;
				uint32 flags = 0;
		        BRect bRect(0.0f, 0.0f, r.width() - 1, r.height() - 1);
				TemporarySurface surface(bRect);
				be_control_look->DrawMenuBarBackground(surface.view(), bRect, bRect, base, flags);
				painter->drawImage(r, surface.image());
			}

   	        painter->setPen(QPen(QColor(152,152,152)));
			painter->drawLine(rect.bottomLeft(), rect.bottomRight());
		}
		painter->restore();
		break;
	case CE_TabBarTabShape:
		painter->save();
		if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(option)) {
			bool rtlHorTabs = (tab->direction == Qt::RightToLeft
							   && (tab->shape == QTabBar::RoundedNorth
								   || tab->shape == QTabBar::RoundedSouth));
			bool selected = tab->state & State_Selected;
#if 0
			bool firstTab = ((!rtlHorTabs && tab->position == QStyleOptionTab::Beginning)
							|| (rtlHorTabs && tab->position == QStyleOptionTab::End));
#endif
			bool lastTab = ((!rtlHorTabs && tab->position == QStyleOptionTab::End)
							|| (rtlHorTabs && tab->position == QStyleOptionTab::Beginning));
			bool previousSelected =
				((!rtlHorTabs
				  && tab->selectedPosition == QStyleOptionTab::PreviousIsSelected)
				 || (rtlHorTabs
					 && tab->selectedPosition == QStyleOptionTab::NextIsSelected));

			rgb_color base = mkHaikuColor(option->palette.color( QPalette::Normal, QPalette::Window));
			QColor backgroundColor(option->palette.color( QPalette::Normal, QPalette::Window));
			QColor frameColor(mkQColor(tint_color(base, 1.30)));
			QColor bevelLight(mkQColor(tint_color(base, 0.8)));
			QColor bevelShadow(mkQColor(tint_color(base, 1.03)));

			if (be_control_look != NULL) {
				QRect r = option->rect;
				uint32 flags = 0;
				uint32 side = BControlLook::B_TOP_BORDER;
				uint32 borders = BControlLook::B_RIGHT_BORDER | BControlLook::B_TOP_BORDER | BControlLook::B_BOTTOM_BORDER;
		        BRect bRect(0.0f, 0.0f, r.width() - 1, r.height() - 1);
				TemporarySurface surface(bRect);

				BRect bRect1 = bRect;

				switch (tab->shape) {
					case QTabBar::TriangularNorth:
					case QTabBar::RoundedNorth:
						bRect1.bottom++;
						side = BControlLook::B_TOP_BORDER;
						borders = (lastTab ? BControlLook::B_RIGHT_BORDER : 0) |
								(!previousSelected ? BControlLook::B_LEFT_BORDER : 0) |
								BControlLook::B_TOP_BORDER |
								BControlLook::B_BOTTOM_BORDER;
						if (lastTab || selected)
							bRect1.right++;
						if(!previousSelected || selected)
							bRect1.left--;
		            	break;
	                case QTabBar::TriangularSouth:
					case QTabBar::RoundedSouth:
						bRect1.top--;
						side = BControlLook::B_BOTTOM_BORDER;
						borders = (lastTab ? BControlLook::B_RIGHT_BORDER : 0) |
								(!previousSelected ? BControlLook::B_LEFT_BORDER : 0) |
								BControlLook::B_TOP_BORDER |
								BControlLook::B_BOTTOM_BORDER;
						if (lastTab || selected)
							bRect1.right++;
						if(!previousSelected || selected)
							bRect1.left--;
						break;
 					case QTabBar::TriangularWest:
					case QTabBar::RoundedWest:
						bRect1.right++;
						side = BControlLook::B_LEFT_BORDER;
						borders = (lastTab ? BControlLook::B_BOTTOM_BORDER : 0) |
								(!previousSelected ? BControlLook::B_TOP_BORDER : 0) |
								BControlLook::B_LEFT_BORDER |
								BControlLook::B_RIGHT_BORDER;
						if (lastTab || selected)
							bRect1.bottom++;
						if(!previousSelected || selected)
							bRect1.top--;
	                	break;
		            case QTabBar::TriangularEast:
	                case QTabBar::RoundedEast:
						bRect1.left--;
	                	side = BControlLook::B_RIGHT_BORDER;
						borders = (lastTab ? BControlLook::B_BOTTOM_BORDER : 0) |
								(!previousSelected ? BControlLook::B_TOP_BORDER : 0) |
								BControlLook::B_LEFT_BORDER |
								BControlLook::B_RIGHT_BORDER;
						if (lastTab || selected)
							bRect1.bottom++;
						if(!previousSelected || selected)
							bRect1.top--;
						break;
				}

				if(selected)
					be_control_look->DrawActiveTab(surface.view(), bRect1, bRect, base, flags, BControlLook::B_ALL_BORDERS, side);
				else
					be_control_look->DrawInactiveTab(surface.view(), bRect1, bRect, base, flags, borders, side);

				painter->drawImage(r, surface.image());
			}
		}
		painter->restore();
		break;

	default:
		QCommonStyle::drawControl(element,option,painter,widget);
		break;
	}
}

/*!
  \reimp
*/
QPalette QHaikuStyle::standardPalette () const
{
	QPalette palette;
	rgb_color panel_background_color = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color control_text = ui_color(B_CONTROL_TEXT_COLOR);
	rgb_color control_text_disabled = control_text;
	control_text_disabled.red = (uint8)(((int32)panel_background_color.red + control_text_disabled.red + 1) / 2);
	control_text_disabled.green = (uint8)(((int32)panel_background_color.green + control_text_disabled.green + 1) / 2);
	control_text_disabled.blue = (uint8)(((int32)panel_background_color.blue + control_text_disabled.blue + 1) / 2);

	palette.setBrush(QPalette::Disabled, QPalette::WindowText, mkQColor(ui_color(B_PANEL_TEXT_COLOR)));
	palette.setBrush(QPalette::Disabled, QPalette::Button, mkQColor(ui_color(B_CONTROL_BACKGROUND_COLOR)));
	palette.setBrush(QPalette::Disabled, QPalette::Light, QColor(QRgb(0xffffffff)));
	palette.setBrush(QPalette::Disabled, QPalette::Midlight, QColor(QRgb(0xffffffff)));
	palette.setBrush(QPalette::Disabled, QPalette::Dark, QColor(QRgb(0xff555555)));
	palette.setBrush(QPalette::Disabled, QPalette::Mid, QColor(QRgb(0xffc7c7c7)));
	palette.setBrush(QPalette::Disabled, QPalette::Text, QColor(QRgb(0xffc7c7c7)));
	palette.setBrush(QPalette::Disabled, QPalette::BrightText, QColor(QRgb(0xffffffff)));
	palette.setBrush(QPalette::Disabled, QPalette::ButtonText, mkQColor(control_text_disabled));
	palette.setBrush(QPalette::Disabled, QPalette::Base, QColor(QRgb(0xffefefef)));
	palette.setBrush(QPalette::Disabled, QPalette::AlternateBase, palette.color(QPalette::Disabled, QPalette::Base).darker(110));
	palette.setBrush(QPalette::Disabled, QPalette::Window, mkQColor(panel_background_color));
	palette.setBrush(QPalette::Disabled, QPalette::Shadow, mkQColor(ui_color(B_SHADOW_COLOR)));
	palette.setBrush(QPalette::Disabled, QPalette::Highlight, mkQColor(ui_color(B_PANEL_BACKGROUND_COLOR)));
	palette.setBrush(QPalette::Disabled, QPalette::HighlightedText, mkQColor(ui_color(B_PANEL_TEXT_COLOR)));
	palette.setBrush(QPalette::Disabled, QPalette::Link, QColor(QRgb(0xff0000ee)));
	palette.setBrush(QPalette::Disabled, QPalette::LinkVisited, QColor(QRgb(0xff52188b)));

	palette.setBrush(QPalette::Active, QPalette::WindowText, mkQColor(ui_color(B_PANEL_TEXT_COLOR)));
	palette.setBrush(QPalette::Active, QPalette::Button, mkQColor(ui_color(B_CONTROL_BACKGROUND_COLOR)));
	palette.setBrush(QPalette::Active, QPalette::Light, mkQColor(ui_color(B_SHINE_COLOR)));
	palette.setBrush(QPalette::Active, QPalette::Midlight, QColor(QRgb(0xffffffff)));
	palette.setBrush(QPalette::Active, QPalette::Dark, QColor(QRgb(0xff555555)));
	palette.setBrush(QPalette::Active, QPalette::Mid, QColor(QRgb(0xffc7c7c7)));
	palette.setBrush(QPalette::Active, QPalette::Text, mkQColor(ui_color(B_DOCUMENT_TEXT_COLOR)));
	palette.setBrush(QPalette::Active, QPalette::BrightText, mkQColor(ui_color(B_SHINE_COLOR)));
	palette.setBrush(QPalette::Active, QPalette::ButtonText, mkQColor(control_text));
	palette.setBrush(QPalette::Active, QPalette::Base, mkQColor(ui_color(B_LIST_BACKGROUND_COLOR)));
	palette.setBrush(QPalette::Active, QPalette::AlternateBase, mkQColor(ui_color(B_LIST_BACKGROUND_COLOR)));
	palette.setBrush(QPalette::Active, QPalette::Window, mkQColor(panel_background_color));
	palette.setBrush(QPalette::Active, QPalette::Shadow, mkQColor(ui_color(B_SHADOW_COLOR)));
	palette.setBrush(QPalette::Active, QPalette::Highlight, mkQColor(ui_color(B_LIST_SELECTED_BACKGROUND_COLOR)));
	palette.setBrush(QPalette::Active, QPalette::HighlightedText, mkQColor(ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR)));
	palette.setBrush(QPalette::Active, QPalette::Link, mkQColor(ui_color(B_LINK_TEXT_COLOR)));
	palette.setBrush(QPalette::Active, QPalette::LinkVisited, mkQColor(ui_color(B_LINK_VISITED_COLOR)));
	palette.setBrush(QPalette::Active, QPalette::ToolTipBase, mkQColor(ui_color(B_TOOL_TIP_BACKGROUND_COLOR)));
	palette.setBrush(QPalette::Active, QPalette::ToolTipText, mkQColor(ui_color(B_TOOL_TIP_TEXT_COLOR)));

	palette.setBrush(QPalette::Inactive, QPalette::WindowText, mkQColor(ui_color(B_PANEL_TEXT_COLOR)));
	palette.setBrush(QPalette::Inactive, QPalette::Button, mkQColor(ui_color(B_CONTROL_BACKGROUND_COLOR)));
	palette.setBrush(QPalette::Inactive, QPalette::Light, mkQColor(ui_color(B_SHINE_COLOR)));
	palette.setBrush(QPalette::Inactive, QPalette::Midlight, QColor(QRgb(0xffffffff)));
	palette.setBrush(QPalette::Inactive, QPalette::Dark, QColor(QRgb(0xff555555)));
	palette.setBrush(QPalette::Inactive, QPalette::Mid, QColor(QRgb(0xffc7c7c7)));
	palette.setBrush(QPalette::Inactive, QPalette::Text, mkQColor(ui_color(B_DOCUMENT_TEXT_COLOR)));
	palette.setBrush(QPalette::Inactive, QPalette::BrightText, mkQColor(ui_color(B_SHINE_COLOR)));
	palette.setBrush(QPalette::Inactive, QPalette::ButtonText, mkQColor(control_text));
	palette.setBrush(QPalette::Inactive, QPalette::Base, mkQColor(ui_color(B_LIST_BACKGROUND_COLOR)));
	palette.setBrush(QPalette::Inactive, QPalette::AlternateBase, mkQColor(ui_color(B_LIST_BACKGROUND_COLOR)));
	palette.setBrush(QPalette::Inactive, QPalette::Window, mkQColor(panel_background_color));
	palette.setBrush(QPalette::Inactive, QPalette::Shadow, mkQColor(ui_color(B_SHADOW_COLOR)));
	palette.setBrush(QPalette::Inactive, QPalette::Highlight, mkQColor(ui_color(B_LIST_SELECTED_BACKGROUND_COLOR)));
	palette.setBrush(QPalette::Inactive, QPalette::HighlightedText, mkQColor(ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR)));
	palette.setBrush(QPalette::Inactive, QPalette::Link, mkQColor(ui_color(B_LINK_TEXT_COLOR)));
	palette.setBrush(QPalette::Inactive, QPalette::LinkVisited, mkQColor(ui_color(B_LINK_VISITED_COLOR)));
	return palette;
}

/*!
  \reimp
*/
void QHaikuStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
										 QPainter *painter, const QWidget *widget) const
{
	QColor button = option->palette.button().color();
	QColor dark;
	QColor grooveColor;
	QColor darkOutline;
	dark.setHsv(button.hue(),
				qMin(255, (int)(button.saturation()*1.9)),
				qMin(255, (int)(button.value()*0.7)));
	grooveColor.setHsv(button.hue(),
				qMin(255, (int)(button.saturation()*2.6)),
				qMin(255, (int)(button.value()*0.9)));
	darkOutline.setHsv(button.hue(),
				qMin(255, (int)(button.saturation()*3.0)),
				qMin(255, (int)(button.value()*0.6)));

	QColor alphaCornerColor;
	if (widget) {
		// ### backgroundrole/foregroundrole should be part of the style option
		alphaCornerColor = mergedColors(option->palette.color(widget->backgroundRole()), darkOutline);
	} else {
		alphaCornerColor = mergedColors(option->palette.window().color(), darkOutline);
	}
	QPalette palette = option->palette;

	switch (control) {
	case CC_SpinBox:
		painter->save();
		if (const QStyleOptionSpinBox *spinBox = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
			bool isEnabled = (spinBox->state & State_Enabled);
			/*bool hover = isEnabled && (spinBox->state & State_MouseOver);*/
			bool upIsActive = (spinBox->activeSubControls == SC_SpinBoxUp);
			bool downIsActive = (spinBox->activeSubControls == SC_SpinBoxDown);

			QRect rect = option->rect.adjusted(0, 0, 0, 0);
			BRect bRect(0.0f, 0.0f, rect.width() - 1, rect.height() - 1);
			QRect editRect = proxy()->subControlRect(CC_SpinBox, spinBox, SC_SpinBoxEditField, widget).adjusted(0,0,0,0);
			QRect upRect = proxy()->subControlRect(CC_SpinBox, spinBox, SC_SpinBoxUp, widget);
			QRect downRect = proxy()->subControlRect(CC_SpinBox, spinBox, SC_SpinBoxDown, widget);
		    BRect bEditRect(editRect.left(), editRect.top(), editRect.right(), editRect.bottom());
		    BRect bUpRect(upRect.left(), upRect.top(), upRect.right(), upRect.bottom());
		    BRect bDownRect(downRect.left(), downRect.top(), downRect.right(), downRect.bottom());

			rgb_color base = mkHaikuColor(option->palette.color( QPalette::Normal, QPalette::Button));
			rgb_color bgColor = mkHaikuColor(option->palette.color( QPalette::Normal, QPalette::Window));

			uint32 flags = 0;
			if (!(option->state & State_Enabled))
				flags |= BControlLook::B_DISABLED;
			if (option->state & State_HasFocus)
				flags |= BControlLook::B_FOCUSED;

			TemporarySurface surface(bRect);

			surface.view()->SetViewColor(base);
			surface.view()->SetLowColor(bgColor);
			surface.view()->SetHighColor(bgColor);
			surface.view()->FillRect(bRect);

			if (spinBox->frame) {
				rgb_color baseEdit = ui_color(B_DOCUMENT_BACKGROUND_COLOR);
				bEditRect.InsetBy(-1, -1);
				surface.view()->SetLowColor(baseEdit);
				surface.view()->SetHighColor(baseEdit);
				surface.view()->FillRect(bEditRect);
				be_control_look->DrawTextControlBorder(surface.view(), bEditRect, bEditRect, bgColor, flags);
			}

			if (spinBox->buttonSymbols == QAbstractSpinBox::NoButtons) {
				painter->drawImage(rect, surface.image());
				painter->restore();
				break;
			}

			float frameTint = B_DARKEN_1_TINT;
			float fgTintUp, bgTintUp;
			float fgTintDown, bgTintDown;

			if (!isEnabled)
				fgTintUp = B_DARKEN_1_TINT;
			else if (upIsActive)
				fgTintUp = B_DARKEN_MAX_TINT;
			else
				fgTintUp = 1.777f;

			if (isEnabled && upIsActive)
				bgTintUp = B_DARKEN_1_TINT;
			else
				bgTintUp = B_NO_TINT;

			if (!isEnabled)
				fgTintDown = B_DARKEN_1_TINT;
			else if (downIsActive)
				fgTintDown = B_DARKEN_MAX_TINT;
			else
				fgTintDown = 1.777f;

			if (isEnabled && downIsActive)
				bgTintDown = B_DARKEN_1_TINT;
			else
				bgTintDown = B_NO_TINT;

			if (bgColor.red + bgColor.green + bgColor.blue <= 128 * 3) {
				frameTint = 2.0f - frameTint;
				fgTintUp = 2.0f - fgTintUp;
				bgTintUp = 2.0f - bgTintUp;
				fgTintDown = 2.0f - fgTintDown;
				bgTintDown = 2.0f - bgTintDown;
			}
			uint32 borders = be_control_look->B_TOP_BORDER | be_control_look->B_BOTTOM_BORDER;

			// draw the button
			be_control_look->DrawButtonFrame(surface.view(), bUpRect, bUpRect,
				tint_color(bgColor, frameTint), bgColor, 0, borders | be_control_look->B_RIGHT_BORDER);
			be_control_look->DrawButtonBackground(surface.view(), bUpRect, bUpRect,
				tint_color(bgColor, bgTintUp), 0, borders | be_control_look->B_RIGHT_BORDER);

			be_control_look->DrawButtonFrame(surface.view(), bDownRect, bDownRect,
				tint_color(bgColor, frameTint), bgColor, 0, borders | be_control_look->B_LEFT_BORDER);
			be_control_look->DrawButtonBackground(surface.view(), bDownRect, bDownRect,
				tint_color(bgColor, bgTintDown), 0, borders | be_control_look->B_LEFT_BORDER);

			bUpRect.InsetBy(1, floor(bUpRect.Width() / 3));
			bDownRect.InsetBy(1, floor(bDownRect.Width() / 3));

			if (bUpRect.IntegerWidth() % 2 != 0)
				bUpRect.right += 1;

			if (bUpRect.IntegerHeight() % 2 != 0)
				bUpRect.bottom += 1;

			if (bDownRect.IntegerWidth() % 2 != 0)
				bDownRect.right += 1;

			if (bDownRect.IntegerHeight() % 2 != 0)
				bDownRect.bottom += 1;

			surface.view()->SetHighColor(tint_color(bgColor, fgTintUp));

			float halfHeight = floorf(bUpRect.Height() / 2);
			surface.view()->StrokeLine(BPoint(bUpRect.left, bUpRect.top + halfHeight),
				BPoint(bUpRect.right, bUpRect.top + halfHeight));

			float halfWidth = floorf(bUpRect.Width() / 2);
			surface.view()->StrokeLine(BPoint(bUpRect.left + halfWidth, bUpRect.top + 2),
				BPoint(bUpRect.left + halfWidth, bUpRect.bottom - 2));

			surface.view()->StrokeLine(BPoint(bDownRect.left, bUpRect.top + halfHeight),
				BPoint(bDownRect.right, bUpRect.top + halfHeight));

			painter->drawImage(rect, surface.image());
	    }
		painter->restore();
		break;
	case CC_TitleBar:
		painter->save();
		if (const QStyleOptionTitleBar *titleBar = qstyleoption_cast<const QStyleOptionTitleBar *>(option)) {
			bool active = (titleBar->titleBarState & State_Active);

			int titleBarHeight = proxy()->pixelMetric(PM_TitleBarHeight);
			int frameWidth = proxy()->pixelMetric(PM_MdiSubWindowFrameWidth);

			QRect fullRect = titleBar->rect;

			QPalette palette = option->palette;
			QColor highlight = option->palette.highlight().color();

			QColor titleBarFrameBorder(active ? highlight.darker(180): dark.darker(110));
			QColor titleBarHighlight(active ? highlight.lighter(120): palette.window().color().lighter(120));
			QColor textAlphaColor(active ? 0xffffff : 0xff000000 );

			QColor textColorActive(mkQColor(ui_color(B_WINDOW_TEXT_COLOR)));
		    QColor textColorInactive(mkQColor(ui_color(B_WINDOW_INACTIVE_TEXT_COLOR)));
			QColor textColor(active ? textColorActive : textColorInactive);

			QColor tabColorActive(mkQColor(ui_color(B_WINDOW_TAB_COLOR)));
		    QColor tabColorInactive(mkQColor(ui_color(B_WINDOW_INACTIVE_TAB_COLOR)));

			QColor tabColorActiveLight(mkQColor(tint_color(ui_color(B_WINDOW_TAB_COLOR), (B_LIGHTEN_MAX_TINT + B_LIGHTEN_2_TINT) / 2)));
		    QColor tabColorInactiveLight(mkQColor(tint_color(ui_color(B_WINDOW_INACTIVE_TAB_COLOR), (B_LIGHTEN_MAX_TINT + B_LIGHTEN_2_TINT) / 2)));

			QColor titlebarColor = QColor(active ? tabColorActive : tabColorInactive);
			QColor titlebarColor2 = QColor(active ? tabColorActiveLight : tabColorInactiveLight);

			color_which bcolor = active ? B_WINDOW_BORDER_COLOR : B_WINDOW_INACTIVE_BORDER_COLOR;
			color_which tcolor = active ? B_WINDOW_TAB_COLOR : B_WINDOW_INACTIVE_TAB_COLOR;

			QColor frameColorActive(mkQColor(ui_color(bcolor)));
			QColor bevelShadow1(mkQColor(tint_color(ui_color(bcolor), 1.07)));
			QColor bevelShadow2(mkQColor(tint_color(ui_color(bcolor), B_DARKEN_2_TINT)));
			QColor bevelShadow3(mkQColor(tint_color(ui_color(bcolor), B_DARKEN_3_TINT)));
			QColor bevelLight(mkQColor(tint_color(ui_color(bcolor), B_LIGHTEN_2_TINT)));

			QColor tabColor(mkQColor(ui_color(tcolor)));
			QColor tabBevelLight(mkQColor(tint_color(ui_color(tcolor), B_LIGHTEN_2_TINT)));
			QColor tabShadow(mkQColor(tint_color(ui_color(tcolor), (B_DARKEN_1_TINT + B_NO_TINT) / 2)));
			QColor buttonFrame(mkQColor(tint_color(ui_color(tcolor), B_DARKEN_2_TINT)));

			qt_haiku_draw_windows_frame(painter, fullRect.adjusted(0, titleBarHeight - frameWidth, 0, titleBarHeight - frameWidth),
				active ? B_WINDOW_BORDER_COLOR : B_WINDOW_INACTIVE_BORDER_COLOR,
				BControlLook::B_LEFT_BORDER | BControlLook::B_RIGHT_BORDER | BControlLook::B_TOP_BORDER, false);

			// tab
			QRect tabRect = fullRect;

			QRect textRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarLabel, widget);

			int tabWidth = textRect.width() + mdiTabTextMarginLeft + mdiTabTextMarginRight;
			tabRect.setWidth(tabWidth);

			QLinearGradient gradient(option->rect.topLeft(), option->rect.bottomLeft());
			gradient.setColorAt(0, titlebarColor2);
			gradient.setColorAt(1, titlebarColor);

			painter->setPen(bevelShadow2);
			painter->drawLine(tabRect.topLeft(), tabRect.bottomLeft());
			painter->drawLine(tabRect.topLeft(), tabRect.topRight());
			painter->setPen(bevelShadow3);
			painter->drawLine(tabRect.topRight(), tabRect.bottomRight() - QPoint(0, frameWidth));

			painter->setPen(tabBevelLight);
			painter->drawLine(tabRect.topLeft() + QPoint(1, 1), tabRect.bottomLeft() + QPoint(1, -4));
			painter->drawLine(tabRect.topLeft() + QPoint(1, 1), tabRect.topRight() + QPoint(-1, 1));
			painter->setPen(tabShadow);
			painter->drawLine(tabRect.topRight() + QPoint(-1, 2), tabRect.bottomRight() - QPoint(1, frameWidth - 1));

			painter->fillRect(tabRect.adjusted(2, 2, -2, 2 - frameWidth), gradient);

			// draw title
			QFont font = painter->font();
			painter->setPen(textColor);
			// Note workspace also does elliding but it does not use the correct font
			QString title = QFontMetrics(font).elidedText(titleBar->text, Qt::ElideMiddle, textRect.width() - 14);
			painter->drawText(textRect, title, QTextOption(Qt::AlignLeft | Qt::AlignVCenter));

			// max button
			if ((titleBar->subControls & SC_TitleBarMaxButton) && (titleBar->titleBarFlags & Qt::WindowMaximizeButtonHint) &&
				!(titleBar->titleBarState & Qt::WindowMaximized)) {
				QRect maxButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarMaxButton, widget);
				if (maxButtonRect.isValid()) {
					bool sunken = (titleBar->activeSubControls & SC_TitleBarMaxButton) && (titleBar->state & State_Sunken);

					QRect bigBox = maxButtonRect.adjusted(4, 4, 0, 0);
					QRect smallBox = maxButtonRect.adjusted(0, 0, -7, -7);

					QLinearGradient gradient(bigBox.left(), bigBox.top(), bigBox.right(), bigBox.bottom());
					gradient.setColorAt(sunken?1:0, Qt::white);
					gradient.setColorAt(sunken?0:1, tabColor);

					painter->setPen(buttonFrame);
					painter->fillRect(bigBox, gradient);
					painter->drawRect(bigBox);
					gradient.setStart(smallBox.left(), smallBox.top());
					gradient.setFinalStop(smallBox.right(), smallBox.bottom());
					painter->fillRect(smallBox, gradient);
					painter->drawRect(smallBox);
				}
			}

			// close button
			if ((titleBar->subControls & SC_TitleBarCloseButton) && (titleBar->titleBarFlags & Qt::WindowSystemMenuHint)) {
				QRect closeButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarCloseButton, widget);
				if (closeButtonRect.isValid()) {
					bool sunken = (titleBar->activeSubControls & SC_TitleBarCloseButton) && (titleBar->state & State_Sunken);
					QLinearGradient gradient(closeButtonRect.left(), closeButtonRect.top(), closeButtonRect.right(), closeButtonRect.bottom());
					gradient.setColorAt(sunken?1:0, Qt::white);
					gradient.setColorAt(sunken?0:1, tabColor);
					painter->fillRect(closeButtonRect, gradient);
					painter->setPen(buttonFrame);
					painter->drawRect(closeButtonRect);
				}
			}

			// normalize button
			if ((titleBar->subControls & SC_TitleBarNormalButton) &&
			   (((titleBar->titleBarFlags & Qt::WindowMinimizeButtonHint) &&
			   (titleBar->titleBarState & Qt::WindowMinimized)) ||
			   ((titleBar->titleBarFlags & Qt::WindowMaximizeButtonHint) &&
			   (titleBar->titleBarState & Qt::WindowMaximized)))) {
				QRect normalButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarNormalButton, widget);
				if (normalButtonRect.isValid()) {
					bool sunken = (titleBar->activeSubControls & SC_TitleBarNormalButton) && (titleBar->state & State_Sunken);

					QRect bigBox = normalButtonRect.adjusted(4, 4, 0, 0);
					QRect smallBox = normalButtonRect.adjusted(0, 0, -7, -7);

					QLinearGradient gradient(bigBox.left(), bigBox.top(), bigBox.right(), bigBox.bottom());
					gradient.setColorAt(sunken?1:0, Qt::white);
					gradient.setColorAt(sunken?0:1, tabColor);

					painter->setPen(buttonFrame);
					painter->fillRect(bigBox, gradient);
					painter->drawRect(bigBox);
					gradient.setStart(smallBox.left(), smallBox.top());
					gradient.setFinalStop(smallBox.right(), smallBox.bottom());
					painter->fillRect(smallBox, gradient);
					painter->drawRect(smallBox);
				}
			}
		}
		painter->restore();
		break;
	case CC_ScrollBar:
		painter->save();
		if (const QStyleOptionSlider *scrollBar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
			bool isEnabled = scrollBar->state & State_Enabled;
			/*bool reverse = scrollBar->direction == Qt::RightToLeft;*/
			bool horizontal = scrollBar->orientation == Qt::Horizontal;
			bool sunken = scrollBar->state & State_Sunken;

			bool isBorderHighlighted = false;
			orientation fOrientation = horizontal?B_HORIZONTAL:B_VERTICAL;

			BRect bRect(0.0f, 0.0f, option->rect.width() - 1, option->rect.height() - 1);
			BRect bounds = bRect;
			QRect scrollBarSlider = proxy()->subControlRect(control, scrollBar, SC_ScrollBarSlider, widget);

			if (horizontal)
				scrollBarSlider.adjust(0,1,0,-1);
			else
				scrollBarSlider.adjust(1,0,-1,0);

			BRect scrollBarSliderBRect = BRect(scrollBarSlider.left(), scrollBarSlider.top(),
												scrollBarSlider.right(), scrollBarSlider.bottom());

			rgb_color normal = mkHaikuColor(option->palette.color( QPalette::Normal, QPalette::Window));

			TemporarySurface surface(bRect);
			surface.view()->SetHighColor(tint_color(normal, B_DARKEN_2_TINT));

			if (isBorderHighlighted && isEnabled) {
				rgb_color borderColor = surface.view()->HighColor();
				rgb_color highlightColor = ui_color(B_KEYBOARD_NAVIGATION_COLOR);
				surface.view()->BeginLineArray(4);
				surface.view()->AddLine(BPoint(bounds.left + 1, bounds.bottom),
					BPoint(bounds.right, bounds.bottom), borderColor);
				surface.view()->AddLine(BPoint(bounds.right, bounds.top + 1),
					BPoint(bounds.right, bounds.bottom - 1), borderColor);
				if (fOrientation == B_HORIZONTAL) {
					surface.view()->AddLine(BPoint(bounds.left, bounds.top + 1),
						BPoint(bounds.left, bounds.bottom), borderColor);
				} else {
					surface.view()->AddLine(BPoint(bounds.left, bounds.top),
						BPoint(bounds.left, bounds.bottom), highlightColor);
				}
				if (fOrientation == B_HORIZONTAL) {
					surface.view()->AddLine(BPoint(bounds.left, bounds.top),
						BPoint(bounds.right, bounds.top), highlightColor);
				} else {
					surface.view()->AddLine(BPoint(bounds.left + 1, bounds.top),
						BPoint(bounds.right, bounds.top), borderColor);
				}
				surface.view()->EndLineArray();
			} else
				surface.view()->StrokeRect(bounds);

			bounds.InsetBy(1.0f, 1.0f);

			bool enabled = isEnabled && scrollBar->minimum < scrollBar->maximum;

			rgb_color light, dark, dark1, dark2;
			if (enabled) {
				light = tint_color(normal, B_LIGHTEN_MAX_TINT);
				dark = tint_color(normal, B_DARKEN_3_TINT);
				dark1 = tint_color(normal, B_DARKEN_1_TINT);
				dark2 = tint_color(normal, B_DARKEN_2_TINT);
			} else {
				light = tint_color(normal, B_LIGHTEN_MAX_TINT);
				dark = tint_color(normal, B_DARKEN_2_TINT);
				dark1 = tint_color(normal, B_LIGHTEN_2_TINT);
				dark2 = tint_color(normal, B_LIGHTEN_1_TINT);
			}

			surface.view()->SetDrawingMode(B_OP_OVER);

			BRect thumbBG = bounds;

			if (scrollBar->subControls & SC_ScrollBarSubLine) {
				bool pushed = (scrollBar->activeSubControls & SC_ScrollBarSubLine) && sunken;
				if (fOrientation == B_HORIZONTAL) {
					BRect buttonFrame(bounds.left, bounds.top, bounds.left + bounds.Height(), bounds.bottom);
					qt_haiku_draw_scroll_arrow(surface.view(), ARROW_LEFT, buttonFrame, bRect, enabled, pushed, fOrientation);
				} else {
					BRect buttonFrame(bounds.left, bounds.top, bounds.right, bounds.top + bounds.Width());
					qt_haiku_draw_scroll_arrow(surface.view(), ARROW_UP, buttonFrame, bRect, enabled, pushed, fOrientation);
				}
			}

			if (scrollBar->subControls & SC_ScrollBarAddLine) {
				bool pushed = (scrollBar->activeSubControls & SC_ScrollBarAddLine) && sunken;
				if (fOrientation == B_HORIZONTAL) {
					BRect buttonFrame(bounds.left, bounds.top, bounds.left + bounds.Height(), bounds.bottom);
					buttonFrame.OffsetTo(bounds.right - bounds.Height(), bounds.top);
					qt_haiku_draw_scroll_arrow(surface.view(), ARROW_RIGHT, buttonFrame, bRect, enabled, pushed, fOrientation);
				} else {
					BRect buttonFrame(bounds.left, bounds.top, bounds.right, bounds.top + bounds.Width());
					buttonFrame.OffsetTo(bounds.left, bounds.bottom - bounds.Width());
					qt_haiku_draw_scroll_arrow(surface.view(), ARROW_DOWN, buttonFrame, bRect, enabled, pushed, fOrientation);
				}
			}

			surface.view()->SetDrawingMode(B_OP_COPY);

			if (fOrientation == B_HORIZONTAL) {
				thumbBG.left += bounds.Height() + 1;
				thumbBG.right -= bounds.Height() + 1;
			} else {
				thumbBG.top += bounds.Width() + 1;
				thumbBG.bottom -= bounds.Width() + 1;
			}

			// Draw groove
			if (scrollBar->subControls & SC_ScrollBarGroove) {
				surface.view()->SetHighColor(dark1);
				uint32 flags = 0;
				if (!enabled)
					flags |= BControlLook::B_DISABLED;
				if (fOrientation == B_HORIZONTAL) {
					BRect leftOfThumb(thumbBG.left, thumbBG.top, scrollBarSliderBRect.left - 1, thumbBG.bottom);
					BRect rightOfThumb(scrollBarSliderBRect.right + 1, thumbBG.top, thumbBG.right, thumbBG.bottom);
					be_control_look->DrawScrollBarBackground(surface.view(), leftOfThumb, rightOfThumb, bRect, normal, flags, fOrientation);
				} else {
					BRect topOfThumb(thumbBG.left, thumbBG.top, thumbBG.right, scrollBarSliderBRect.top - 1);
					BRect bottomOfThumb(thumbBG.left, scrollBarSliderBRect.bottom + 1, thumbBG.right, thumbBG.bottom);
					be_control_look->DrawScrollBarBackground(surface.view(), topOfThumb, bottomOfThumb, bRect, normal, flags, fOrientation);
				}
			}
			// Draw scroll thumb
			if (scrollBar->subControls & SC_ScrollBarSlider) {
				rgb_color thumbColor = ui_color(B_SCROLL_BAR_THUMB_COLOR);
				if (enabled) {
					//scrollBarSliderBRect.left++;scrollBarSliderBRect.right--;
					be_control_look->DrawButtonBackground(surface.view(), scrollBarSliderBRect, bRect,
						thumbColor, 0, BControlLook::B_ALL_BORDERS, fOrientation);
				} else {
					if (scrollBar->minimum >= scrollBar->maximum) {
						qt_haiku_draw_disabled_background(surface.view(), thumbBG, light, dark, dark1, fOrientation);
					} else {
						float bgTint = 1.06;
						rgb_color bgLight = tint_color(light, bgTint * 3);
						rgb_color bgShadow = tint_color(dark, bgTint);
						rgb_color bgFill = tint_color(dark1, bgTint);
						if (fOrientation == B_HORIZONTAL) {
							// left of thumb
							BRect besidesThumb(thumbBG);
							besidesThumb.right = scrollBarSliderBRect.left - 1;
							qt_haiku_draw_disabled_background(surface.view(), besidesThumb, bgLight, bgShadow, bgFill, fOrientation);
							// right of thumb
							besidesThumb.left = scrollBarSliderBRect.right + 1;
							besidesThumb.right = thumbBG.right;
							qt_haiku_draw_disabled_background(surface.view(), besidesThumb, bgLight, bgShadow, bgFill, fOrientation);
						} else {
							// above thumb
							BRect besidesThumb(thumbBG);
							besidesThumb.bottom = scrollBarSliderBRect.top - 1;
							qt_haiku_draw_disabled_background(surface.view(), besidesThumb, bgLight, bgShadow, bgFill, fOrientation);
							// below thumb
							besidesThumb.top = scrollBarSliderBRect.bottom + 1;
							besidesThumb.bottom = thumbBG.bottom;
							qt_haiku_draw_disabled_background(surface.view(), besidesThumb, bgLight, bgShadow, bgFill, fOrientation);
						}
						// thumb bevel
						surface.view()->BeginLineArray(4);
							surface.view()->AddLine(BPoint(scrollBarSliderBRect.left, scrollBarSliderBRect.bottom),
									BPoint(scrollBarSliderBRect.left, scrollBarSliderBRect.top), light);
							surface.view()->AddLine(BPoint(scrollBarSliderBRect.left + 1, scrollBarSliderBRect.top),
									BPoint(scrollBarSliderBRect.right, scrollBarSliderBRect.top), light);
							surface.view()->AddLine(BPoint(scrollBarSliderBRect.right, scrollBarSliderBRect.top + 1),
									BPoint(scrollBarSliderBRect.right, scrollBarSliderBRect.bottom), dark2);
							surface.view()->AddLine(BPoint(scrollBarSliderBRect.right - 1, scrollBarSliderBRect.bottom),
									BPoint(scrollBarSliderBRect.left + 1, scrollBarSliderBRect.bottom), dark2);
						surface.view()->EndLineArray();
						// thumb fill
						scrollBarSliderBRect.InsetBy(1.0, 1.0);
						surface.view()->SetHighColor(dark1);
						surface.view()->FillRect(scrollBarSliderBRect);
					}
				}
			}
			painter->drawImage(option->rect, surface.image());
		}
		painter->restore();
		break;
	case CC_ComboBox:
		painter->save();
		if (const QStyleOptionComboBox *comboBox = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
			bool isEnabled = (comboBox->state & State_Enabled);

			BRect bRect(0.0f, 0.0f, comboBox->rect.width() - 1, comboBox->rect.height() - 1);
			TemporarySurface surface(bRect);
			rgb_color base = mkHaikuColor(option->palette.color( QPalette::Normal, QPalette::Window));

			uint32 flags = 0;

			if (!isEnabled)
				flags |= BControlLook::B_DISABLED;
			if (comboBox->state & State_On)
				flags |= BControlLook::B_ACTIVATED;
			if (comboBox->state & State_HasFocus)
				flags |= BControlLook::B_FOCUSED;
			if (comboBox->state & State_Sunken)
				flags |= BControlLook::B_CLICKED;
			if (comboBox->state & State_NoChange)
				flags |= BControlLook::B_DISABLED | BControlLook::B_ACTIVATED;

			BRect bRect2 = bRect;
			bRect2.InsetBy(1, 1);
			be_control_look->DrawMenuFieldBackground(surface.view(), bRect2, bRect2, base, true, flags);
			be_control_look->DrawMenuFieldFrame(surface.view(), bRect, bRect, base, base, flags);

			painter->drawImage(comboBox->rect, surface.image());
		}
		painter->restore();
		break;
	case CC_GroupBox:
		painter->save();
		if (const QStyleOptionGroupBox *groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(option)) {
			QRect textRect = proxy()->subControlRect(CC_GroupBox, groupBox, SC_GroupBoxLabel, widget);
			QRect checkBoxRect = proxy()->subControlRect(CC_GroupBox, groupBox, SC_GroupBoxCheckBox, widget);
			bool flat = groupBox->features & QStyleOptionFrame::Flat;

			if (!flat) {
				if (groupBox->subControls & QStyle::SC_GroupBoxFrame) {
					QStyleOptionFrame frame;
					frame.QStyleOption::operator=(*groupBox);
					frame.features = groupBox->features;
					frame.lineWidth = groupBox->lineWidth;
					frame.midLineWidth = groupBox->midLineWidth;
					frame.rect = proxy()->subControlRect(CC_GroupBox, option, SC_GroupBoxFrame, widget);

					painter->save();
					QRegion region(groupBox->rect);
					bool ltr = groupBox->direction == Qt::LeftToRight;
					region -= checkBoxRect.united(textRect).adjusted(ltr ? -4 : 0, 0, ltr ? 0 : 4, 0);
					if (!groupBox->text.isEmpty() ||  groupBox->subControls & SC_GroupBoxCheckBox)
						painter->setClipRegion(region);
					frame.palette.setBrush(QPalette::Dark, option->palette.mid().color().lighter(110));
					proxy()->drawPrimitive(PE_FrameGroupBox, &frame, painter);
					painter->restore();
				}
			}
			// Draw title
			if ((groupBox->subControls & QStyle::SC_GroupBoxLabel) && !groupBox->text.isEmpty()) {
				if (!groupBox->text.isEmpty()) {
					QColor textColor = groupBox->textColor;
					if (textColor.isValid())
						painter->setPen(textColor);
					const QFont font = groupBox->subControls & SC_GroupBoxCheckBox ?
						QFontDatabase::systemFont(QFontDatabase::GeneralFont) :
						QFontDatabase::systemFont(QFontDatabase::TitleFont);
					painter->setFont(font);
					painter->drawText(textRect, Qt::TextHideMnemonic | Qt::AlignLeft| groupBox->textAlignment, groupBox->text);
				}
			}
			if (groupBox->subControls & SC_GroupBoxCheckBox) {
				QStyleOptionButton box;
				box.QStyleOption::operator=(*groupBox);
				box.rect = checkBoxRect;
				proxy()->drawPrimitive(PE_IndicatorCheckBox, &box, painter, widget);
			}
		}
		painter->restore();
		break;
	case CC_Slider:
   	painter->save();
		if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
			QRect groove = subControlRect(CC_Slider, option, SC_SliderGroove, widget);
			QRect handle = subControlRect(CC_Slider, option, SC_SliderHandle, widget);
			/*QRect ticks = subControlRect(CC_Slider, option, SC_SliderTickmarks, widget);*/

			bool ticksAbove = slider->tickPosition & QSlider::TicksAbove;
			bool ticksBelow = slider->tickPosition & QSlider::TicksBelow;

			orientation orient = slider->orientation == Qt::Horizontal?B_HORIZONTAL:B_VERTICAL;

			if (be_control_look != NULL) {
				QRect r = groove;

				rgb_color base = mkHaikuColor(option->palette.color( QPalette::Normal, QPalette::Window));
				rgb_color fill_color = tint_color(base, B_DARKEN_1_TINT);
				rgb_color shadow_color = tint_color(base, B_DARKEN_2_TINT);

				uint32 flags = 0;

		        BRect bRect(0.0f, 0.0f, option->rect.width() - 1,  option->rect.height() - 1);
				TemporarySurface surface(bRect);

				surface.view()->SetViewColor(base);
				surface.view()->SetHighColor(base);
				surface.view()->SetLowColor(base);
				surface.view()->FillRect(bRect);
				surface.view()->SetHighColor(base);

				if (option->subControls & SC_SliderTickmarks) {
					int mlocation = B_HASH_MARKS_NONE;
					if (ticksAbove)
						mlocation |= B_HASH_MARKS_TOP;
					if (ticksBelow)
						mlocation |= B_HASH_MARKS_BOTTOM;
					int interval =  slider->tickInterval <= 0 ? 1 : slider->tickInterval;
					int num = 1 + ((slider->maximum-slider->minimum) / interval);
					int len = pixelMetric(PM_SliderLength, slider, widget) / 2;
					r = (orient == B_HORIZONTAL) ? option->rect.adjusted(len-1, -2, 1-len, 2) : option->rect.adjusted(0, len, 0, -len);
					BRect bMarksRect = BRect(r.left(), r.top(), r.right(), r.bottom());
					be_control_look->DrawSliderHashMarks(surface.view(), bMarksRect, bMarksRect, base, num, (hash_mark_location)mlocation, flags, orient);
				}
				surface.view()->SetDrawingMode(B_OP_COPY);
				if ((option->subControls & SC_SliderGroove) && groove.isValid()) {
		            QRect gr = groove;
		            if (slider->orientation == Qt::Horizontal) {
						gr.setHeight(7);
						gr.moveTop(slider->rect.center().y() - 3);
					} else {
						gr.setWidth(7);
						gr.moveLeft(slider->rect.center().x() - 3);
					}
					BRect bGrooveRect = BRect(gr.left(), gr.top(), gr.right(), gr.bottom());
					be_control_look->DrawSliderBar(surface.view(), bGrooveRect, bGrooveRect, base, fill_color, flags, orient);
				}
				surface.view()->SetDrawingMode(B_OP_ALPHA);
				if (option->subControls & SC_SliderHandle ) {
					BRect bThumbRect = BRect(handle.left(), handle.top(), handle.right(), handle.bottom());
					if (ticksAbove && !ticksBelow)
						be_control_look->DrawSliderTriangle(surface.view(), bThumbRect, bThumbRect, base, flags, orient);
					else {
						BRect bThumbRect2 = bThumbRect;
						be_control_look->DrawSliderThumb(surface.view(), bThumbRect, bThumbRect, base, flags, orient);
						surface.view()->SetHighColor(shadow_color);
						surface.view()->StrokeLine(bThumbRect2.LeftBottom() + BPoint(1, 0), bThumbRect2.RightBottom());
						surface.view()->StrokeLine(bThumbRect2.RightTop() + BPoint(0, 1), bThumbRect2.RightBottom());
					}
				}
				painter->drawImage(slider->rect, surface.image());
			}
			painter->restore();
		}
		break;
	case CC_Dial:
		if (const QStyleOptionSlider *dial = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
			QPalette pal = widget->palette();
			pal.setColor(QPalette::Active, QPalette::Highlight, mkQColor(ui_color(B_NAVIGATION_BASE_COLOR)));
			if (widget!=NULL)
				((QDial*)widget)->setPalette(pal);
			QStyleHelper::drawDial(dial, painter);
		}
		break;

	default:
		QProxyStyle::drawComplexControl(control, option, painter, widget);
		break;
	}
}

/*!
  \reimp
*/
int QHaikuStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
	int ret = -1;
	switch (metric) {
	case PM_ToolTipLabelFrameWidth:
		ret = 2;
		break;
	case PM_ButtonDefaultIndicator:
		ret = 2;
		break;
	case PM_ButtonShiftHorizontal:
	case PM_ButtonShiftVertical:
		ret = 0;
		break;
	case PM_DialogButtonsSeparator:
		ret = 6;
		break;
	case PM_SplitterWidth:
		ret = 10;
		break;
	case PM_ScrollBarSliderMin:
		ret = 26;
		break;
	case PM_MenuPanelWidth: //menu framewidth
		ret = 0;
		break;
	case PM_TitleBarHeight:
		ret = 24 + 5;
		break;
	case PM_ScrollBarExtent:
		ret = 15;//B_V_SCROLL_BAR_WIDTH;
		break;
	case PM_ScrollView_ScrollBarSpacing:
		ret = 0;
		break;
	case PM_SliderThickness:
		ret = 14;
		break;
	case PM_SliderLength:
		ret = 18;
		break;
	case PM_DockWidgetTitleMargin:
		ret = 1;
		break;
	case PM_MenuBarVMargin:
		ret = 0;
		break;
	case PM_DefaultFrameWidth:
		ret = 2;
		break;
	case PM_MdiSubWindowFrameWidth:
		ret = 5;
		break;
	case PM_SpinBoxFrameWidth:
		ret = 2;
		break;
	case PM_MenuBarItemSpacing:
		ret = 0;
		break;
	case PM_MenuBarHMargin:
		ret = 0;
		break;
	case PM_ToolBarHandleExtent:
		ret = 9;
		break;
	case PM_ToolBarItemSpacing:
		ret = 2;
		break;
	case PM_ToolBarFrameWidth:
		ret = 0;
		break;
	case PM_ToolBarItemMargin:
		ret = 1;
		break;
	case PM_SmallIconSize:
		ret = smallIconsSizeSettings;
		break;
	case PM_LargeIconSize:
		ret = largeIconsSizeSettings;
		break;
	case PM_ButtonIconSize:
		ret = smallIconsSizeSettings;
		break;
	case PM_ToolBarIconSize:
		ret = toolbarIconsSizeSettings;
		break;
	case PM_MessageBoxIconSize:
		ret = largeIconsSizeSettings;
		break;
	case PM_ListViewIconSize:
		ret = smallIconsSizeSettings;
		break;
	case PM_IconViewIconSize:
		ret = largeIconsSizeSettings;
		break;
	case PM_MenuVMargin:
		ret = 1;
		break;
	case PM_MenuHMargin:
		ret = 1;
		break;
	case PM_DockWidgetTitleBarButtonMargin:
		ret = 4;
		break;
	case PM_MaximumDragDistance:
		return -1;
	case PM_TabCloseIndicatorWidth:
	case PM_TabCloseIndicatorHeight:
		return 16;
	case PM_TabBarTabVSpace:
		return 7;
	case PM_TabBarTabHSpace:
		return 14;
	case PM_TabBarTabShiftVertical:
		return 0;
	case PM_TabBarTabShiftHorizontal:
		return 0;
	case PM_TabBarScrollButtonWidth:
		return 16;
	case PM_TabBar_ScrollButtonOverlap:
		return 0;
	case PM_IndicatorWidth:
		return 15;
	case PM_IndicatorHeight:
		return 15;
	case PM_ExclusiveIndicatorWidth:
		return 17;
	case PM_ExclusiveIndicatorHeight:
		return 17;
	case PM_HeaderMargin:
		return 1;
	case PM_TabBarTabOverlap:
		return 16;
//    case PM_TabBarBaseOverlap:
//        return 0;
	default:
		break;
	}

	return ret != -1 ? ret : QProxyStyle::pixelMetric(metric, option, widget);
}

/*!
  \reimp
*/
QSize QHaikuStyle::sizeFromContents(ContentsType type, const QStyleOption *option,
										const QSize &size, const QWidget *widget) const
{
	QSize newSize = QProxyStyle::sizeFromContents(type, option, size, widget);
	switch (type) {
	case CT_PushButton:
		if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
			if (!btn->text.isEmpty()) {
				newSize += QSize(7, 2);
				if(newSize.width() < 64)
					newSize.setWidth(64);
			}
			if (!btn->icon.isNull() && btn->iconSize.height() > 16)
				newSize -= QSize(0, 2);
		}
		break;
	case CT_GroupBox:
		// Since we use a bold font we have to recalculate base width
		if (const QGroupBox *gb = qobject_cast<const QGroupBox*>(widget)) {
			QFont font = gb->font();
			font.setBold(true);
			QFontMetrics metrics(font);
			int baseWidth = metrics.horizontalAdvance(gb->title()) + metrics.horizontalAdvance(QLatin1Char(' '));
			if (gb->isCheckable()) {
				baseWidth += proxy()->pixelMetric(QStyle::PM_IndicatorWidth, option, widget);
				baseWidth += proxy()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing, option, widget);
			}
			newSize.setWidth(qMax(baseWidth, newSize.width()));
		}
		newSize += QSize(0, 1);
		break;
	case CT_RadioButton:
		newSize += QSize(1, 1);
		break;
	case CT_CheckBox:
		newSize += QSize(1, 1);
		break;
	case CT_ToolButton:
		if (widget && qobject_cast<QToolBar *>(widget->parentWidget()))
			newSize += QSize(4, 6);
		break;
	case CT_Slider:
		if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
			if (slider->orientation == Qt::Horizontal)
				newSize += QSize(0, 12);
			else
				newSize += QSize(12, 0);
		}
		break;
	case CT_SpinBox:
		newSize = QSize(newSize.width() * 1.3, newSize.height() + 6);
		break;
	case CT_ComboBox:
		newSize = sizeFromContents(CT_PushButton, option, size, widget);
		newSize.rwidth() += 28;
		newSize.rheight() += 5;
		break;
	case CT_LineEdit:
		newSize -= QSize(0, 4);
		break;
	case CT_MenuBar:
		newSize += QSize(0, 1);
		break;
	case CT_MenuBarItem:
		newSize += QSize(7, -1);
		break;
	case CT_MenuItem:
		if (const QStyleOptionMenuItem *menuItem = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
			if (!menuItem->icon.isNull() && showMenuIcon) {
				if (const QComboBox *combo = qobject_cast<const QComboBox*>(widget)) {
					newSize.setHeight(qMax(combo->iconSize().height() + 2, newSize.height()));
				}
			}
			if (menuItem->menuItemType == QStyleOptionMenuItem::Separator) {
				newSize += QSize(-2, 8);
				break;
			}
			int width = 20; //margins

			int maxpmw = menuItem->maxIconWidth;
			int tabIdx = menuItem->text.indexOf(QLatin1Char('\t'));
			QString itemText = menuItem->text.left(tabIdx);
			QString shotcutText = tabIdx > 0 ? menuItem->text.mid(tabIdx + 1) : "";
			width += maxpmw;
			QFontMetrics fm(menuItem->font);
			width += fm.horizontalAdvance(itemText);

			QStringList shotcutList = shotcutText.split('+');
			if (shotcutText.right(1).contains('+')) {
				shotcutList.removeAll(QString(""));
				shotcutList << "+";
			}
			shotcutList.last().replace("Return","⏎");
			shotcutList.last().replace("Left","←");
			shotcutList.last().replace("Right","→");
			shotcutList.last().replace("Up","↑");
			shotcutList.last().replace("Down","↓");
			shotcutList.last().replace("Tab","↹");

			if (shotcutList.last().size() > 1 && shotcutList.count() > 1)
				shotcutList.clear();

			if (shotcutList.count() > 0) {
				if (shotcutList.contains("Ctrl"))
					width += sMenuItemControl->Bounds().Width() + 1;
				if (shotcutList.contains("Alt"))
					width += sMenuItemAlt->Bounds().Width() + 1;
				if (shotcutList.contains("Meta"))
					width += sMenuItemOption->Bounds().Width() + 1;
				if (shotcutList.contains("Shift"))
					width += sMenuItemShift->Bounds().Width() + 1;
				width += fm.horizontalAdvance(shotcutList.last()) + 4;
			}

			newSize.setHeight(fm.height() + 4);
			width += roundf(newSize.height() * 2 / 3);
			newSize.setWidth(qMax(width, 100));
		}
		break;
	case CT_SizeGrip:
		newSize = QSize(15, 15);
		break;
	case CT_TabBarTab:
		break;
	case CT_MdiControls:
		break;
	default:
		break;
	}
	return newSize;
}

/*!
  \reimp
*/
void QHaikuStyle::polish(QApplication *app)
{
	QProxyStyle::polish(app);
}

/*!
  \reimp
*/
void QHaikuStyle::polish(QWidget *widget)
{
	QProxyStyle::polish(widget);
	if (qobject_cast<QAbstractButton*>(widget)
		|| qobject_cast<QComboBox *>(widget)
		|| qobject_cast<QProgressBar *>(widget)
		|| qobject_cast<QScrollBar *>(widget)
		|| qobject_cast<QSplitterHandle *>(widget)
		|| qobject_cast<QAbstractSlider *>(widget)
		|| qobject_cast<QAbstractSpinBox *>(widget)
		|| (widget->inherits("QDockSeparator"))
		|| (widget->inherits("QDockWidgetSeparator"))
		) {
		widget->setAttribute(Qt::WA_Hover, true);
	}

	if (qobject_cast<QProgressBar *>(widget))
		widget->installEventFilter(this);

	if (qobject_cast<QMdiSubWindow *>(widget))
		widget->installEventFilter(this);

	if (qobject_cast<QSizeGrip *>(widget))
		widget->installEventFilter(this);
}

/*!
  \reimp
*/
void QHaikuStyle::polish(QPalette &pal)
{
	QProxyStyle::polish(pal);
	//this is a workaround for some themes such as Human, where the contrast
	//between text and background is too low.
	QColor highlight = pal.highlight().color();
	QColor highlightText = pal.highlightedText().color();
	if (qAbs(qGray(highlight.rgb()) - qGray(highlightText.rgb())) < 150) {
		if (qGray(highlightText.rgb()) < 128)
			pal.setBrush(QPalette::Highlight, highlight.lighter(145));
	}
}

/*!
  \reimp
*/
void QHaikuStyle::unpolish(QWidget *widget)
{
	QProxyStyle::unpolish(widget);
	if (qobject_cast<QAbstractButton*>(widget)
		|| qobject_cast<QComboBox *>(widget)
		|| qobject_cast<QProgressBar *>(widget)
		|| qobject_cast<QScrollBar *>(widget)
		|| qobject_cast<QSplitterHandle *>(widget)
		|| qobject_cast<QAbstractSlider *>(widget)
		|| qobject_cast<QAbstractSpinBox *>(widget)
		|| (widget->inherits("QDockSeparator"))
		|| (widget->inherits("QDockWidgetSeparator"))
		) {
		widget->setAttribute(Qt::WA_Hover, false);
	}
	if (qobject_cast<QProgressBar *>(widget))
		widget->removeEventFilter(this);

	if (qobject_cast<QMdiSubWindow *>(widget))
		widget->removeEventFilter(this);

	if (qobject_cast<QSizeGrip *>(widget))
		widget->removeEventFilter(this);
}

/*!
  \reimp
*/
void QHaikuStyle::unpolish(QApplication *app)
{
	QProxyStyle::unpolish(app);
}

/*!
  \reimp
*/
bool QHaikuStyle::event(QEvent *event)
{
	switch (event->type()) {
	case QEvent::Timer: {
		QTimerEvent *timerEvent = reinterpret_cast<QTimerEvent *>(event);
		if (timerEvent->timerId() == animateTimer) {
			Q_ASSERT(progressAnimationFps > 0);
			animateStep = startTime.elapsed() / (1000 / progressAnimationFps);
			foreach (QProgressBar *bar, animatedProgressBars)
				bar->update();
		}
		event->ignore();
	}
	default:
		break;
	}

	return QProxyStyle::event(event);
}

/*!
  \reimp
*/
bool QHaikuStyle::eventFilter(QObject *o, QEvent *e)
{
	switch (e->type())
	{
#ifndef _QS_HAIKU_TAB_FIX_WIDTH_
	case QEvent::WindowTitleChange:
		if(QMdiSubWindow* w = qobject_cast<QMdiSubWindow*>(o)) {
			QStyleHintReturnMask mask;

			QStyleOptionTitleBar titleBarOptions;

			titleBarOptions.initFrom(w);
		    titleBarOptions.subControls = QStyle::SC_All;
			titleBarOptions.titleBarFlags = w->windowFlags();
			titleBarOptions.titleBarState = w->windowState();
			titleBarOptions.state = QStyle::State_Active;
			titleBarOptions.titleBarState = QStyle::State_Active;
		    titleBarOptions.rect = QRect(0, 0, w->width(), w->height());
		    titleBarOptions.version = 2;	//TODO: ugly dirty hack for 10px mask error

			if (!w->windowTitle().isEmpty()) {
		        titleBarOptions.text = w->windowTitle();
		        QFont font = QApplication::font("QMdiSubWindowTitleBar");
		        font.setBold(true);
				titleBarOptions.fontMetrics = QFontMetrics(font);
		    }

			if (styleHint(SH_WindowFrame_Mask, &titleBarOptions, w, &mask))
				w->setMask(mask.region);

			w->repaint();
		}
		break;
#endif
	case QEvent::StyleChange:
	case QEvent::Paint:
	case QEvent::Show:
		if (QProgressBar *bar = qobject_cast<QProgressBar *>(o)) {
			if (bar->minimum() == bar->maximum())
				startProgressAnimation(this, bar);
			else
				stopProgressAnimation(this, bar);
		}
		if (QSizeGrip *grip = qobject_cast<QSizeGrip *>(o)) {
			if (grip->window()->isTopLevel()) {
				QWindow *mainwindow = grip->window()->windowHandle();
				if (mainwindow)
					mainwindow->setProperty("size-grip", true);
			}
		}
		break;
	case QEvent::Destroy:
	case QEvent::Hide:
		stopProgressAnimation(this, static_cast<QProgressBar *>(o));
		if (QSizeGrip *grip = qobject_cast<QSizeGrip *>(o)) {
			if (grip->window()->isTopLevel()) {
				QWindow *mainwindow = grip->window()->windowHandle();
				if (mainwindow)
					mainwindow->setProperty("size-grip", false);
			}
		}
		break;
	default:
		break;
	}
	return QProxyStyle::eventFilter(o, e);
}

void QHaikuStyle::startProgressAnimation(QObject *o, QProgressBar *bar)
{
	if (!animatedProgressBars.contains(bar)) {
		animatedProgressBars << bar;
		if (!animateTimer) {
			Q_ASSERT(progressAnimationFps > 0);
			animateStep = 0;
			startTime.start();
			animateTimer = o->startTimer(1000 / progressAnimationFps);
		}
	}
}

void QHaikuStyle::stopProgressAnimation(QObject *o, QProgressBar *bar)
{
	if (!animatedProgressBars.isEmpty()) {
		animatedProgressBars.removeOne(bar);
		if (animatedProgressBars.isEmpty() && animateTimer) {
			o->killTimer(animateTimer);
			animateTimer = 0;
		}
	}
}

/*!
  \reimp
*/
QRect QHaikuStyle::subControlRect(ComplexControl control, const QStyleOptionComplex *option,
									   SubControl subControl, const QWidget *widget) const
{
	QRect rect = QProxyStyle::subControlRect(control, option, subControl, widget);

	switch (control) {
	case CC_Slider:
		if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
			bool ticksAbove = slider->tickPosition & QSlider::TicksAbove;
			bool ticksBelow = slider->tickPosition & QSlider::TicksBelow;
			/*int tickSize = proxy()->pixelMetric(PM_SliderTickmarkOffset, option, widget);*/

			QPoint grooveCenter = slider->rect.center();

			switch (subControl) {
			case SC_SliderHandle: {
				if (ticksAbove && !ticksBelow) {
	                if (slider->orientation == Qt::Horizontal) {
	                    rect.setHeight(8);
	                    rect.setWidth(12);
	                    int centerY = grooveCenter.y();
	                    rect.moveTop(centerY);
	                } else {
	                    rect.setWidth(8);
	                    rect.setHeight(12);
	                    int centerX = grooveCenter.x();
	                    rect.moveLeft(centerX);
	                }
				} else {
	                if (slider->orientation == Qt::Horizontal) {
	                    rect.setHeight(proxy()->pixelMetric(PM_SliderThickness));
	                    rect.setWidth(proxy()->pixelMetric(PM_SliderLength));
	                    int centerY = slider->rect.center().y() - rect.height() / 2;
	                    rect.moveTop(centerY + 1);
	                } else {
	                    rect.setWidth(proxy()->pixelMetric(PM_SliderThickness));
	                    rect.setHeight(proxy()->pixelMetric(PM_SliderLength));
	                    int centerX = slider->rect.center().x() - rect.width() / 2;
	                    rect.moveLeft(centerX + 1);
	                }
				}
				break;
			}
			case SC_SliderGroove: {
				rect.moveCenter(grooveCenter);
				if (slider->orientation == Qt::Horizontal)
					rect.adjust(0, -3, 0, -3);
				else
					rect.adjust(-3, 0, -3, 0);
				break;
			}
			case SC_SliderTickmarks:
				break;
			default:
				break;
			}
		}
		break;
	case CC_SpinBox:
		if (const QStyleOptionSpinBox *spinbox = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
			int frameWidth = spinbox->frame ? proxy()->pixelMetric(PM_SpinBoxFrameWidth, spinbox, widget) : 0;
			if(option->rect.height()-(frameWidth*2) < 16)
				frameWidth = -(16 - option->rect.height()) / 2;
			QRect frame = spinbox->rect.adjusted(0, frameWidth, 0, -frameWidth);
			QSize buttonSize = QSize(frame.height() * 0.6, frame.height() - 2);

			switch (subControl) {
			case SC_SpinBoxUp:
				if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons)
					return QRect();
				rect = QRect(spinbox->rect.right() - buttonSize.width(), frame.top() - 3, buttonSize.width() + 1, buttonSize.height());
				break;
			case SC_SpinBoxDown:
				if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons)
					return QRect();
				rect = QRect((spinbox->rect.right() - buttonSize.width() * 2) - 1, frame.top() - 3, buttonSize.width() + 1, buttonSize.height());
				break;
			case SC_SpinBoxEditField:
				if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons) {
					rect = frame.adjusted(0, 0, 0, 2);
				} else {
					rect = QRect(frame.left(), frame.top() - 2, (spinbox->rect.width() - buttonSize.width() * 2) - 3, buttonSize.height() - 2);
				}
				break;
			case SC_SpinBoxFrame:
				rect = spinbox->rect.adjusted(0, 0, 0, 4);
			default:
				break;
			}
			rect = visualRect(spinbox->direction, spinbox->rect, rect).adjusted(0, 0, 0, 4);
		}
		break;
	case CC_GroupBox:
		if (const QStyleOptionGroupBox * groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(option)) {
			rect = option->rect;
			int topMargin = 10;
			int topHeight = 14;
			QRect frameRect = rect;
			frameRect.setTop(topMargin);

			if (subControl == SC_GroupBoxFrame)
				return frameRect;
			else if (subControl == SC_GroupBoxContents) {
				int margin = 1;
				int leftMarginExtension = 8;
				return frameRect.adjusted(leftMarginExtension + margin, margin + topHeight, -margin, -margin);
			}

			QFontMetrics fontMetrics = option->fontMetrics;
			if (qobject_cast<const QGroupBox *>(widget)) {
				//Prepare metrics for a bold font
				const QFont font = groupBox->subControls & SC_GroupBoxCheckBox ?
					QFontDatabase::systemFont(QFontDatabase::GeneralFont) :
					QFontDatabase::systemFont(QFontDatabase::TitleFont);
				fontMetrics = QFontMetrics(font);
			} else if (QStyleHelper::isInstanceOf(groupBox->styleObject, QAccessible::Grouping)) {
				QVariant var = groupBox->styleObject->property("font");
				if (var.isValid() && var.canConvert<QFont>()) {
					const QFont font = groupBox->subControls & SC_GroupBoxCheckBox ?
						QFontDatabase::systemFont(QFontDatabase::GeneralFont) :
						QFontDatabase::systemFont(QFontDatabase::TitleFont);
					fontMetrics = QFontMetrics(font);
				}
			}

			QSize textRect = fontMetrics.boundingRect(groupBox->text).size() + QSize(4, 1);
			int indicatorWidth = proxy()->pixelMetric(PM_IndicatorWidth, option, widget);
			int indicatorHeight = proxy()->pixelMetric(PM_IndicatorHeight, option, widget);

			if (subControl == SC_GroupBoxCheckBox) {
				rect.setWidth(indicatorWidth);
				rect.setHeight(indicatorHeight);
				rect.moveTop((textRect.height() - indicatorHeight) / 2);
				rect.adjust(8, 2, 8, 2);
			} else if (subControl == SC_GroupBoxLabel) {
				rect.adjust(10, 2, 0, 2);
				if (groupBox->subControls & SC_GroupBoxCheckBox) {
					rect.adjust(indicatorWidth + 4, 0, 0, 0);
				}
				rect.setSize(textRect);
			}
			rect = visualRect(option->direction, option->rect, rect);
		}

		return rect;
	case CC_ComboBox:
		switch (subControl) {
		case SC_ComboBoxArrow:
			rect = visualRect(option->direction, option->rect, rect);
			rect.setRect(rect.right() - 9, rect.top() - 2,
						 10, rect.height() + 4);
			rect = visualRect(option->direction, option->rect, rect);
			break;
		case SC_ComboBoxEditField: {
			int frameWidth = proxy()->pixelMetric(PM_DefaultFrameWidth);
			rect = visualRect(option->direction, option->rect, rect);
			rect.setRect(option->rect.left() + frameWidth, option->rect.top() + frameWidth + 1,
						 option->rect.width() - 10 - 2 * frameWidth,
						 option->rect.height() - 2 * frameWidth - 2);
			if (const QStyleOptionComboBox *box = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
				if (!box->editable) {
					rect.adjust(6, 0, 0, 0);
					if (box->state & (State_Sunken | State_On))
						rect.translate(1, 1);
				}
			}
			rect = visualRect(option->direction, option->rect, rect);
			break;
		}
		default:
			break;
		}
		break;
	case CC_TitleBar:
		if (const QStyleOptionTitleBar *tb = qstyleoption_cast<const QStyleOptionTitleBar *>(option)) {
			SubControl sc = subControl;
			QRect &ret = rect;
			const int controlHeight = 15 ;

			bool isMinimized = tb->titleBarState & Qt::WindowMinimized;
			bool isMaximized = tb->titleBarState & Qt::WindowMaximized;

			QFontMetrics fontMetrics = option->fontMetrics;
			if (widget) {
				QFont font = widget->font();
				font.setBold(true);
				fontMetrics = QFontMetrics(font);
			}

#ifdef _QS_HAIKU_TAB_FIX_WIDTH_
			int textWidth = mdiTabWidthFix;
#else
			int textWidth = mdiTabWidthMin;
			if (!tb->text.isEmpty()) {
				textWidth = fontMetrics.width(tb->text) + 20;
	            if (tb->version == 2)		//TODO: ugly dirty hack for 10px mask error
		        	textWidth -= 10;
				if (textWidth < mdiTabWidthMin)
					textWidth = mdiTabWidthMin;
			}
#endif
			int tabWidth = textWidth + mdiTabTextMarginLeft +  mdiTabTextMarginRight;

			if (tabWidth > tb->rect.width())
				tabWidth = tb->rect.width();

			switch (sc) {
			case SC_TitleBarLabel:
				if (tb->titleBarFlags & (Qt::WindowTitleHint | Qt::WindowSystemMenuHint)) {
					ret = tb->rect;
					ret.adjust(0, 2, 0, -2);
					ret.setWidth(tabWidth - (mdiTabTextMarginLeft + mdiTabTextMarginRight));
					ret.adjust(mdiTabTextMarginLeft, 0, mdiTabTextMarginLeft, -1);
				} else
					ret = QRect();
				break;
			case SC_TitleBarNormalButton:
				if ( (isMinimized && (tb->titleBarFlags & Qt::WindowMinimizeButtonHint)) ||
					 (isMaximized && (tb->titleBarFlags & Qt::WindowMaximizeButtonHint))) {
					ret = tb->rect;
					ret.adjust(0, 2, 0, -2);
					ret.setRect(ret.left() + tabWidth - mdiTabTextMarginRight,  tb->rect.top() + 5, controlHeight, controlHeight);
				}
				break;
			case SC_TitleBarMaxButton:
				if (!isMaximized && (tb->titleBarFlags & Qt::WindowMaximizeButtonHint)) {
					ret = tb->rect;
					ret.adjust(0, 2, 0, -2);
					ret.setRect(ret.left() + tabWidth - mdiTabTextMarginRight,  tb->rect.top() + 5, controlHeight, controlHeight);
				}
				break;
			case SC_TitleBarContextHelpButton:
			case SC_TitleBarMinButton:
			case SC_TitleBarShadeButton:
			case SC_TitleBarUnshadeButton:
			case SC_TitleBarSysMenu:
				ret = QRect();
				break;
			case SC_TitleBarCloseButton:
				ret.setRect(tb->rect.left() + 5, tb->rect.top() + 5, controlHeight, controlHeight);
				break;
			default:
				break;
			}
			ret = visualRect(tb->direction, tb->rect, ret);
		}
		break;
	default:
		break;
	}

	return rect;
}


/*!
  \reimp
*/
QRect QHaikuStyle::itemPixmapRect(const QRect &r, int flags, const QPixmap &pixmap) const
{
	return QProxyStyle::itemPixmapRect(r, flags, pixmap);
}

/*!
  \reimp
*/
void QHaikuStyle::drawItemPixmap(QPainter *painter, const QRect &rect,
							int alignment, const QPixmap &pixmap) const
{
	QProxyStyle::drawItemPixmap(painter, rect, alignment, pixmap);
}

/*!
  \reimp
*/
QStyle::SubControl QHaikuStyle::hitTestComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
							  const QPoint &pt, const QWidget *w) const
{
	return QProxyStyle::hitTestComplexControl(cc, opt, pt, w);
}

/*!
  \reimp
*/
QPixmap QHaikuStyle::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap,
										const QStyleOption *opt) const
{
	return QProxyStyle::generatedIconPixmap(iconMode, pixmap, opt);
}

/*!
  \reimp
*/
int QHaikuStyle::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
							   QStyleHintReturn *returnData) const
{
	int ret = 0;
	switch (hint) {
	case SH_ScrollBar_MiddleClickAbsolutePosition:
		ret = int(true);
		break;
	case SH_EtchDisabledText:
		ret = int(false);
		break;
	case SH_UnderlineShortcut:
		ret = int(false);
		break;
	case SH_Menu_AllowActiveAndDisabled:
		ret = false;
		break;
	case SH_MainWindow_SpaceBelowMenuBar:
		ret = -24;
		break;
	case SH_MenuBar_MouseTracking:
		ret = 1;
		break;
	case SH_TitleBar_AutoRaise:
		ret = 1;
		break;
	case SH_TitleBar_NoBorder:
		ret = 1;
		break;
	case SH_LineEdit_PasswordCharacter:
		ret = 0x00B7;
		break;
	case SH_ItemView_ShowDecorationSelected:
		ret = true;
		break;
	case SH_Table_GridLineColor:
		if (option) {
			ret = option->palette.window().color().darker(120).rgb();
		}
		break;
	case SH_ComboBox_Popup:
		if (const QStyleOptionComboBox *cmb = qstyleoption_cast<const QStyleOptionComboBox *>(option))
			ret = !cmb->editable;
		else
			ret = 0;
		break;
	case SH_WindowFrame_Mask:
		ret = 1;
		if (QStyleHintReturnMask *mask = qstyleoption_cast<QStyleHintReturnMask *>(returnData)) {
			if (const QStyleOptionTitleBar *titleBar = qstyleoption_cast<const QStyleOptionTitleBar *>(option)) {
				QRect textRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarLabel, widget);
		   		int frameWidth = proxy()->pixelMetric(PM_MdiSubWindowFrameWidth);
				int tabHeight = pixelMetric(PM_TitleBarHeight, titleBar, widget) - frameWidth;
				int tabWidth = textRect.width() + mdiTabTextMarginLeft + mdiTabTextMarginRight;
		   		mask->region = option->rect;
		   		mask->region -= QRect(tabWidth, option->rect.top(), option->rect.width()-tabWidth, tabHeight);
			}
		}
		break;
	case SH_MessageBox_TextInteractionFlags:
		ret = Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse;
		break;
	case SH_DialogButtonBox_ButtonsHaveIcons:
		ret = false;
		break;
	case SH_MessageBox_CenterButtons:
		ret = false;
		break;
#ifndef QT_NO_WIZARD
	case SH_WizardStyle:
		ret = QWizard::ClassicStyle;
		break;
#endif
	case SH_ItemView_ArrowKeysNavigateIntoChildren:
		ret = false;
		break;
	case SH_Menu_SubMenuPopupDelay:
		ret = 225;
		break;
	case SH_ScrollBar_Transient:
		ret = false;
		break;
	case SH_ScrollView_FrameOnlyAroundContents:
		ret = 1;
		break;
	case SH_ToolButtonStyle:
		ret = toolbarIconMode;
		break;
	default:
		ret = QProxyStyle::styleHint(hint, option, widget, returnData);
		break;
	}
	return ret;
}

/*! \reimp */
QRect QHaikuStyle::subElementRect(SubElement sr, const QStyleOption *opt, const QWidget *w) const
{
	QRect r = QProxyStyle::subElementRect(sr, opt, w);
	switch (sr) {
	case SE_SpinBoxLayoutItem:
		break;
	case SE_TabWidgetTabBar:
		if (const QStyleOptionTabWidgetFrame *twf = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(opt)) {
			if (twf->shape == QTabBar::TriangularNorth || twf->shape == QTabBar::RoundedNorth ||
				twf->shape == QTabBar::TriangularSouth || twf->shape == QTabBar::RoundedSouth)
				r.adjust(16, 1, 16, 1);
			else
				r.adjust(1, 1, 1, 1);
		}
		break;
	case SE_TabBarTabText:
		{
			if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(opt)) {
				bool selected = tab->state & State_Selected;
				switch (tab->shape) {
					case QTabBar::RoundedNorth:
					case QTabBar::TriangularNorth:
						{
							r.translate(0, 2);
							if (selected)
								r.translate(0, -1);
						}
						break;
					case QTabBar::RoundedSouth:
					case QTabBar::TriangularSouth:
						{
							r.translate(0, -3);
							if (selected)
								r.translate(0, 1);
						}
						break;
					case QTabBar::RoundedWest:
					case QTabBar::TriangularWest:
					case QTabBar::RoundedEast:
					case QTabBar::TriangularEast:
						{
							r.translate(0, 1);
							if (selected)
								r.translate(0, -1);
						}
						break;
				}
			}
		}
		break;
	case SE_TabBarTabLeftButton:
	case SE_TabBarTabRightButton:
		{
			if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(opt)) {
				bool selected = tab->state & State_Selected;
				switch (tab->shape) {
					case QTabBar::RoundedNorth:
					case QTabBar::TriangularNorth:
						{
							r.translate(0, 2);
							if (selected)
								r.translate(0, -1);
						}
						break;
					case QTabBar::RoundedSouth:
					case QTabBar::TriangularSouth:
						{
							r.translate(0, -3);
							if (selected)
								r.translate(0, 1);
						}
						break;
					case QTabBar::RoundedWest:
					case QTabBar::TriangularWest:
						{
							r.translate(3, 0);
							if (selected)
								r.translate(-1, 0);
						}
						break;
					case QTabBar::RoundedEast:
					case QTabBar::TriangularEast:
						{
							r.translate(-2, 0);
							if (selected)
								r.translate(1, 0);
						}
						break;
				}
			}
		}
		break;
	case SE_PushButtonFocusRect:
		r.adjust(0, 1, 0, -1);
		break;
	case SE_ProgressBarContents:
		r = subElementRect(SE_ProgressBarGroove, opt, w).adjusted(0,2,0,1);
		break;
	default:
		break;
	}
	return r;
}

/*!
	\reimp
*/
QIcon QHaikuStyle::standardIcon(StandardPixmap standardIcon, const QStyleOption *option,
									 const QWidget *widget) const
{
	return QProxyStyle::standardIcon(standardIcon, option, widget);
}

/*!
 \reimp
 */
QPixmap QHaikuStyle::standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt,
									  const QWidget *widget) const
{
	QPixmap pixmap;
	uint32 atype = B_EMPTY_ALERT;

	switch (standardPixmap) {
	case SP_MessageBoxQuestion:
		atype = B_IDEA_ALERT;
		break;
	case SP_MessageBoxInformation:
		atype = B_INFO_ALERT;
		break;
	case SP_MessageBoxCritical:
		atype = B_STOP_ALERT;
		break;
	case SP_MessageBoxWarning:
		atype = B_WARNING_ALERT;
		break;
	default:
		break;
	}

	if(	atype != B_EMPTY_ALERT ) {
		int size = pixelMetric(PM_MessageBoxIconSize, opt, widget);
   		QImage image = get_haiku_alert_icon(atype, size);
   		if(!image.isNull()) {
			pixmap = QPixmap::fromImage(image);
   			return pixmap;
   		}
   	}

#ifndef QT_NO_IMAGEFORMAT_XPM
	switch (standardPixmap) {
	case SP_TitleBarNormalButton:
		return QPixmap((const char **)dock_widget_restore_xpm);
	case SP_TitleBarMinButton:
		return QPixmap((const char **)workspace_minimize);
	case SP_TitleBarCloseButton:
	case SP_DockWidgetCloseButton:
		return QPixmap((const char **)dock_widget_close_xpm);

	default:
		break;
	}
#endif //QT_NO_IMAGEFORMAT_XPM


	return QProxyStyle::standardPixmap(standardPixmap, opt, widget);
}

QT_END_NAMESPACE
