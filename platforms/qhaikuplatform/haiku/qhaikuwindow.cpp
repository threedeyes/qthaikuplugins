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

#include "qhaikuwindow.h"
#include "qhaikucommon.h"
#include "qhaikukeymap.h"

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
	uint32 code = 0;
	uint32 i = 0;
	if (modifiers()&&B_NUM_LOCK) {
	    while (platformHaikuScanCodes_Numlock[i]) {
			if ( key == platformHaikuScanCodes_Numlock[i + 1]) {
				code = platformHaikuScanCodes_Numlock[i];
				break;
			}
			i += 2;
		}
		if(code>0)
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
#if !defined(QT_NO_OPENGL)
	fGLView = NULL;
#endif	
 	AddChild(fView);
	RemoveShortcut('W', B_COMMAND_KEY);
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
				uint32 modifier = msg->FindInt32("modifiers");
				uint32 key = msg->FindInt32("key");
				QString text;
				const char* bytes;
				if(msg->FindString("bytes", &bytes) == B_OK)
					text = QString::fromUtf8(bytes);
				uint32 qt_keycode = translateKeyCode(key);
				if ((qt_keycode == Qt::Key_Tab) && (modifier & B_CONTROL_KEY))
					break;
				bool press = (msg->what == B_KEY_DOWN) || (msg->what == B_UNMAPPED_KEY_DOWN);
				Q_EMIT keyEvent(press ? QEvent::KeyPress : QEvent::KeyRelease, qt_keycode, fView->hostToQtModifiers(modifiers()), text);
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
	fView->PreventMouse();
	Q_EMIT windowZoomed();
}


void QtHaikuWindow::FrameResized(float width, float height)
{
	fView->PreventMouse();
	Q_EMIT windowResized(QSize(static_cast<int>(width), static_cast<int>(height)));
}


