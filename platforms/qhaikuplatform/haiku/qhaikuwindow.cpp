/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2015-2020 Gerasim Troeglazov,
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

#include "qhaikuwindow.h"
#include "qhaikucommon.h"
#include "qhaikukeymap.h"
#include "qhaikusettings.h"

#include <private/qguiapplication_p.h>
#include <private/qwindow_p.h>

#include <qpa/qplatformscreen.h>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformtheme.h>

#include <qapplication.h>
#include <qfileinfo.h>
#include <qguiapplication.h>
#include <qstatusbar.h>
#include <qsizegrip.h>

#include <QMimeData>
#include <QDragMoveEvent>

#include <private/qwindow_p.h>

#include <qdebug.h>

QT_BEGIN_NAMESPACE

static uint32 translateKeyCode(uint32 key)
{
	uint32 code = Qt::Key_unknown;
	uint32 i = 0;
	if ( modifiers() & B_NUM_LOCK ) {
	    while (platformHaikuScanCodes_Numlock[i]) {
			if ( key == platformHaikuScanCodes_Numlock[i + 1]) {
				code = platformHaikuScanCodes_Numlock[i];
				break;
			}
			i += 2;
		}
		if (code != Qt::Key_unknown)
			return code;
	}

	i = 0;
    while (platformHaikuScanCodes[i]) {
		if ( key == platformHaikuScanCodes[i + 1]) {
			code = platformHaikuScanCodes[i];
			break;
		}
		i += 2;
	}
	return code;
}


static bool activeWindowChangeQueued(const QWindow *window)
{
    QWindowSystemInterfacePrivate::ActivatedWindowEvent *systemEvent =
        static_cast<QWindowSystemInterfacePrivate::ActivatedWindowEvent *>
        (QWindowSystemInterfacePrivate::peekWindowSystemEvent(QWindowSystemInterfacePrivate::ActivatedWindow));
    return systemEvent && systemEvent->activated != window;
}


static QWindow *childWindowAt(QWindow *win, const QPoint &p)
{
    for (QObject *obj : win->children()) {
        if (obj->isWindowType()) {
            QWindow *childWin = static_cast<QWindow *>(obj);
            if (childWin->isVisible()) {
                if (QWindow *recurse = childWindowAt(childWin, p))
                    return recurse;
            }
        }
    }
    if (!win->isTopLevel()
            && !(win->flags() & Qt::WindowTransparentForInput)
            && win->geometry().contains(win->parent()->mapFromGlobal(p))) {
        return win;
    }
    return nullptr;
}


QtHaikuWindow::QtHaikuWindow(QHaikuWindow *qwindow,
		BRect frame,
		const char *title,
		window_look look,
		window_feel feel,
		uint32 flags)
		: QObject()
		, BWindow(frame, title, look, feel, flags)
{
	fQWindow = qwindow;
	fView = new QHaikuSurfaceView(Bounds());
	fView->SetEventMask(0, B_NO_POINTER_HISTORY);
#if !defined(QT_NO_OPENGL)
	fGLView = NULL;
#endif
 	AddChild(fView);
 	Qt::WindowType type =  static_cast<Qt::WindowType>(int(qwindow->window()->flags() & Qt::WindowType_Mask));
	bool dialog = ((type == Qt::Dialog) || (type == Qt::Sheet) || (type == Qt::MSWindowsFixedSizeDialogHint));
	bool tool = (type == Qt::Tool || type == Qt::Drawer);
	if (!dialog && !tool)
		RemoveShortcut('W', B_COMMAND_KEY);
	AddShortcut('Q', B_COMMAND_KEY, new BMessage(kQuitApplication));
}


QtHaikuWindow::~QtHaikuWindow()
{
}


QHaikuSurfaceView* QtHaikuWindow::View(void)
{
	return fView;
}


void QtHaikuWindow::DispatchMessage(BMessage *msg, BHandler *handler)
{
	switch(msg->what) {
		case B_UNMAPPED_KEY_UP:
		case B_KEY_UP:
		case B_UNMAPPED_KEY_DOWN:
		case B_KEY_DOWN:
			{
				uint32 modifiers = msg->FindInt32("modifiers");
				uint32 key = msg->FindInt32("key");
				QString text;
				const char* bytes;
				if(msg->FindString("bytes", &bytes) == B_OK)
					text = QString::fromUtf8(bytes);
				uint32 qt_keycode = translateKeyCode(key);
				if (qt_keycode == Qt::Key_Print)
					break;
				if (qt_keycode == Qt::Key_Tab && modifiers & B_CONTROL_KEY)
					break;
				if (qt_keycode == Qt::Key_Tab && modifiers & B_SHIFT_KEY)
					qt_keycode = Qt::Key_Backtab;
				if (qt_keycode == Qt::Key_Q && modifiers & B_COMMAND_KEY)
					break;
				if (qt_keycode == Qt::Key_M && modifiers & B_COMMAND_KEY && modifiers & B_CONTROL_KEY)
					break;
				bool press = msg->what == B_KEY_DOWN || msg->what == B_UNMAPPED_KEY_DOWN;
				Q_EMIT keyEvent(press ? QEvent::KeyPress : QEvent::KeyRelease, qt_keycode, fView->hostToQtModifiers(modifiers), text);
				break;
			}
		default:
			break;
	}
	BWindow::DispatchMessage(msg, handler);
}


