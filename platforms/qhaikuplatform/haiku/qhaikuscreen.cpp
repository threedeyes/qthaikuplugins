/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2015-2022 Gerasim Troeglazov,
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

#include "qhaikucommon.h"
#include "qhaikuwindow.h"
#include "qhaikucursor.h"
#include "qhaikuintegration.h"

#include <QtGui/private/qpixmap_raster_p.h>
#include <QtGui/private/qguiapplication_p.h>

#include <qpa/qplatformcursor.h>
#include <qpa/qplatformwindow.h>

#include <qdebug.h>

QT_BEGIN_NAMESPACE

QHaikuScreen::QHaikuScreen()
	: QPlatformScreen()
    , m_cursor(new QHaikuCursor)
	, m_screen(new BScreen(B_MAIN_SCREEN_ID))
{
	Q_ASSERT(m_screen->IsValid());
}


QHaikuScreen::~QHaikuScreen()
{
    delete m_cursor;
    delete m_screen;
}


QPlatformCursor *QHaikuScreen::cursor() const
{
    return m_cursor;
}


QRect QHaikuScreen::geometry() const
{	
	const BRect frame = m_screen->Frame();
    return QRect(frame.left, frame.top, frame.Width() + 1, frame.Height() + 1);
}


QPixmap QHaikuScreen::grabWindow(WId id, int x, int y, int width, int height) const
{
    QRect rect(x, y, width, height);
	
    QHaikuWindow *window = QHaikuWindow::windowForWinId(id);

    if (!window || window->window()->type() == Qt::Desktop) {
    	if (width < 0 || height < 0) {
    		width = geometry().width() - x;
    		height = geometry().height() - y;
    	}
    	BBitmap *bitmap = NULL;
    	BRect rect(x, y, x + width, y + height);
    	m_screen->GetBitmap(&bitmap, false, &rect);
    	if (bitmap->IsValid()) {
    		QImage image((uchar*)bitmap->Bits(), \
    			width, height, \
    			bitmap->BytesPerRow(), \
    			QImage::Format_RGB32);
    		image = image.copy(QRect(0, 0, width, height));
    		delete bitmap;
    		return QPixmap::fromImage(image);
    	}
		return QPixmap();
    }

    QHaikuBackingStore *store = QHaikuBackingStore::backingStoreForWinId(id);
    if (store)
        return store->grabWindow(id, rect);

    return QPixmap();
}

QPlatformScreen::SubpixelAntialiasingType QHaikuScreen::subpixelAntialiasingTypeHint() const
{
    QPlatformScreen::SubpixelAntialiasingType type = QPlatformScreen::subpixelAntialiasingTypeHint();

    if (type == QPlatformScreen::Subpixel_None) {
		bool subpixel = false;
		get_subpixel_antialiasing(&subpixel);

		if (subpixel)
            type = QPlatformScreen::Subpixel_RGB;
    }

    return type;
}


QSizeF QHaikuScreen::physicalSize() const
{
    int dpi = 100;

	BScreen screen(B_MAIN_SCREEN_ID);
	monitor_info info;

	if (screen.GetMonitorInfo(&info) == B_OK) {
		double x = info.width / 2.54;
		double y = info.height / 2.54;
		if (x > 0 && y > 0)
			dpi = (int32)round((screen.Frame().Width() / x + screen.Frame().Height() / y) / 2);
	}

    return QSizeF(geometry().size()) / dpi * qreal(25.4);
}

QDpi QHaikuScreen::logicalDpi() const
{
    return QDpi(72, 72);
}

qreal QHaikuScreen::pixelDensity() const
{
    return 1.0;
}

Qt::ScreenOrientation QHaikuScreen::nativeOrientation() const
{
    return Qt::PrimaryOrientation;
}

Qt::ScreenOrientation QHaikuScreen::orientation() const
{
    return Qt::PrimaryOrientation;
}

QT_END_NAMESPACE
