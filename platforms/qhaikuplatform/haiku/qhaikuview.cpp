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

#include <QGuiApplication>
#include <QMimeData>
#include <QDragMoveEvent>

#include "qhaikuwindow.h"
#include "qhaikuview.h"

QT_BEGIN_NAMESPACE

Q_DECLARE_METATYPE(QEvent::Type)
Q_DECLARE_METATYPE(Qt::DropActions)
Q_DECLARE_METATYPE(Qt::MouseButton)
Q_DECLARE_METATYPE(Qt::MouseButtons)
Q_DECLARE_METATYPE(Qt::MouseEventSource)
Q_DECLARE_METATYPE(Qt::KeyboardModifiers)
Q_DECLARE_METATYPE(Qt::Orientation)

QHaikuSurfaceView::QHaikuSurfaceView(BRect rect)
	: QObject()
	, BView(rect, "QHaikuSurfaceView", B_FOLLOW_ALL, B_WILL_DRAW),
	lastMouseState(Qt::NoButton),
	lastMouseButton(Qt::NoButton)
{
    qRegisterMetaType<QEvent::Type>();
	qRegisterMetaType<Qt::DropActions>();
    qRegisterMetaType<Qt::MouseButton>();
    qRegisterMetaType<Qt::MouseButtons>();
    qRegisterMetaType<Qt::MouseEventSource>();
    qRegisterMetaType<Qt::KeyboardModifiers>();
    qRegisterMetaType<Qt::Orientation>();

	SetDrawingMode(B_OP_COPY);
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	lastMouseMoveTime = system_time();
}

void
QHaikuSurfaceView::Draw(BRect rect)
{
	QRegion region(QRect(rect.left, rect.top, rect.IntegerWidth() + 1, rect.IntegerHeight() + 1));
	Q_EMIT exposeEvent(region);
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

Qt::MouseButton
QHaikuSurfaceView::hostToQtButton(uint32 buttons) const
{
    if (buttons & B_PRIMARY_MOUSE_BUTTON)
        return Qt::LeftButton;
    if (buttons & B_TERTIARY_MOUSE_BUTTON)
        return Qt::MidButton;
    if (buttons & B_SECONDARY_MOUSE_BUTTON)
        return Qt::RightButton;
    return Qt::NoButton;
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

bool
QHaikuSurfaceView::isSizeGripperContains(BPoint point)
{
	if (Window()->Look() != B_DOCUMENT_WINDOW_LOOK)
		return false;

	BRect gripRect = Window()->Bounds();
	gripRect.left = gripRect.right - 15;
	gripRect.top = gripRect.bottom - 15;
	if (gripRect.Contains(point))
		return true;

	return false;
}

void 
QHaikuSurfaceView::MouseDown(BPoint point)
{
	BPoint s_point = ConvertToScreen(point);	
	QPoint localPoint(point.x, point.y);
	QPoint globalPoint(s_point.x, s_point.y);

	if (isSizeGripperContains(point))
		return;

	uint32 buttons = Window()->CurrentMessage()->FindInt32("buttons");

	QHaikuWindow *wnd = ((QtHaikuWindow*)Window())->fQWindow;

	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);

	if (wnd->window()->flags() & Qt::FramelessWindowHint) {
		Window()->Activate();
	}

	lastMouseState = hostToQtButtons(buttons);
	lastMouseButton = hostToQtButton(buttons);

	Q_EMIT mouseEvent(localPoint, globalPoint, lastMouseState, lastMouseButton, QEvent::MouseButtonPress,
		hostToQtModifiers(modifiers()), Qt::MouseEventNotSynthesized);
}

void 
QHaikuSurfaceView::MouseUp(BPoint point)
{
	BPoint s_point = ConvertToScreen(point);
	QPoint localPoint(point.x, point.y);
	QPoint globalPoint(s_point.x, s_point.y);

	if (isSizeGripperContains(point))
		return;

	BPoint pointer;
	uint32 buttons;
	GetMouse(&pointer, &buttons);

	Qt::MouseButtons state = hostToQtButton(buttons);

	Q_EMIT mouseEvent(localPoint, globalPoint, state, lastMouseButton, QEvent::MouseButtonRelease,
		hostToQtModifiers(modifiers()), Qt::MouseEventNotSynthesized);
}

void 
QHaikuSurfaceView::MouseMoved(BPoint point, uint32 transit, const BMessage *msg)
{
	bool isTabletEvent = Window()->CurrentMessage()->HasFloat("be:tablet_x");

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
	// 50 events per sec. limit
	if (timeNow - lastMouseMoveTime > Q_HAIKU_MOUSE_EVENTS_TIME) {
		BPoint pointer;
		uint32 buttons;
		GetMouse(&pointer, &buttons);

		if (isSizeGripperContains(point))
			return;

		if ( modifiers() & B_CONTROL_KEY
			&& modifiers() & B_COMMAND_KEY
			&& buttons & B_SECONDARY_MOUSE_BUTTON)
			return;

		lastLocalMousePoint = localPoint;
		lastGlobalMousePoint = globalPoint;

		if (msg != NULL) {
			QMimeData *dragData = new QMimeData();
			QList<QUrl> urls;
			entry_ref aRef;
			for (int i = 0; msg->FindRef("refs", i, &aRef) == B_OK; i++) {
				BEntry entry(&aRef);
				BPath path;
				entry.GetPath(&path);
				QUrl url = QUrl::fromLocalFile(path.Path());
				urls.append(url);
			}
			if (urls.count() > 0)
				dragData->setUrls(urls);
			ssize_t dataLength = 0;
			const char* text = NULL;
			if (msg->FindData("text/plain", B_MIME_TYPE, (const void**)&text, &dataLength) == B_OK) {
				if (dataLength > 0) {
					dragData->setText(QString::fromUtf8(text, dataLength));
				}
			}
			Q_EMIT mouseDragEvent(localPoint, Qt::CopyAction | Qt::MoveAction | Qt::LinkAction, dragData,
				hostToQtButtons(buttons), hostToQtModifiers(modifiers()));
		} else {
			if (isTabletEvent) {
				BScreen scr;
				float x = Window()->CurrentMessage()->FindFloat("be:tablet_x");
				float y = Window()->CurrentMessage()->FindFloat("be:tablet_y");
				float pressure = Window()->CurrentMessage()->FindFloat("be:tablet_pressure");
				int32 eraser = Window()->CurrentMessage()->FindFloat("be:tablet_eraser");
				QPointF globalTablePoint(x * scr.Frame().Width(), y * scr.Frame().Height());
				Q_EMIT tabletEvent(QPointF(localPoint), QPointF(globalPoint), QTabletEvent::Stylus,
					eraser==0 ? QTabletEvent::Pen :  QTabletEvent::Eraser, hostToQtButtons(buttons),
					pressure, hostToQtModifiers(modifiers()));
				Q_EMIT mouseEvent(localPoint, globalPoint, Qt::NoButton, Qt::NoButton, QEvent::MouseMove,
					hostToQtModifiers(modifiers()), Qt::MouseEventNotSynthesized);
			} else {
				Q_EMIT mouseEvent(localPoint, globalPoint, hostToQtButtons(buttons), Qt::NoButton, QEvent::MouseMove,
					hostToQtModifiers(modifiers()), Qt::MouseEventNotSynthesized);
			}
		}

		lastMouseMoveTime = timeNow;
	}
}
