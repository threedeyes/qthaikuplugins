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

#include <qpa/qplatformscreen.h>
#include <qpa/qwindowsysteminterface.h>

#include <qguiapplication.h>
#include <qstatusbar.h>

#include <private/qwindow_p.h>

#include <qdebug.h>

QT_BEGIN_NAMESPACE

 static uint32 ScanCodes[] = {
        Qt::Key_Escape,		0x01,
        Qt::Key_F1,			0x02,
        Qt::Key_F2,			0x03,
        Qt::Key_F3,			0x04,
        Qt::Key_F4,			0x05,
        Qt::Key_F5,			0x06,
        Qt::Key_F6,			0x07,
        Qt::Key_F7,			0x08,
        Qt::Key_F8,			0x09,
        Qt::Key_F9,			0x0A,
        Qt::Key_F10,		0x0B,
        Qt::Key_F11,		0x0C,
        Qt::Key_F12,		0x0D,
        Qt::Key_Print,		0x0E,
//      Qt::Key_ScrollLock = 0x0F,  //modificator
        Qt::Key_Pause,		0x22,

		Qt::Key_AsciiTilde, 0x11,
        Qt::Key_1,			0x12,
        Qt::Key_2,			0x13,
        Qt::Key_3,			0x14,
        Qt::Key_4,			0x15,
        Qt::Key_5,			0x16,
        Qt::Key_6,			0x17,
        Qt::Key_7,			0x18,
        Qt::Key_8,			0x19,
        Qt::Key_9,			0x1A,
        Qt::Key_0,			0x1B,
        Qt::Key_Minus,		0x1C,
        Qt::Key_Plus,		0x1D,
        Qt::Key_Backspace,	0x1E,
        Qt::Key_Insert,		0x1F,
        Qt::Key_Home,		0x20,
        Qt::Key_PageUp,		0x21,
//		Qt::Key_NumLock,	0x22,   //modificator    
		Qt::Key_Slash,		0x23,
		Qt::Key_Asterisk,	0x24,
		Qt::Key_Minus,		0x25,		
		                
        Qt::Key_Tab,		0x26,        
        Qt::Key_Q,			0x27,
        Qt::Key_W,			0x28,
        Qt::Key_E,			0x29,
        Qt::Key_R,			0x2A,
        Qt::Key_T,			0x2B,
        Qt::Key_Y,			0x2C,
        Qt::Key_U,			0x2D,
        Qt::Key_I,			0x2E,
        Qt::Key_O,			0x2F,
        Qt::Key_P,			0x30,      
        Qt::Key_BracketLeft,0x31,
        Qt::Key_BracketRight,0x32,
		Qt::Key_Backslash,	0x33,
        Qt::Key_Delete,		0x34,
        Qt::Key_End,		0x35,
        Qt::Key_PageDown,	0x36, 
		Qt::Key_Home,		0x37, //numpad		      
        Qt::Key_Up,			0x38, //numpad
        Qt::Key_PageUp,		0x39, //numpad
        Qt::Key_Plus,		0x3A, //numpad

//		Qt::Key_CapsLock,	0x3B, //modificator
        Qt::Key_A,			0x3C,
        Qt::Key_S,			0x3D,
        Qt::Key_D,			0x3E,
        Qt::Key_F,			0x3F,
        Qt::Key_G,			0x40,
        Qt::Key_H,			0x41,
        Qt::Key_J,			0x42,
        Qt::Key_K,			0x43,
        Qt::Key_L,			0x44,
        Qt::Key_Colon,		0x45,
        Qt::Key_QuoteDbl,	0x46,
        Qt::Key_Return,		0x47,              
        Qt::Key_Left,		0x48, //numpad
		Qt::Key_5,			0x49, //numpad ???
        Qt::Key_Right,		0x4A, //numpad

        Qt::Key_Z,			0x4C,
        Qt::Key_X,			0x4D,
        Qt::Key_C,			0x4E,
        Qt::Key_V,			0x4F,
        Qt::Key_B,			0x50,
        Qt::Key_N,			0x51,
        Qt::Key_M,			0x51,
        Qt::Key_Less,		0x52,
        Qt::Key_Greater,	0x54,
        Qt::Key_Question,	0x55,
        Qt::Key_Up,			0x57,	//cursor
        Qt::Key_End,		0x58,	//numpad
        Qt::Key_Down,		0x59,   //numpad
        Qt::Key_PageDown,	0x5A,   //numpad
		Qt::Key_Enter,		0x5B,   //numpad

		Qt::Key_Space,		0x5E,
		Qt::Key_Left,		0x61,   //cursor
		Qt::Key_Down,		0x62,   //cursor
		Qt::Key_Right,		0x63,   //cursor
		Qt::Key_Insert,		0x64,   //cursor
		Qt::Key_Delete,		0x65,   //numpad
		0,					0x00
	};

