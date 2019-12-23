/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2015-2017 Gerasim Troeglazov,
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

#include <QtGui/private/qpixmap_raster_p.h>
#include <QtGui/private/qguiapplication_p.h>

#include <qpa/qplatformcursor.h>
#include <qpa/qplatformwindow.h>

#include <QUuid>
#include <QCryptographicHash> 
#include <QBitmap>
#include <QPixmap>
#include <QImage>
#include <QColor>
#include <QFile> 
#include <QDebug>

#include <Application.h>

#include "qhaikucursor.h"

QT_BEGIN_NAMESPACE

QHaikuCursor::QHaikuCursor()
{
	m_cursorHash.insert(Qt::ArrowCursor, new BCursor(B_CURSOR_ID_SYSTEM_DEFAULT));
	m_cursorHash.insert(Qt::UpArrowCursor, new BCursor(B_CURSOR_ID_RESIZE_NORTH));
	m_cursorHash.insert(Qt::CrossCursor, new BCursor(B_CURSOR_ID_CROSS_HAIR));
	m_cursorHash.insert(Qt::WaitCursor, new BCursor(B_CURSOR_ID_PROGRESS));
	m_cursorHash.insert(Qt::IBeamCursor, new BCursor(B_CURSOR_ID_I_BEAM));
	m_cursorHash.insert(Qt::SizeVerCursor, new BCursor(B_CURSOR_ID_RESIZE_NORTH_SOUTH));
	m_cursorHash.insert(Qt::SizeHorCursor, new BCursor(B_CURSOR_ID_RESIZE_EAST_WEST));
	m_cursorHash.insert(Qt::SizeBDiagCursor, new BCursor(B_CURSOR_ID_RESIZE_NORTH_EAST_SOUTH_WEST));
	m_cursorHash.insert(Qt::SizeFDiagCursor, new BCursor(B_CURSOR_ID_RESIZE_NORTH_WEST_SOUTH_EAST));
	m_cursorHash.insert(Qt::SizeAllCursor, new BCursor(B_CURSOR_ID_MOVE));
	m_cursorHash.insert(Qt::BlankCursor, new BCursor(B_CURSOR_ID_NO_CURSOR));
	m_cursorHash.insert(Qt::SplitVCursor, new BCursor(B_CURSOR_ID_RESIZE_NORTH_SOUTH));
	m_cursorHash.insert(Qt::SplitHCursor, new BCursor(B_CURSOR_ID_RESIZE_EAST_WEST));
	m_cursorHash.insert(Qt::PointingHandCursor, new BCursor(B_CURSOR_ID_FOLLOW_LINK));
	m_cursorHash.insert(Qt::ForbiddenCursor, new BCursor(B_CURSOR_ID_NOT_ALLOWED));
	m_cursorHash.insert(Qt::OpenHandCursor, new BCursor(B_CURSOR_ID_GRAB));
	m_cursorHash.insert(Qt::ClosedHandCursor, new BCursor(B_CURSOR_ID_GRABBING));
	m_cursorHash.insert(Qt::WhatsThisCursor, new BCursor(B_CURSOR_ID_HELP));
	m_cursorHash.insert(Qt::BusyCursor, new BCursor(B_CURSOR_ID_PROGRESS));
	m_bitmap = NULL;
}

QHaikuCursor::~QHaikuCursor()
{
	removeCurrentCursorBitmap();
	foreach (BCursor *cursor, m_cursorHash)
		delete cursor;
}

void QHaikuCursor::changeCursor(QCursor *windowCursor, QWindow *window)
{
	if (!windowCursor || !window)
		return;
	QScreen *screen = window->screen();
	if (!screen)
		return;

	if (windowCursor->shape() == Qt::BitmapCursor || windowCursor->shape() == Qt::CustomCursor) {
		QPoint hotspot = windowCursor->hotSpot();
		unsigned char emptyCursor[] = { 16, 1, hotspot.x(), hotspot.y(),
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	    be_app->SetCursor(emptyCursor);

  		QImage image = windowCursor->pixmap().isNull() ? windowCursor->bitmap()->toImage() : windowCursor->pixmap().toImage();
  		if (windowCursor->pixmap().isNull()) {
  			QImage mask = windowCursor->mask()->toImage();
  			QPixmap maskedBitmap = QPixmap::fromImage(image);
  			maskedBitmap.setMask(*windowCursor->mask());
   			image = maskedBitmap.toImage();
  		}
   		image.convertTo(QImage::Format_ARGB32);
		removeCurrentCursorBitmap();
   		m_bitmap = new BBitmap(BRect(0,0, image.width()-1, image.height()-1), B_RGBA32);
   		memcpy(m_bitmap->Bits(), image.bits(), image.sizeInBytes());
	    return;
	}

	BCursor *cursor = m_cursorHash.value(windowCursor->shape(), NULL);
	if (cursor != NULL) {
		be_app->SetCursor(cursor);
		removeCurrentCursorBitmap();
		return;
	}

	be_app->SetCursor((BCursor*)B_CURSOR_SYSTEM_DEFAULT);
	removeCurrentCursorBitmap();
}

BBitmap *QHaikuCursor::getCurrentCursorBitmap(void)
{
	return m_bitmap;
}

void QHaikuCursor::removeCurrentCursorBitmap(void)
{
	if (m_bitmap != NULL) {
   		delete m_bitmap;
   		m_bitmap = NULL;
   	}
}

QT_END_NAMESPACE