void QtHaikuWindow::MessageReceived(BMessage* msg)
{
	if (msg->WasDropped()) {
		Q_EMIT dropAction(new BMessage(*msg));
		return;
	}
	switch(msg->what) {
		case kCloseWindow:
		{
			Quit();
			return;
		}
		case kSetTitle:
		{
			const char *title = msg->FindString("title");
			if (title != NULL)
				SetTitle(title);
			return;
		}
		case kQuitApplication:
		{
			be_app->PostMessage(B_QUIT_REQUESTED);
			return;
		}
		case kSizeGripEnable:
		{
			if (Look() == B_TITLED_WINDOW_LOOK)
				SetLook(B_DOCUMENT_WINDOW_LOOK);
			break;
		}
		case kSizeGripDisable:
		{
			if (Look() == B_DOCUMENT_WINDOW_LOOK)
				SetLook(B_TITLED_WINDOW_LOOK);
			break;
		}
		case B_MOUSE_WHEEL_CHANGED:
		{
			 float shift_x=0;
			 float shift_y=0;
			 if (msg->FindFloat("be:wheel_delta_x", &shift_x) != B_OK)
			 	shift_x = 0;
			 if (msg->FindFloat("be:wheel_delta_y", &shift_y) != B_OK)
			 	shift_y = 0;

			 if (shift_y != 0)
				Q_EMIT wheelEvent(fView->lastLocalMousePoint, fView->lastGlobalMousePoint,
					-shift_y * 120, Qt::Vertical, fView->hostToQtModifiers(modifiers()));
			 if (shift_x != 0)
				Q_EMIT wheelEvent(fView->lastLocalMousePoint, fView->lastGlobalMousePoint,
					-shift_x * 120, Qt::Horizontal, fView->hostToQtModifiers(modifiers()));
			 break;
		}
	default:
		BWindow::MessageReceived(msg);
		break;
	}
}


void QtHaikuWindow::Zoom(BPoint origin, float w, float h)
{
	Q_UNUSED(origin);
	Q_UNUSED(w);
	Q_UNUSED(h);
	Q_EMIT windowZoomed();
}

void QtHaikuWindow::Minimize(bool minimized)
{
	BWindow::Minimize(minimized);
	Q_EMIT windowMinimized(minimized);
}

void QtHaikuWindow::FrameResized(float width, float height)
{
	Q_EMIT windowResized(QSize(static_cast<int>(width), static_cast<int>(height)));
}


void QtHaikuWindow::FrameMoved(BPoint point)
{
	Q_EMIT windowMoved(QPoint(point.x, point.y));
}


void QtHaikuWindow::WindowActivated(bool active)
{
	Q_EMIT windowActivated(active);
}


bool QtHaikuWindow::QuitRequested()
{
	Q_EMIT quitRequested();
    return false;
}

