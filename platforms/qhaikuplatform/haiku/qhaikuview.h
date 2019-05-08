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

#ifndef _H_HAIKUSURFACEVIEW_
#define _H_HAIKUSURFACEVIEW_

#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformwindow.h>
#include <qpa/qplatformdrag.h>
#include <qdebug.h>

#include <SupportDefs.h>
#include <Bitmap.h>
#include <View.h>
#include <Point.h>
#include <Rect.h>

#define Q_HAIKU_MOUSE_EVENTS_TIME 10000
#define Q_HAIKU_MOUSE_PREVENT_TIME 300000

class QHaikuSurfaceView : public QObject, public BView
{
		Q_OBJECT
 public:
		QHaikuSurfaceView(BRect rect);
		~QHaikuSurfaceView();
		
		virtual void Draw(BRect rect);
		virtual void MouseDown(BPoint p);
		virtual void MouseUp(BPoint p);
		virtual void MouseMoved(BPoint point, uint32 transit, const BMessage *msg);
		void SetViewBitmap(BBitmap *bmp);

		void PreventMouse(void);

		Qt::MouseButton hostToQtButton(uint32 buttons) const;
		Qt::MouseButtons hostToQtButtons(uint32 buttons) const;
		Qt::KeyboardModifiers hostToQtModifiers(uint32 keyState) const;
		
		QPoint	lastLocalMousePoint;
		QPoint 	lastGlobalMousePoint;
		
 private:
		bool isSizeGripperContains(BPoint);
		Qt::MouseButtons lastMouseState;
		Qt::MouseButton lastMouseButton;
		bigtime_t lastMouseMoveTime;
		bigtime_t mousePreventTime;
		BBitmap *viewBitmap;
 Q_SIGNALS:
		void mouseEvent(const QPoint &localPosition,
			const QPoint &globalPosition,
			Qt::MouseButtons state,
			Qt::MouseButton button,
			QEvent::Type type,
			Qt::KeyboardModifiers modifiers,
			Qt::MouseEventSource source);
		void mouseDragEvent(const QPoint &localPosition,
			Qt::DropActions actions,
			QMimeData *data,
			Qt::MouseButtons buttons,
			Qt::KeyboardModifiers modifiers);
	    void enteredView();
		void exitedView();
};

#endif