void QtHaikuWindow::FrameMoved(BPoint point)
{
	fView->PreventMouse();
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
    , m_positionIncludesFrame(false)
    , m_visible(false)
    , m_pendingGeometryChangeOnShow(true)
    , m_window(new QtHaikuWindow(this, BRect(wnd->geometry().left(),
    				wnd->geometry().top(),
    				wnd->geometry().right(),
    				wnd->geometry().bottom()),
    				wnd->title().toUtf8(),
    				B_NO_BORDER_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 0))
{
	connect(m_window, SIGNAL(quitRequested()), SLOT(platformWindowQuitRequested()), Qt::BlockingQueuedConnection);
    connect(m_window, SIGNAL(windowMoved(QPoint)), SLOT(platformWindowMoved(QPoint)));
	connect(m_window, SIGNAL(windowResized(QSize)), SLOT(platformWindowResized(QSize)));
    connect(m_window, SIGNAL(windowActivated(bool)), SLOT(platformWindowActivated(bool)));
    connect(m_window, SIGNAL(windowZoomed()), SLOT(platformWindowZoomed()));
    connect(m_window, SIGNAL(dropAction(BMessage*)), SLOT(platformDropAction(BMessage*)));
	connect(m_window, SIGNAL(wheelEvent(QPoint, QPoint, int, Qt::Orientation, Qt::KeyboardModifiers)),
		this, SLOT(platformWheelEvent(QPoint, QPoint, int, Qt::Orientation, Qt::KeyboardModifiers)));
	connect(m_window, SIGNAL(keyEvent(QEvent::Type, int, Qt::KeyboardModifiers, QString)),
		this, SLOT(platformKeyEvent(QEvent::Type, int, Qt::KeyboardModifiers, QString)));

	connect(m_window->View(), SIGNAL(enteredView()), this, SLOT(platformEnteredView()));
	connect(m_window->View(), SIGNAL(exitedView()), this, SLOT(platformExitedView()));
	connect(m_window->View(), SIGNAL(mouseEvent(QPoint, QPoint, Qt::MouseButtons, Qt::KeyboardModifiers, Qt::MouseEventSource)),
		this, SLOT(platformMouseEvent(QPoint, QPoint, Qt::MouseButtons, Qt::KeyboardModifiers, Qt::MouseEventSource)));

    window()->setProperty("size-grip", false);

    setWindowFlags(wnd->flags());
    setWindowState(wnd->windowState());
    handleContentOrientationChange(wnd->contentOrientation());

    setGeometry(wnd->geometry());

    QWindowSystemInterface::flushWindowSystemEvents();

	static WId counter = 0;
    m_winId = ++counter;

    m_windowForWinIdHash[m_winId] = this;
}


QHaikuWindow::~QHaikuWindow()
{
	m_window->Lock();
	m_window->Quit();
	m_windowForWinIdHash.remove(m_winId);
}


void QHaikuWindow::setWindowFlags(Qt::WindowFlags flags)
{
	windowFlags = flags;

	Qt::WindowType type =  static_cast<Qt::WindowType>(int(flags & Qt::WindowType_Mask)) ;

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

	if (flags & Qt::MSWindowsFixedSizeDialogHint)
    	wflag |= B_NOT_RESIZABLE | B_NOT_ZOOMABLE;

	if (flags & Qt::CustomizeWindowHint){
		if (!(flags & Qt::WindowMinimizeButtonHint))
    		wflag |= B_NOT_MINIMIZABLE;
		if (!(flags & Qt::WindowMaximizeButtonHint))
			wflag |= B_NOT_ZOOMABLE;
		if (!(flags & Qt::WindowCloseButtonHint))
			wflag |= B_NOT_CLOSABLE;
	}

    if (flags & Qt::WindowStaysOnTopHint)
        wfeel = B_FLOATING_ALL_WINDOW_FEEL;

	if (flags & Qt::WindowStaysOnTopHint &&
		flags & Qt::FramelessWindowHint &&
		tool) {
		wlook = B_NO_BORDER_WINDOW_LOOK;
		wflag |= B_WILL_ACCEPT_FIRST_CLICK | \
			B_AVOID_FRONT | \
			B_AVOID_FOCUS | \
			B_NO_WORKSPACE_ACTIVATION | \
			B_NOT_ANCHORED_ON_ACTIVATE;
		wfeel = B_FLOATING_ALL_WINDOW_FEEL;
	}

	m_window->SetLook(wlook);
	m_window->SetFeel(wfeel);
	m_window->SetFlags(wflag);
}


void QHaikuWindow::setWindowTitle(const QString &title)
{
	QString newTitle = QPlatformWindow::formatWindowTitle(title, QStringLiteral(" - "));
	newTitle = QPlatformTheme::removeMnemonics(newTitle).trimmed();
	m_window->SetTitle(newTitle.toUtf8());
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
		minW = minimumSize.width();
    if (minimumSize.height() > 0)
		minH = minimumSize.height();
    if (maximumSize.width() < QWINDOWSIZE_MAX)
		maxW = maximumSize.width();
    if (maximumSize.height() < QWINDOWSIZE_MAX)
		maxH = maximumSize.height();

	m_window->SetSizeLimits(minW, maxW, minH, maxH);
}


void QHaikuWindow::setGeometryImpl(const QRect &rect)
{
    QRect adjusted = rect;
    if (adjusted.width() <= 0)
        adjusted.setWidth(1);
    if (adjusted.height() <= 0)
        adjusted.setHeight(1);

    if (m_positionIncludesFrame) {
        adjusted.translate(m_margins.left(), m_margins.top());
    } else {
        // make sure we're not placed off-screen
        if (adjusted.left() < m_margins.left())
            adjusted.translate(m_margins.left(), 0);
        if (adjusted.top() < m_margins.top())
            adjusted.translate(0, m_margins.top());
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


void QHaikuWindow::setVisible(bool visible)
{
    if (visible == m_visible)
        return;

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

	m_window->Lock();
	if (window()->modality() == Qt::WindowModal ||
		window()->modality() == Qt::ApplicationModal) {
		m_window->SetFeel(B_MODAL_ALL_WINDOW_FEEL);
	}
	if (QGuiApplication::modalWindow() != NULL) {
		m_window->SetFeel(B_MODAL_ALL_WINDOW_FEEL);
	}
	if (visible) {
		if (window()->type() == Qt::Popup) {
			m_window->SetWorkspaces(B_CURRENT_WORKSPACE);
			m_window->Show();
			m_window->Activate(true);
	    } else
			m_window->Show();
    } else {
    	setWindowFlags(windowFlags);
		m_window->Hide();
    }

    m_window->Unlock();

    m_visible = visible;
}


void QHaikuWindow::requestActivateWindow()
{
    if (window()->type() != Qt::Desktop)
		m_window->Activate(true);
    QWindowSystemInterface::handleExposeEvent(window(), window()->geometry());
}


void QHaikuWindow::windowEvent(QEvent *event)
{
	switch (event->type()) {
		case QEvent::DynamicPropertyChange:
			if ( window()->property("size-grip") == true)
				m_window->PostMessage(kSizeGripEnable);
			else
				m_window->PostMessage(kSizeGripDisable);
			break;
		default:
			break;
	}

    QPlatformWindow::windowEvent(event);
}


QMargins QHaikuWindow::frameMargins() const
{
    return m_margins;
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

    switch (state) {
    case Qt::WindowFullScreen:
    	m_window->SetLook(B_NO_BORDER_WINDOW_LOOK);
        setGeometryImpl(screen()->geometry());
        break;
    case Qt::WindowMaximized:
    	m_normalGeometry = geometry();
		setWindowFlags(window()->flags());
		setGeometryImpl(screen()->availableGeometry().adjusted(m_margins.left(),
			m_margins.top(), -m_margins.right(), -m_margins.bottom()));
		break;
    case Qt::WindowMinimized:
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

QHaikuSurfaceView *QHaikuWindow::viewForWinId(WId id)
{
	if (id == -1)
		return NULL;
	QHaikuWindow *window = m_windowForWinIdHash.value(id, 0);
	if (window != NULL) {
		return window->m_window->View();
	}
    return NULL;
}

QHaikuWindow *QHaikuWindow::windowForWinId(WId id)
{
	return m_windowForWinIdHash.value(id, 0);
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
    
    if (m_visible) {
        QWindowSystemInterface::handleGeometryChange(window(), adjusted);
        QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(), adjusted.size()));
    } else {
        m_pendingGeometryChangeOnShow = true;
    }
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
            QWindowSystemInterface::handleWindowActivated(Q_NULLPTR);
		}
		return;
	}
	QWindowSystemInterface::handleWindowActivated(window());
	QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationActive);
}