QHaikuWindow::QHaikuWindow(QWindow *wnd)
    : QPlatformWindow(wnd)
    , m_window(new QtHaikuWindow(this, BRect(wnd->geometry().left(),
					wnd->geometry().top(),
					wnd->geometry().right(),
					wnd->geometry().bottom()),
					wnd->title().toUtf8().constData(),
					B_NO_BORDER_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 0))
    , m_parent(0)
    , m_systemMoveResizeEnabled(false)
    , m_positionIncludesFrame(false)
    , m_visible(false)
    , m_pendingGeometryChangeOnShow(true)
{
	connect(m_window, SIGNAL(quitRequested()), SLOT(platformWindowQuitRequested()));
    connect(m_window, SIGNAL(windowMoved(QPoint)), SLOT(platformWindowMoved(QPoint)));
	connect(m_window, SIGNAL(windowResized(QSize)), SLOT(platformWindowResized(QSize)));
    connect(m_window, SIGNAL(windowActivated(bool)), SLOT(platformWindowActivated(bool)));
    connect(m_window, SIGNAL(windowZoomed()), SLOT(platformWindowZoomed()));
    connect(m_window, SIGNAL(windowMinimized(bool)), SLOT(platformWindowMinimized(bool)));
    connect(m_window, SIGNAL(dropAction(BMessage*)), SLOT(platformDropAction(BMessage*)));
	connect(m_window, SIGNAL(wheelEvent(QPoint, QPoint, int, Qt::Orientation, Qt::KeyboardModifiers)),
		this, SLOT(platformWheelEvent(QPoint, QPoint, int, Qt::Orientation, Qt::KeyboardModifiers)));
	connect(m_window, SIGNAL(keyEvent(QEvent::Type, int, Qt::KeyboardModifiers, QString)),
		this, SLOT(platformKeyEvent(QEvent::Type, int, Qt::KeyboardModifiers, QString)));

	connect(m_window->View(), SIGNAL(enteredView()), this, SLOT(platformEnteredView()));
	connect(m_window->View(), SIGNAL(exitedView()), this, SLOT(platformExitedView()));
	connect(m_window->View(), SIGNAL(mouseEvent(QPoint, QPoint, Qt::MouseButtons, Qt::MouseButton, QEvent::Type, Qt::KeyboardModifiers, Qt::MouseEventSource)),
		this, SLOT(platformMouseEvent(QPoint, QPoint, Qt::MouseButtons, Qt::MouseButton, QEvent::Type, Qt::KeyboardModifiers, Qt::MouseEventSource)));
	connect(m_window->View(), SIGNAL(mouseDragEvent(QPoint, Qt::DropActions, QMimeData*,  Qt::MouseButtons, Qt::KeyboardModifiers)),
		this, SLOT(platformMouseDragEvent(QPoint, Qt::DropActions, QMimeData*,  Qt::MouseButtons, Qt::KeyboardModifiers)));
	connect(m_window->View(), SIGNAL(tabletEvent(QPointF, QPointF, int, int, Qt::MouseButtons, float, Qt::KeyboardModifiers)),
		this, SLOT(platformTabletEvent(QPointF, QPointF, int, int, Qt::MouseButtons, float, Qt::KeyboardModifiers)));
	connect(m_window->View(), SIGNAL(exposeEvent(QRegion)), this, SLOT(platformExposeEvent(QRegion)));

    setWindowFlags(wnd->flags());
    setWindowState(wnd->windowState());
    handleContentOrientationChange(wnd->contentOrientation());

    setGeometry(wnd->geometry());

    QWindowSystemInterface::flushWindowSystemEvents();

    m_lastMousePos = QPoint(0,0);
}


void QHaikuWindow::destroy()
{
	m_window->PostMessage(kCloseWindow);
	m_window = NULL;
}


void QHaikuWindow::setWindowFlags(Qt::WindowFlags flags)
{
	windowFlags = flags;

	Qt::WindowType type =  static_cast<Qt::WindowType>(int(flags & Qt::WindowType_Mask));

	bool popup = (type == Qt::Popup);
	bool splash = (type == Qt::SplashScreen);
	bool dialog = ((type == Qt::Dialog) || (type == Qt::Sheet) || (type == Qt::MSWindowsFixedSizeDialogHint));
	bool tool = (type == Qt::Tool || type == Qt::Drawer);
	bool tooltip = (type == Qt::ToolTip);

	window_look wlook = B_TITLED_WINDOW_LOOK;
	window_feel wfeel = B_NORMAL_WINDOW_FEEL;
	uint32 wflag = 0;

	if (tool) {
		wlook = B_FLOATING_WINDOW_LOOK;
		wflag |= B_WILL_ACCEPT_FIRST_CLICK;
	}

	if (splash) {
		wlook = B_NO_BORDER_WINDOW_LOOK;
		wflag = B_NO_WORKSPACE_ACTIVATION | B_NOT_ANCHORED_ON_ACTIVATE;
	}

	if (popup) {
		wlook = B_NO_BORDER_WINDOW_LOOK;
		wflag |= B_WILL_ACCEPT_FIRST_CLICK | \
			B_AVOID_FRONT | \
			B_AVOID_FOCUS | \
			B_NO_WORKSPACE_ACTIVATION | \
			B_NOT_ANCHORED_ON_ACTIVATE;
		wfeel = window_feel(1025);
	}

	if (tooltip) {
		wlook = B_NO_BORDER_WINDOW_LOOK;
		wflag |= B_WILL_ACCEPT_FIRST_CLICK | \
			B_AVOID_FRONT | \
			B_AVOID_FOCUS | \
			B_NO_WORKSPACE_ACTIVATION | \
			B_NOT_ANCHORED_ON_ACTIVATE;
		wfeel = window_feel(1025);
	}

    if (flags & Qt::FramelessWindowHint) {
    	wlook = B_NO_BORDER_WINDOW_LOOK;
    	wflag |= B_WILL_ACCEPT_FIRST_CLICK | \
    		B_NO_WORKSPACE_ACTIVATION | \
    		B_NOT_ANCHORED_ON_ACTIVATE;
    }

	if (flags & Qt::MSWindowsFixedSizeDialogHint ||
		windowMinimumSize() == windowMaximumSize() ||
		wlook == B_NO_BORDER_WINDOW_LOOK)
    	wflag |= B_NOT_RESIZABLE | B_NOT_ZOOMABLE;

	if (flags & Qt::CustomizeWindowHint){
		if (!(flags & Qt::WindowMinimizeButtonHint))
    		wflag |= B_NOT_MINIMIZABLE;
		if (!(flags & Qt::WindowMaximizeButtonHint))
			wflag |= B_NOT_ZOOMABLE;
		if (!(flags & Qt::WindowCloseButtonHint))
			wflag |= B_NOT_CLOSABLE;
	}

    if (flags & Qt::WindowStaysOnTopHint) {
        wfeel = B_FLOATING_ALL_WINDOW_FEEL;
    }

	if (flags & Qt::WindowStaysOnTopHint &&
		flags & Qt::FramelessWindowHint &&
		tool) {
		wlook = B_NO_BORDER_WINDOW_LOOK;
		wflag |= B_WILL_ACCEPT_FIRST_CLICK |
			B_AVOID_FRONT |
			B_NO_WORKSPACE_ACTIVATION |
			B_NOT_ANCHORED_ON_ACTIVATE;
		// Telegram notify hack
		if (flags & Qt::NoDropShadowWindowHint)
			wflag |= B_AVOID_FOCUS;
		wfeel = B_FLOATING_ALL_WINDOW_FEEL;
	}
	if (dialog) {
		if (window()->isModal()) {
			wfeel = B_MODAL_APP_WINDOW_FEEL;
		}
		if (QGuiApplication::modalWindow() != NULL &&
			window()->type() == Qt::Dialog) {
			window()->setModality(Qt::ApplicationModal);
			wfeel = B_MODAL_APP_WINDOW_FEEL;
		}
	}
	m_window->SetLook(wlook);
	m_window->SetFeel(wfeel);
	m_window->SetFlags(wflag);
}