static uint32 ScanCodes_Numlock[] = {
		Qt::Key_7,			0x37,
        Qt::Key_8,			0x38,
        Qt::Key_9,			0x39,
        Qt::Key_Plus,		0x3A,
        Qt::Key_4,			0x48,
		Qt::Key_5,			0x49,
        Qt::Key_6,			0x4A,
        Qt::Key_1,			0x58,
        Qt::Key_2,			0x59,
        Qt::Key_3,			0x5A,
		Qt::Key_Enter,		0x5B,
		Qt::Key_Comma,		0x65,
		0,					0x00
	};

static uint32 translateKeyCode(uint32 key)
{
	uint32 code = 0;
	uint32 i = 0;
	if(modifiers()&&B_NUM_LOCK) {
	    while (ScanCodes_Numlock[i]) {
	      		if ( key == ScanCodes_Numlock[i + 1]) {
	            code = ScanCodes_Numlock[i];
	      		    break;
	        	}
	       	i += 2;
	  	}	
		if(code>0)
			return code;   
	}
	
	i = 0;
    while (ScanCodes[i]) {
      		if ( key == ScanCodes[i + 1]) {
            code = ScanCodes[i];
      		    break;
        	}
       	i += 2;
  	}	
	return code;   
}


QtHaikuWindow::QtHaikuWindow(QHaikuWindow *qwindow,
		BRect frame,
		const char *title,
		window_look look,
		window_feel feel,
		uint32 flags)
		:BWindow(frame, title, look, feel, flags)
{
	fQWindow = qwindow;
	fView = new QHaikuSurfaceView(Bounds());
#if !defined(QT_NO_OPENGL)
	fGLView = NULL;
#endif	
 	AddChild(fView);
}

QtHaikuWindow::~QtHaikuWindow()
{
}


QHaikuSurfaceView *
QtHaikuWindow::View(void)
{
	return fView;
}


void
QtHaikuWindow::DispatchMessage(BMessage *msg, BHandler *handler)
{
	switch(msg->what) {
		case B_UNMAPPED_KEY_DOWN:
		case B_KEY_DOWN:
			{
				uint32 modifier = msg->FindInt32("modifiers");
				uint32 key = msg->FindInt32("key");
				
				QString text;
				
				const char* bytes;;
				if(msg->FindString("bytes", &bytes) == B_OK)
					text = QString::fromUtf8(bytes);
				
				Qt::KeyboardModifiers modifiers;
		        if (modifier & B_SHIFT_KEY)
		            modifiers |= Qt::ShiftModifier;
		
		        if (modifier & B_CONTROL_KEY)
		            modifiers |= Qt::AltModifier;
		
		        if (modifier & B_COMMAND_KEY)
		            modifiers |= Qt::ControlModifier;
		
				QWindowSystemInterface::handleWindowActivated(fQWindow->window());
		
		        QWindowSystemInterface::handleKeyEvent(0,
                                        QEvent::KeyPress,
                                        translateKeyCode(key),
                                        modifiers,
                                        text,
                                        false);

				break;	
			}
		case B_UNMAPPED_KEY_UP:
		case B_KEY_UP:
			{
				uint32 modifier = msg->FindInt32("modifiers");
				uint32 key = msg->FindInt32("key");
				
				QString text;
				
				const char* bytes;;
				if(msg->FindString("bytes", &bytes) == B_OK)
					text = QString::fromUtf8(bytes);
				
				Qt::KeyboardModifiers modifiers;
		        if (modifier & B_SHIFT_KEY)
		            modifiers |= Qt::ShiftModifier;
		
		        if (modifier & B_CONTROL_KEY)
		            modifiers |= Qt::AltModifier;
		
		        if (modifier & B_COMMAND_KEY)
		            modifiers |= Qt::ControlModifier;
		
		        QWindowSystemInterface::handleKeyEvent(0,
                                        QEvent::KeyRelease,
                                        translateKeyCode(key),
                                        modifiers,
                                        text,
                                        false);
				
				break;
			}	
	default:
		break;
	}			
	BWindow::DispatchMessage(msg, handler);
}


