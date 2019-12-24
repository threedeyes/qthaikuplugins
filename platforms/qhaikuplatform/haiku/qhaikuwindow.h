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

#ifndef QHAIKUWINDOW_H
#define QHAIKUWINDOW_H

#include <qpa/qplatformbackingstore.h>
#include <qpa/qplatformwindow.h>

#include <Window.h>
#include <Rect.h>
#include <View.h>
#include <Entry.h>
#include <Path.h>
#include <Screen.h>
#include <String.h>
#include <Deskbar.h>
#include <Roster.h>

#if !defined(QT_NO_OPENGL)
#include <GLView.h>
#endif

#include <qhash.h>

#include "qhaikuview.h"

#define kQuitApplication	'QAPP'
#define kSizeGripEnable		'SGEN'
#define kSizeGripDisable	'SGDI'

QT_BEGIN_NAMESPACE

class QHaikuWindow;

class QtHaikuWindow : public QObject, public BWindow
{
	Q_OBJECT
public:
	QtHaikuWindow(QHaikuWindow *qwindow, BRect frame,
		const char *title, window_look look, window_feel feel, uint32 flags);
	~QtHaikuWindow();

	void FrameResized(float width, float height);
	void FrameMoved(BPoint point);
	void MessageReceived(BMessage* msg);
	virtual void DispatchMessage(BMessage *, BHandler *);	
	virtual void WindowActivated(bool active);
	virtual bool QuitRequested();

	virtual void Zoom(BPoint origin, float w, float h);

	QHaikuSurfaceView *View(void);

	QHaikuSurfaceView *fView;
#if !defined(QT_NO_OPENGL)
	BGLView *fGLView;
#endif
	QHaikuWindow *fQWindow;
Q_SIGNALS:
    void windowMoved(const QPoint &pos);
    void windowResized(const QSize &size);
    void windowActivated(bool activated);
    void windowZoomed();
    void quitRequested();
    void dropAction(BMessage *message);
	void wheelEvent(const QPoint &localPosition,
		const QPoint &globalPosition,
		int delta,
		Qt::Orientation orientation,
		Qt::KeyboardModifiers modifiers);
	void keyEvent(QEvent::Type type,
		int key,
		Qt::KeyboardModifiers modifiers,
		const QString &text);
};

class QHaikuWindow : public QObject, public QPlatformWindow
{
	Q_OBJECT
public:
	QHaikuWindow(QWindow *window);
	~QHaikuWindow();

	void setGeometry(const QRect &rect) override;
	void setWindowTitle(const QString &title) override;
	void setWindowState(Qt::WindowStates state) override;
	void setWindowFlags(Qt::WindowFlags flags) override;
	void setParent(const QPlatformWindow *window) override;

	bool windowEvent(QEvent *event) override;

	QMargins frameMargins() const;

	void setVisible(bool visible);
	void requestActivateWindow();
    
	bool setKeyboardGrabEnabled(bool) Q_DECL_OVERRIDE { return false; }
	bool setMouseGrabEnabled(bool) Q_DECL_OVERRIDE { return false; }
	void propagateSizeHints();

	WId winId() const override { return m_winId; }

	static QHaikuWindow *windowForWinId(WId id);
	static QHaikuSurfaceView *viewForWinId(WId id);

	void raise();
	void lower();

	QtHaikuWindow *m_window;
	QHaikuWindow *m_parent;
private:
	void setFrameMarginsEnabled(bool enabled);
	void setGeometryImpl(const QRect &rect);
	void getDecoratorSize(float* borderWidth, float* tabHeight);
	void maximizeWindowRespected(bool respected);
	void syncDeskBarVisible(void);

	QRect m_normalGeometry;
	QMargins m_margins;

	WId m_winId;
	static QHash<WId, QHaikuWindow *> m_windowForWinIdHash;

	Qt::WindowFlags windowFlags;
	bool m_positionIncludesFrame;
	bool m_visible;
	bool m_pendingGeometryChangeOnShow;
private Q_SLOTS:
	void platformWindowQuitRequested();
	void platformWindowMoved(const QPoint &pos);
	void platformWindowResized(const QSize &size);
	void platformWindowActivated(bool activated);
	void platformWindowZoomed();
	void platformDropAction(BMessage *message);
	void platformEnteredView();
	void platformExitedView();
	void platformMouseEvent(const QPoint &localPosition,
		const QPoint &globalPosition,
		Qt::MouseButtons state,
		Qt::MouseButton button,
		QEvent::Type type,
		Qt::KeyboardModifiers modifiers,
		Qt::MouseEventSource source);
	void platformMouseDragEvent(const QPoint &localPosition,
		Qt::DropActions actions,
		QMimeData *data,
		Qt::MouseButtons buttons,
		Qt::KeyboardModifiers modifiers);
	void platformWheelEvent(const QPoint &localPosition,
		const QPoint &globalPosition,
		int delta,
		Qt::Orientation orientation,
		Qt::KeyboardModifiers modifiers);
	void platformKeyEvent(QEvent::Type type,
		int key,
		Qt::KeyboardModifiers modifiers,
		const QString &text);
	void platformExposeEvent(QRegion region);
};

QT_END_NAMESPACE

#endif