void QHaikuWindow::setParent(const QPlatformWindow *window)
{
	if (m_parent != (QHaikuWindow*)window) {
		m_parent = (QHaikuWindow*)window;
	}
}


void QHaikuWindow::setWindowTitle(const QString &title)
{
	QString newTitle = QPlatformWindow::formatWindowTitle(title, QStringLiteral(" - "));
	newTitle = QPlatformTheme::removeMnemonics(newTitle).trimmed();
	BMessage message(kSetTitle);
	message.AddString("title", newTitle.toUtf8().constData());
	m_window->PostMessage(&message);
}


void QHaikuWindow::setGeometry(const QRect &rect)
{
    if (window()->windowState() != Qt::WindowNoState)
        return;

    m_positionIncludesFrame = qt_window_private(window())->positionPolicy == QWindowPrivate::WindowFrameInclusive;

    setFrameMarginsEnabled(true);
    setGeometryImpl(rect);

    m_normalGeometry = geometry();
}

void QHaikuWindow::propagateSizeHints()
{
	QWindow *win = window();

    QSize minimumSize = win->minimumSize();
    QSize maximumSize = win->maximumSize();

    float minW, maxW, minH, maxH;
    m_window->GetSizeLimits(&minW, &maxW, &minH, &maxH);

    if (minimumSize.width() > 0)
		minW = minimumSize.width() - 1;
    if (minimumSize.height() > 0)
		minH = minimumSize.height() - 1;
    if (maximumSize.width() < QWINDOWSIZE_MAX)
		maxW = maximumSize.width() - 1;
    if (maximumSize.height() < QWINDOWSIZE_MAX)
		maxH = maximumSize.height() - 1;

	m_window->SetSizeLimits(minW, maxW, minH, maxH);

	setWindowFlags(window()->flags());
}


void QHaikuWindow::setGeometryImpl(const QRect &rect)
{
    QRect adjusted = rect;

    if ( adjusted.width() <= 0 ) {
        adjusted.setWidth(1);
    }

    if ( adjusted.height() <= 0 ) {
        adjusted.setHeight(1);
    }

	if (window()->parent() == NULL) {
	    if (m_positionIncludesFrame) {
	        adjusted.translate(m_margins.left(), m_margins.top());
	    } else {
	        // make sure we're not placed off-screen
	        if (adjusted.left() < m_margins.left())
	            adjusted.translate(m_margins.left(), 0);
	        if (adjusted.top() < m_margins.top())
	            adjusted.translate(0, m_margins.top());
	    }
	}

    QPlatformWindow::setGeometry(adjusted);
    m_window->MoveTo(adjusted.left(), adjusted.top());
    m_window->ResizeTo(adjusted.width() - 1, adjusted.height() - 1);

    if (m_visible) {
        QWindowSystemInterface::handleGeometryChange(window(), adjusted);
        QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(), adjusted.size()));
    } else {
        m_pendingGeometryChangeOnShow = true;
    }
}