void QHaikuWindow::platformWindowZoomed()
{
	if (window()->windowState() & Qt::WindowMaximized) {
		setWindowState(Qt::WindowNoState);
	} else {
		setWindowState(Qt::WindowMaximized);
	}
}

void QHaikuWindow::platformDropAction(BMessage *msg)
{
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
			Qt::CopyAction | Qt::MoveAction | Qt::LinkAction);

	if (response.isAccepted()) {
		const Qt::DropAction action = response.acceptedAction();
	}
}

void QHaikuWindow::platformEnteredView()
{
    QWindowSystemInterface::handleEnterEvent(window());
}

void QHaikuWindow::platformExitedView()
{
    QWindowSystemInterface::handleLeaveEvent(window());
}

void QHaikuWindow::platformMouseEvent(const QPoint &localPosition,
	const QPoint &globalPosition,
	Qt::MouseButtons buttons,
	Qt::KeyboardModifiers modifiers,
	Qt::MouseEventSource source)
{
    QWindowSystemInterface::handleMouseEvent(window(), localPosition, globalPosition, buttons, modifiers, source);
}

void QHaikuWindow::platformWheelEvent(const QPoint &localPosition,
	const QPoint &globalPosition,
	int delta,
	Qt::Orientation orientation,
	Qt::KeyboardModifiers modifiers)
{
    QWindowSystemInterface::handleWheelEvent(window(), localPosition, globalPosition, delta, orientation, modifiers);
}

void QHaikuWindow::platformKeyEvent(QEvent::Type type, int key, Qt::KeyboardModifiers modifiers, const QString &text)
{
    QWindowSystemInterface::handleKeyEvent(window(), type, key, modifiers, text);
}

QHash<WId, QHaikuWindow *> QHaikuWindow::m_windowForWinIdHash;

QT_END_NAMESPACE
