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

#include <stdio.h>

#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformwindow.h>
#include <qdebug.h>

#include "qhaikuwindow.h"
#include "qhaikuview.h"


QHaikuSurfaceView::QHaikuSurfaceView(BRect rect) : 
	BView(rect, "QHaikuSurfaceView", B_FOLLOW_ALL, B_WILL_DRAW),
	viewBitmap(0)
{
	SetDrawingMode(B_OP_COPY);
	lastMouseMoveTime = system_time();
	//SetViewColor(B_TRANSPARENT_32_BIT);
}

QHaikuSurfaceView::~QHaikuSurfaceView()
{	
}

void
QHaikuSurfaceView::Draw(BRect rect)
{
	if(viewBitmap != NULL) {
		SetDrawingMode(B_OP_COPY);
		DrawBitmap(viewBitmap, rect, rect);
	}
}

Qt::MouseButtons 
QHaikuSurfaceView::hostToQtButtons(uint32 buttons)
{
	Qt::MouseButtons qtButtons = Qt::NoButton;
    
    if (buttons & B_PRIMARY_MOUSE_BUTTON)
        qtButtons |= Qt::LeftButton;
    if (buttons & B_TERTIARY_MOUSE_BUTTON)
        qtButtons |= Qt::MidButton;
    if (buttons & B_SECONDARY_MOUSE_BUTTON)
        qtButtons |= Qt::RightButton;
        
    return qtButtons;
}


void 
QHaikuSurfaceView::MouseDown(BPoint point)
{
	BPoint s_point = ConvertToScreen(point);	
	QPoint localPoint(point.x, point.y);
	QPoint globalPoint(s_point.x, s_point.y);
	
	uint32 buttons = Window()->CurrentMessage()->FindInt32("buttons");
	
	lastButtons = hostToQtButtons(buttons);
	
	QHaikuWindow *wnd = ((QtHaikuWindow*)Window())->fQWindow;
	
	SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
	
	if (wnd->window()->flags() & Qt::FramelessWindowHint) {
		Window()->Activate();
	}
	
	QWindowSystemInterface::handleMouseEvent( wnd->window(), localPoint, globalPoint, lastButtons);
}

void 
QHaikuSurfaceView::MouseUp(BPoint point)
{
	BPoint s_point = ConvertToScreen(point);
	QPoint localPoint(point.x, point.y);
	QPoint globalPoint(s_point.x, s_point.y);
	
	QHaikuWindow *wnd = ((QtHaikuWindow*)Window())->fQWindow;

	BPoint pointer;
	uint32 buttons;
	GetMouse(&pointer, &buttons);

	QWindowSystemInterface::handleMouseEvent( wnd->window(), localPoint, globalPoint, hostToQtButtons(buttons));
}

void 
QHaikuSurfaceView::MouseMoved(BPoint point, uint32 transit, const BMessage *msg)
{
	BPoint s_point = ConvertToScreen(point);
	QPoint localPoint(point.x, point.y);
	QPoint globalPoint(s_point.x, s_point.y);
	
	bigtime_t timeNow = system_time();
	
	if (timeNow - lastMouseMoveTime > 10000) {
		BPoint pointer;
		uint32 buttons;
		GetMouse(&pointer, &buttons);

		lastLocalMousePoint = localPoint;
		lastGlobalMousePoint = globalPoint;
	
		QHaikuWindow *wnd = ((QtHaikuWindow*)Window())->fQWindow;

		QWindowSystemInterface::handleMouseEvent( wnd->window(), localPoint, globalPoint, hostToQtButtons(buttons));
		lastMouseMoveTime = timeNow;
	}
}

void
QHaikuSurfaceView::SetViewBitmap(BBitmap *bmp)
{
	viewBitmap = bmp;
}