void QHaikuWindow::syncDeskBarVisible(void)
{
	QSettings settings(QT_SETTINGS_FILENAME, QSettings::NativeFormat);
	settings.beginGroup("QPA");
	if (!settings.value("hide_from_deskbar", true).toBool())
		return;

	app_info appInfo;
	if (be_app->GetAppInfo(&appInfo) == B_OK) {
		int visible = 0;
		const QWindowList windows = QGuiApplication::topLevelWindows();
		for (int i = 0; i < windows.size(); ++i) {
			if (windows.at(i)->isVisible() && windows.at(i)->type() != Qt::Popup)
				++visible;
		}
		if (visible == 0) {
			BMessage message;
			message.what = 'BRAQ';
			message.AddInt32("be:team", appInfo.team);
			BMessenger("application/x-vnd.Be-TSKB").SendMessage(&message);
		} else {
			BMessage message;
			message.what = 'BRAS';
			message.AddInt32("be:team", appInfo.team);
			message.AddInt32("be:flags", appInfo.flags);
			message.AddString("be:signature", appInfo.signature);
			message.AddRef("be:ref", &appInfo.ref);
			BMessenger("application/x-vnd.Be-TSKB").SendMessage(&message);
		}
	}
}

QHaikuScreen *QHaikuWindow::platformScreen() const
{
	return static_cast<QHaikuScreen *>(window()->screen()->handle());
}

void QHaikuWindow::setVisible(bool visible)
{
	if (window()->parent())
		return;
	if (visible == m_visible)
		return;

	if (visible) {
		if (window()->type() == Qt::Popup) {
			m_window->SetWorkspaces(B_CURRENT_WORKSPACE);
			m_window->Show();
			m_window->Activate(true);
	    } else {
			m_window->Show();
			if (window()->isModal() && window()->type() == Qt::Dialog)
				m_window->SetFeel(B_MODAL_APP_WINDOW_FEEL);
	    }
    } else {
		setWindowFlags(window()->flags());
		m_window->Hide();
    }

	if (visible) {
		if (window()->type() != Qt::ToolTip)
			QWindowSystemInterface::handleWindowActivated(window());

		if (m_pendingGeometryChangeOnShow) {
			m_pendingGeometryChangeOnShow = false;
			QWindowSystemInterface::handleGeometryChange(window(), geometry());
		}
	}

    if (visible) {
        QRect rect(QPoint(), geometry().size());
        QWindowSystemInterface::handleExposeEvent(window(), rect);
    } else {
        QWindowSystemInterface::handleExposeEvent(window(), QRegion());
    }

	syncDeskBarVisible();

    m_visible = visible;
}


void QHaikuWindow::requestActivateWindow()
{
    if (window()->type() != Qt::Desktop)
		m_window->Activate(true);
    QWindowSystemInterface::handleExposeEvent(window(), window()->geometry());
}


bool QHaikuWindow::startSystemResize(Qt::Edges edges)
{
	if (Q_UNLIKELY(window()->flags().testFlag(Qt::MSWindowsFixedSizeDialogHint)) || edges == 0)
        return false;

	m_systemResizeEdges = edges;
	m_systemMoveWindowGeometry = window()->geometry();
	m_systemMoveResizeEnabled = true;
    return true;
}

bool QHaikuWindow::startSystemMove()
{
	m_systemResizeEdges = 0;
	m_systemMoveWindowGeometry = window()->geometry();
	m_systemMoveResizeEnabled = true;
    return true;
}

void QHaikuWindow::raise()
{
    if (window()->type() != Qt::Desktop) {
        m_window->Activate(true);
        QWindowSystemInterface::handleExposeEvent(window(), window()->geometry());
    }
}

void QHaikuWindow::lower()
{
    if (window()->type() != Qt::Desktop) {
        m_window->Activate(false);
        QWindowSystemInterface::handleExposeEvent(window(), window()->geometry());
    }
}


void QHaikuWindow::setFrameMarginsEnabled(bool enabled)
{
    if (enabled && !(window()->flags() & Qt::FramelessWindowHint)) {
    	BRect frame = m_window->Frame();
    	BRect decoratorFrame = m_window->DecoratorFrame();

		int left = frame.left - decoratorFrame.left;
		int top = frame.top - decoratorFrame.top;
		int right = decoratorFrame.right - frame.right;
		int bottom = decoratorFrame.bottom - frame.bottom;

		m_margins = QMargins(left, top, right, bottom);
    } else
        m_margins = QMargins(0, 0, 0, 0);
}