void QtHaikuWindow::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
	case B_MOUSE_WHEEL_CHANGED:
		{
			 float shift_x=0;
			 float shift_y=0;
			 if(msg->FindFloat("be:wheel_delta_x",&shift_x)!=B_OK)
			 	shift_x = 0;
			 if(msg->FindFloat("be:wheel_delta_y",&shift_y)!=B_OK)			 	
			 	shift_y = 0;
			 
			 if(shift_y !=0) {
			 	QWindowSystemInterface::handleWheelEvent(fQWindow->window(), fView->lastLocalMousePoint, fView->lastGlobalMousePoint, -shift_y * 96, Qt::Vertical);
			 }
			 if(shift_x !=0) {
			 	QWindowSystemInterface::handleWheelEvent(fQWindow->window(), fView->lastLocalMousePoint, fView->lastGlobalMousePoint, shift_x * 96, Qt::Horizontal);
			 }

			 break;
		}		
	default:
		BWindow::MessageReceived(msg);
		break;
	}
}


void
QtHaikuWindow::Zoom(BPoint origin, float w, float h)
{	
	if (fQWindow->window()->windowState() & Qt::WindowMaximized) {
		fQWindow->setWindowState(Qt::WindowNoState);
	} else {
		fQWindow->setWindowState(Qt::WindowMaximized);
	}
}


void
QtHaikuWindow::FrameResized(float width, float height)
{
	if (fQWindow->window()->windowState() & Qt::WindowMaximized) {
		QWindowSystemInterface::handleWindowStateChanged(fQWindow->window(), Qt::WindowNoState);
	}
	fQWindow->FrameResized(width, height);
}


void
QtHaikuWindow::FrameMoved(BPoint point)
{
	fQWindow->FrameMoved(point);
}


void
QtHaikuWindow::WindowActivated(bool active)
{
	if (!active) {
		QWindowSystemInterface::handleWindowActivated(Q_NULLPTR, Qt::OtherFocusReason);		
		QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationInactive);
		return;
	}

	QWindowSystemInterface::handleWindowActivated(fQWindow->window());	
	QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationActive);

	fQWindow->requestActivateWindow();
}


bool QtHaikuWindow::QuitRequested()
{
	QWindowSystemInterface::handleCloseEvent(fQWindow->window());
	return false;
}

QHaikuWindow::QHaikuWindow(QWindow *wnd)
    : QPlatformWindow(wnd)
    , m_positionIncludesFrame(false)
    , m_visible(false)
    , m_pendingGeometryChangeOnShow(true)
{	
//	qDebug() << "window()->surfaceType() " << window()->surfaceType() << window()->type();
	
	m_window = new QtHaikuWindow(this, BRect(wnd->geometry().left(),
    				wnd->geometry().top(),
    				wnd->geometry().right(),
    				wnd->geometry().bottom()),
    				wnd->title().toUtf8(),
    				B_NO_BORDER_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 0);
    
    setWindowFlags(wnd->flags());
    setWindowState(wnd->windowState());
    handleContentOrientationChange(wnd->contentOrientation());	 	
    setGeometry(wnd->geometry());

    QWindowSystemInterface::flushWindowSystemEvents();
    
    m_winId = (WId)this;
}


QHaikuWindow::~QHaikuWindow()
{
	m_window->Lock();
	m_window->Quit();
}


