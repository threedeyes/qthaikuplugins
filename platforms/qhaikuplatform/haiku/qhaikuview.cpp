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

#include "qhaikuwindow.h"
#include "qhaikuview.h"

QT_BEGIN_NAMESPACE

Q_DECLARE_METATYPE(QEvent::Type)
Q_DECLARE_METATYPE(Qt::MouseButtons)
Q_DECLARE_METATYPE(Qt::MouseEventSource)
Q_DECLARE_METATYPE(Qt::KeyboardModifiers)
Q_DECLARE_METATYPE(Qt::Orientation)

QHaikuSurfaceView::QHaikuSurfaceView(BRect rect)
	: QObject()
	, BView(rect, "QHaikuSurfaceView", B_FOLLOW_ALL, B_WILL_DRAW),
	viewBitmap(0)
{
    qRegisterMetaType<QEvent::Type>();
    qRegisterMetaType<Qt::MouseButtons>();
    qRegisterMetaType<Qt::MouseEventSource>();
    qRegisterMetaType<Qt::KeyboardModifiers>();
    qRegisterMetaType<Qt::Orientation>();

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
QHaikuSurfaceView::hostToQtButtons(uint32 buttons) const
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

Qt::KeyboardModifiers
QHaikuSurfaceView::hostToQtModifiers(uint32 keyState) const
{
    Qt::KeyboardModifiers modifiers(Qt::NoModifier);

    if (keyState & B_SHIFT_KEY)
        modifiers |= Qt::ShiftModifier;
    if (keyState & B_CONTROL_KEY)
        modifiers |= Qt::AltModifier;
    if (keyState & B_COMMAND_KEY)
        modifiers |= Qt::ControlModifier;

    return modifiers;
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

	Q_EMIT mouseEvent(localPoint, globalPoint, hostToQtButtons(buttons),
		hostToQtModifiers(modifiers()), Qt::MouseEventNotSynthesized);

	BView::MouseDown(point);
}

void 
QHaikuSurfaceView::MouseUp(BPoint point)
{
	BPoint s_point = ConvertToScreen(point);
	QPoint localPoint(point.x, point.y);
	QPoint globalPoint(s_point.x, s_point.y);

	BPoint pointer;
	uint32 buttons;
	GetMouse(&pointer, &buttons);

	Q_EMIT mouseEvent(localPoint, globalPoint, hostToQtButtons(buttons),
		hostToQtModifiers(modifiers()), Qt::MouseEventNotSynthesized);

	BView::MouseUp(point);
}

void 
QHaikuSurfaceView::MouseMoved(BPoint point, uint32 transit, const BMessage *msg)
{
	if (msg != NULL)
		return;

	switch (transit) {
		case B_INSIDE_VIEW:
			break;
		case B_ENTERED_VIEW:
			Q_EMIT enteredView();
			break;
		case B_EXITED_VIEW:
			Q_EMIT exitedView();
			break;
    }

	BPoint s_point = ConvertToScreen(point);
	QPoint localPoint(point.x, point.y);
	QPoint globalPoint(s_point.x, s_point.y);

	bigtime_t timeNow = system_time();

	if (timeNow - lastMouseMoveTime > 1000) {
		BPoint pointer;
		uint32 buttons;
		GetMouse(&pointer, &buttons);

		lastLocalMousePoint = localPoint;
		lastGlobalMousePoint = globalPoint;

		Q_EMIT mouseEvent(localPoint, globalPoint, hostToQtButtons(buttons),
			hostToQtModifiers(modifiers()), Qt::MouseEventNotSynthesized);

		lastMouseMoveTime = timeNow;
	}
	BView::MouseMoved(point, transit, msg);
}

void
QHaikuSurfaceView::SetViewBitmap(BBitmap *bmp)
{
	viewBitmap = bmp;
}