void QHaikuWindow::getDecoratorSize(float* borderWidth, float* tabHeight)
{
	float borderWidthDef = 5.0;
	float tabHeightDef = 21.0;

	BMessage settings;
	if (m_window->GetDecoratorSettings(&settings) == B_OK) {
		BRect tabRect;
		if (settings.FindRect("tab frame", &tabRect) == B_OK)
			tabHeightDef = tabRect.Height();
		settings.FindFloat("border width", &borderWidthDef);
	} else {
		if (m_window->Look() == B_NO_BORDER_WINDOW_LOOK) {
			borderWidthDef = 0.0;
			tabHeightDef = 0.0;
		}
	}
	if (borderWidth != NULL)
		*borderWidth = borderWidthDef;
	if (tabHeight != NULL)
		*tabHeight = tabHeightDef;
}

void QHaikuWindow::maximizeWindowRespected(bool respected)
{
	BRect screenFrame = (BScreen(B_MAIN_SCREEN_ID)).Frame();
	float maxZoomWidth = screenFrame.Width();
	float maxZoomHeight = screenFrame.Height();

	BRect zoomArea = screenFrame;

	BDeskbar deskbar;
	BRect deskbarFrame = deskbar.Frame();
	if (!deskbar.IsAutoHide() && respected && !(modifiers() & B_SHIFT_KEY)) {
		switch (deskbar.Location()) {
			case B_DESKBAR_TOP:
				zoomArea.top = deskbarFrame.bottom + 2;
				break;
			case B_DESKBAR_BOTTOM:
				zoomArea.bottom = deskbarFrame.top - 2;
				break;
			case B_DESKBAR_LEFT_TOP:
			case B_DESKBAR_LEFT_BOTTOM:
				if (!deskbar.IsAlwaysOnTop() && !deskbar.IsAutoRaise())
					zoomArea.left = deskbarFrame.right + 2;
				break;
			default:
			case B_DESKBAR_RIGHT_TOP:
			case B_DESKBAR_RIGHT_BOTTOM:
				if (!deskbar.IsAlwaysOnTop() && !deskbar.IsAutoRaise())
					zoomArea.right = deskbarFrame.left - 2;
				break;
		}
	}

	float borderWidth;
	float tabHeight;
	getDecoratorSize(&borderWidth, &tabHeight);

	zoomArea.left += borderWidth;
	zoomArea.top += borderWidth + tabHeight;
	zoomArea.right -= borderWidth;
	zoomArea.bottom -= borderWidth;

	if (zoomArea.Height() > maxZoomHeight)
		zoomArea.InsetBy(0, roundf((zoomArea.Height() - maxZoomHeight) / 2));

	if (zoomArea.top > deskbarFrame.bottom
		|| zoomArea.bottom < deskbarFrame.top) {
		zoomArea.left = screenFrame.left + borderWidth;
		zoomArea.right = screenFrame.right - borderWidth;
	}

	if (zoomArea.Width() > maxZoomWidth)
		zoomArea.InsetBy(roundf((zoomArea.Width() - maxZoomWidth) / 2), 0);

	setWindowFlags(window()->flags());
	setGeometryImpl(QRect(zoomArea.left, zoomArea.top, zoomArea.Width() + 1, zoomArea.Height() + 1));
}

void QHaikuWindow::setWindowState(Qt::WindowStates states)
{
    Qt::WindowState state = Qt::WindowNoState;
    if (states & Qt::WindowMinimized)
        state = Qt::WindowMinimized;
    else if (states & Qt::WindowFullScreen)
        state = Qt::WindowFullScreen;
    else if (states & Qt::WindowMaximized)
        state = Qt::WindowMaximized;

    setFrameMarginsEnabled(state != Qt::WindowFullScreen);
    m_positionIncludesFrame = false;

	if (!(states & Qt::WindowMinimized) && m_window->IsMinimized())
		m_window->Minimize(false);

    switch (state) {
    case Qt::WindowFullScreen:
    	m_window->SetLook(B_NO_BORDER_WINDOW_LOOK);
        setGeometryImpl(screen()->geometry());
        break;
    case Qt::WindowMaximized:
    	m_normalGeometry = geometry();
		setWindowFlags(window()->flags());
		maximizeWindowRespected(true);
		break;
    case Qt::WindowMinimized:
		m_normalGeometry = geometry();
		if (!m_window->IsMinimized())
			m_window->Minimize(true);
		setWindowFlags(window()->flags());
		break;
    case Qt::WindowNoState:
		setWindowFlags(window()->flags());
        setGeometryImpl(m_normalGeometry);
        break;
    default:
        break;
    }
    QWindowSystemInterface::handleWindowStateChanged(window(), states);
}

WId QHaikuWindow::winId() const
{
	return reinterpret_cast<WId>(static_cast<BWindow *>(m_window));
}

QHaikuSurfaceView *QHaikuWindow::viewForWinId(WId id)
{
	QHaikuWindow *window = windowForWinId(id);
	if (window != NULL)
		return window->m_window->View();
    return NULL;
}