void
QHaikuWindow::setWindowFlags(Qt::WindowFlags flags)
{
	Qt::WindowType type =  static_cast<Qt::WindowType>(int(flags & Qt::WindowType_Mask)) ;
    
	bool popup = (type == Qt::Popup);
	bool splash = (type == Qt::SplashScreen);
	bool dialog = (type == Qt::Dialog
                   || type == Qt::Sheet
                   || (flags & Qt::MSWindowsFixedSizeDialogHint));
	bool tool = (type == Qt::Tool || type == Qt::Drawer);
	bool tooltip = (type == Qt::ToolTip);

	window_look wlook = dialog ? B_TITLED_WINDOW_LOOK : B_DOCUMENT_WINDOW_LOOK;
	window_feel wfeel = B_NORMAL_WINDOW_FEEL;
	uint32 wflag = B_NO_WORKSPACE_ACTIVATION | B_NOT_ANCHORED_ON_ACTIVATE;

	if (tool) {
		wlook = B_FLOATING_WINDOW_LOOK;
		wflag |= B_WILL_ACCEPT_FIRST_CLICK;
	}

	if (splash)
		wlook = B_NO_BORDER_WINDOW_LOOK;

	if (popup) {
		wlook = B_NO_BORDER_WINDOW_LOOK;
		wflag |= B_WILL_ACCEPT_FIRST_CLICK | B_AVOID_FRONT | B_AVOID_FOCUS;
		wfeel = window_feel(1025);
	}

	if (tooltip) {
		wlook = B_NO_BORDER_WINDOW_LOOK;
		wflag |= B_WILL_ACCEPT_FIRST_CLICK | B_AVOID_FRONT | B_AVOID_FOCUS;
		wfeel = window_feel(1025);
	}

    if (flags & Qt::FramelessWindowHint) {
    	wlook = B_NO_BORDER_WINDOW_LOOK;
    	wflag |= B_WILL_ACCEPT_FIRST_CLICK;
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

	m_window->SetLook(wlook);
	m_window->SetFeel(wfeel);
	m_window->SetFlags(wflag);
}


void
QHaikuWindow::setWindowTitle(const QString &title)
{
	m_window->SetTitle(title.toUtf8());
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
//    QSize baseSize = win->baseSize();
//    QSize sizeIncrement = win->sizeIncrement();
    
    float minW, maxW, minH, maxH;
    m_window->GetSizeLimits(&minW, &maxW, &minH, &maxH);
    
//    qDebug() << "G: " << minW << maxW << minH << maxH;
    
    if (minimumSize.width() > 0)
    	minW = minimumSize.width();
    if (minimumSize.height() > 0)
    	minH = minimumSize.height();    	
    if (maximumSize.width() < QWINDOWSIZE_MAX)
    	maxW = maximumSize.width();
    if (maximumSize.height() < QWINDOWSIZE_MAX)
    	maxH = maximumSize.height();
    	
//    qDebug() << "S: " << minW << maxW << minH << maxH;
   
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
	if (visible) {
		if (window()->type() == Qt::Popup) {
			m_window->SetWorkspaces(B_CURRENT_WORKSPACE);
			m_window->Show();
			m_window->Activate();
	    } else
			m_window->Show();
    } else
		m_window->Hide();

    m_window->Unlock();

    m_visible = visible;
}


void QHaikuWindow::requestActivateWindow()
{
    QHaikuWindow *focusWindow = 0;
    if (QGuiApplication::focusWindow())
        focusWindow = static_cast<QHaikuWindow*>(QGuiApplication::focusWindow()->handle());

    if (focusWindow == this)
        return;

    if (window()->type() != Qt::Desktop)
		m_window->Activate(true);
    
    QWindowSystemInterface::handleWindowActivated(window());
    QWindowSystemInterface::handleExposeEvent(window(), window()->geometry());
}


WId QHaikuWindow::winId() const
{
    return m_winId;
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

void QHaikuWindow::setWindowState(Qt::WindowState state)
{
    setFrameMarginsEnabled(state != Qt::WindowFullScreen);
    m_positionIncludesFrame = false;

    switch (state) {
    case Qt::WindowFullScreen:
        setGeometryImpl(screen()->geometry());
        break;
    case Qt::WindowMaximized:
    	m_normalGeometry = geometry();
		setGeometryImpl(screen()->availableGeometry().adjusted(m_margins.left(),
			m_margins.top(), -m_margins.right(), -m_margins.bottom()));
		break;
    case Qt::WindowMinimized:
        break;
    case Qt::WindowNoState:
        setGeometryImpl(m_normalGeometry);
        break;
    default:
        break;
    }

    QWindowSystemInterface::handleWindowStateChanged(window(), state);
}


QHaikuWindow *QHaikuWindow::windowForWinId(WId id)
{
    return (QHaikuWindow*)id;
}


void 
QHaikuWindow::FrameResized(float w, float h)
{
	QRect adjusted = geometry();
	adjusted.setWidth(w + 1);
	adjusted.setHeight(h + 1);
	
    QPlatformWindow::setGeometry(adjusted);
    
    if (m_visible) {
        QWindowSystemInterface::handleGeometryChange(window(), adjusted);
        QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(), adjusted.size()));
    } else {
        m_pendingGeometryChangeOnShow = true;
    }
}


void 
QHaikuWindow::FrameMoved(BPoint point)
{
	QRect adjusted = geometry();
	adjusted.moveTopLeft(QPoint(point.x, point.y));
	
    QPlatformWindow::setGeometry(adjusted);
        
    if (m_visible) {
        QWindowSystemInterface::handleGeometryChange(window(), adjusted);
        QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(), adjusted.size()));
    } else {
        m_pendingGeometryChangeOnShow = true;
    }
}

QT_END_NAMESPACE