QHaikuWindow *QHaikuWindow::windowForWinId(WId id)
{
	QtHaikuWindow * window = static_cast<QtHaikuWindow *>(reinterpret_cast<BWindow *>(id));
	if (window != NULL)
		return window->fQWindow;
	return NULL;
}


void QHaikuWindow::platformWindowQuitRequested()
{
    QWindowSystemInterface::handleCloseEvent(window());
}


void QHaikuWindow::platformWindowMoved(const QPoint &pos)
{
	QRect adjusted = geometry();
	adjusted.moveTopLeft(pos);

    QPlatformWindow::setGeometry(adjusted);

	if (window()->isTopLevel() && window()->type() == Qt::Window) {
		while (QApplication::activePopupWidget())
			QApplication::activePopupWidget()->close();
    }

    if (m_visible) {
        QWindowSystemInterface::handleGeometryChange(window(), adjusted);
        QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(), adjusted.size()));
    } else {
        m_pendingGeometryChangeOnShow = true;
    }
}

void QHaikuWindow::platformWindowResized(const QSize &size)
{
	QRect adjusted = geometry();
	adjusted.setWidth(size.width() + 1);
	adjusted.setHeight(size.height() + 1);

    QPlatformWindow::setGeometry(adjusted);

    if (m_visible)
        QWindowSystemInterface::handleGeometryChange(window(), adjusted);
    else
        m_pendingGeometryChangeOnShow = true;
}

void QHaikuWindow::platformWindowActivated(bool activated)
{
	if (!activated) {
	    if (window()->isTopLevel() && window()->type() == Qt::Window) {
			while (QApplication::activePopupWidget())
				QApplication::activePopupWidget()->close();
	    }
		if (window() == QGuiApplication::focusWindow()
            && !activeWindowChangeQueued(window())) {
            QWindowSystemInterface::handleWindowActivated(0);
		}
		QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationInactive);
		return;
	}
	QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationActive);
	QWindowSystemInterface::handleWindowActivated(window(), Qt::ActiveWindowFocusReason);
}


void QHaikuWindow::platformWindowZoomed()
{
	if (window()->windowState() & Qt::WindowMaximized) {
		setWindowState(Qt::WindowNoState);
	} else {
		setWindowState(Qt::WindowMaximized);
	}
}

void QHaikuWindow::platformWindowMinimized(bool minimized)
{
	if (minimized) {
		m_lastWindowStates = window()->windowStates();
		m_lastWindowStates &= ~Qt::WindowMinimized;
		QWindowSystemInterface::handleWindowStateChanged(window(), Qt::WindowMinimized);
	} else {
		QWindowSystemInterface::handleWindowStateChanged(window(), m_lastWindowStates);
	}
}

void QHaikuWindow::platformDropAction(BMessage *msg)
{
	if (window()->parent())
		return;
	BPoint dropOffset;
	BPoint dropPoint = msg->DropPoint(&dropOffset);
	m_window->ConvertFromScreen(&dropPoint);
	m_window->ConvertFromScreen(&dropOffset);

	QPoint m_lastPoint = QPoint(dropPoint.x, dropPoint.y);

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
		} else
			return;
	}

	BPoint pointer;
	uint32 buttons;
	if (m_window->View()->LockLooper()) {
		m_window->View()->GetMouse(&pointer, &buttons);
		m_window->View()->UnlockLooper();
	}

	QDragMoveEvent dmEvent(m_lastPoint,
                    Qt::CopyAction | Qt::MoveAction | Qt::LinkAction,
                    dragData,
                    m_window->View()->hostToQtButtons(buttons),
                    m_window->View()->hostToQtModifiers(modifiers()));

    dmEvent.setDropAction(Qt::CopyAction);
    dmEvent.accept();

    QGuiApplication::sendEvent(window(), &dmEvent);

	const QPlatformDropQtResponse response =
		QWindowSystemInterface::handleDrop(window(), dragData, m_lastPoint,
			Qt::CopyAction | Qt::MoveAction | Qt::LinkAction,
			m_window->View()->hostToQtButtons(buttons),
			m_window->View()->hostToQtModifiers(modifiers()));

	if (response.isAccepted()) {
		response.acceptedAction();
	}
}

void QHaikuWindow::platformEnteredView()
{
	QPoint pos = window()->cursor().pos();
    QWindowSystemInterface::handleEnterEvent(window(), window()->mapFromGlobal(pos), pos);
}

void QHaikuWindow::platformExitedView()
{
    QWindowSystemInterface::handleLeaveEvent(window());
    QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0,0), window()->size()));
}

void QHaikuWindow::platformMouseEvent(const QPoint &localPosition,
	const QPoint &globalPosition,
	Qt::MouseButtons state,
	Qt::MouseButton button,
	QEvent::Type type,
	Qt::KeyboardModifiers modifiers,
	Qt::MouseEventSource source)
{
	QWindow *childWindow = childWindowAt(window(), globalPosition);
	if (childWindow) {
		QWindowSystemInterface::handleMouseEvent(childWindow,
			childWindow->mapFromGlobal(globalPosition),
			globalPosition, state, button, type, modifiers, source);
			QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0,0), window()->size()));
	} else {
		QWindowSystemInterface::handleMouseEvent(window(),
			localPosition, globalPosition, state, button, type, modifiers, source);
		if (type == QEvent::MouseButtonRelease && m_systemMoveResizeEnabled)
			m_systemMoveResizeEnabled = false;

		if (type == QEvent::MouseMove && m_systemMoveResizeEnabled) {
			if (m_systemResizeEdges == 0) {
				window()->setFramePosition(m_systemMoveWindowGeometry.topLeft() + (globalPosition - m_lastMousePos));
				QWindowSystemInterface::handleGeometryChange(window(), m_systemMoveWindowGeometry);
				return;
			}
			QRect newGeometry = m_systemMoveWindowGeometry;
			if (m_systemResizeEdges & Qt::RightEdge)
				newGeometry.setRight(MAX(globalPosition.x(), m_systemMoveWindowGeometry.x() + window()->minimumSize().width()));
			if (m_systemResizeEdges & Qt::LeftEdge)
				newGeometry.setLeft(MIN(globalPosition.x(), m_systemMoveWindowGeometry.x() + window()->minimumSize().width()));
			if (m_systemResizeEdges & Qt::TopEdge)
				newGeometry.setTop(MIN(globalPosition.y(), m_systemMoveWindowGeometry.y() + window()->minimumSize().height()));
			if (m_systemResizeEdges & Qt::BottomEdge)
				newGeometry.setBottom(MAX(globalPosition.y(), m_systemMoveWindowGeometry.y() + window()->minimumSize().height()));
			window()->setGeometry(newGeometry.left(), newGeometry.top(), newGeometry.width(), newGeometry.height());
			QWindowSystemInterface::handleGeometryChange(window(), newGeometry);
			return;
		}
	}
	m_lastMousePos = globalPosition;
}

void QHaikuWindow::platformMouseDragEvent(const QPoint &localPosition,
	Qt::DropActions actions,
	QMimeData *data,
	Qt::MouseButtons buttons,
	Qt::KeyboardModifiers modifiers)
{
	QDragMoveEvent dmEvent(localPosition, actions, data, buttons, modifiers);
	dmEvent.setDropAction(Qt::CopyAction);
	dmEvent.accept();
	QGuiApplication::sendEvent(window(), &dmEvent);
}

void QHaikuWindow::platformWheelEvent(const QPoint &localPosition,
	const QPoint &globalPosition,
	int delta,
	Qt::Orientation orientation,
	Qt::KeyboardModifiers modifiers)
{
	const QPoint point = (orientation == Qt::Vertical) ? QPoint(0, delta) : QPoint(delta, 0);

	QWindow *childWindow = childWindowAt(window(), globalPosition);
	if (childWindow) {
		QWindowSystemInterface::handleWheelEvent(childWindow, childWindow->mapFromGlobal(globalPosition), globalPosition, QPoint(), point, modifiers);
		QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0,0), window()->size()));
	} else
        QWindowSystemInterface::handleWheelEvent(window(), localPosition, globalPosition, QPoint(), point, modifiers);
}

void QHaikuWindow::platformTabletEvent(const QPointF &localPosition,
	const QPointF &globalPosition,
	int device,
	int pointerType,
	Qt::MouseButtons buttons,
	float pressure,
	Qt::KeyboardModifiers modifiers)
{
	QWindow *childWindow = childWindowAt(window(), globalPosition.toPoint());
	if (childWindow) {
		QWindowSystemInterface::handleTabletEvent(childWindow, childWindow->mapFromGlobal(globalPosition.toPoint()),
			globalPosition,	device, pointerType, buttons, pressure, 0, 0, 0.0, 0.0, 0, 0, modifiers);
		QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0,0), window()->size()));
	} else {
		QWindowSystemInterface::handleTabletEvent(window(), localPosition, globalPosition,
			device, pointerType, buttons, pressure, 0, 0, 0.0, 0.0, 0, 0, modifiers);
	}
	m_lastMousePos = globalPosition.toPoint();
}


void QHaikuWindow::platformKeyEvent(QEvent::Type type, int key, Qt::KeyboardModifiers modifiers, const QString &text)
{
    QWindowSystemInterface::handleKeyEvent(window(), type, key, modifiers, text);
}

void QHaikuWindow::platformExposeEvent(QRegion region)
{
	QWindowSystemInterface::handleExposeEvent<QWindowSystemInterface::SynchronousDelivery>(window(), region);
}

QT_END_NAMESPACE
