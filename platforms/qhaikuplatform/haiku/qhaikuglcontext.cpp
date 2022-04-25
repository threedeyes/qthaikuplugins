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

#include "qhaikuglcontext.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaOpenGLContext, "qt.qpa.openglcontext", QtWarningMsg);

QHaikuGLContext::QHaikuGLContext(QOpenGLContext *context)
	: QPlatformOpenGLContext()
	, m_mesaContext(NULL)
	, m_shareContext(NULL)
{
    QVariant nativeHandle = context->nativeHandle();

	d_format = context->format();
	d_format.setDepthBufferSize(16);
	d_format.setStencilBufferSize(8);
	d_format.setRedBufferSize(8);
	d_format.setGreenBufferSize(8);
	d_format.setBlueBufferSize(8);
	d_format.setOption(QSurfaceFormat::DebugContext, false);
	d_format.setOption(QSurfaceFormat::StereoBuffers, false);
	d_format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);

	if (!nativeHandle.isNull()) {
		if (!nativeHandle.canConvert<QHaikuNativeGLContext>()) {
			qWarning(lcQpaOpenGLContext, "QOpenGLContext native handle must be a QHaikuNativeContext");
			return;
		}
		m_mesaContext = nativeHandle.value<QHaikuNativeGLContext>().context();
		if (!m_mesaContext) {
			qCWarning(lcQpaOpenGLContext, "QHaikuNativeContext's OSMesaContext cannot be null");
			return;
		}

		if (QPlatformOpenGLContext *shareContext = context->shareHandle())
			m_shareContext = static_cast<QHaikuGLContext *>(shareContext)->nativeContext();

		return;
	}

	if (d_format.renderableType() == QSurfaceFormat::DefaultRenderableType)
		d_format.setRenderableType(QSurfaceFormat::OpenGL);

	if (d_format.renderableType() != QSurfaceFormat::OpenGL)
		return;

	if (QPlatformOpenGLContext *shareHandle = context->shareHandle())
		m_shareContext = static_cast<QHaikuGLContext *>(shareHandle)->m_mesaContext;

	const int requestedVersion = (d_format.majorVersion() << 8) + d_format.minorVersion();
	int mesaProfile = OSMESA_COMPAT_PROFILE;
	if (requestedVersion >= 0x0302) {
		switch (d_format.profile()) {
		case QSurfaceFormat::NoProfile:
			break;
		case QSurfaceFormat::CoreProfile:
			mesaProfile = OSMESA_CORE_PROFILE;
			break;
		case QSurfaceFormat::CompatibilityProfile:
			mesaProfile = OSMESA_COMPAT_PROFILE;
			break;
		}
	}

	const int attribs[] = {
		OSMESA_FORMAT, OSMESA_BGRA,
		OSMESA_DEPTH_BITS, 16,
		OSMESA_STENCIL_BITS, 8,
		OSMESA_ACCUM_BITS, 0,
		OSMESA_PROFILE, mesaProfile,
		OSMESA_CONTEXT_MAJOR_VERSION, d_format.majorVersion(),
		OSMESA_CONTEXT_MINOR_VERSION, d_format.minorVersion(),
		0, 0
	};

	m_mesaContext = OSMesaCreateContextAttribs(attribs, m_shareContext);

	if (m_mesaContext == NULL)
		m_mesaContext = OSMesaCreateContextExt(OSMESA_BGRA, 16, 8, 0, m_shareContext);

	if (!m_mesaContext && m_shareContext) {
		qCWarning(lcQpaOpenGLContext, "Could not create OSMesaContext with shared context, "
			"falling back to unshared context.");
		m_mesaContext = OSMesaCreateContextExt(OSMESA_BGRA, 16, 8, 0, NULL);
		m_shareContext = NULL;
    }

	if (!m_mesaContext) {
		qCWarning(lcQpaOpenGLContext, "Failed to create OSMesaContext");
		return;
	}

	context->setNativeHandle(QVariant::fromValue<QHaikuNativeGLContext>(m_mesaContext));

	qCWarning(lcQpaOpenGLContext).verbosity(3) << "Created" << this << "based on requested" << context->format();
}


QHaikuGLContext::~QHaikuGLContext()
{
	if (m_mesaContext)
		OSMesaDestroyContext(m_mesaContext);
}


QFunctionPointer QHaikuGLContext::getProcAddress(const char *procName)
{
	return (QFunctionPointer)OSMesaGetProcAddress(procName);
}


bool QHaikuGLContext::makeCurrent(QPlatformSurface *surface)
{
	if (!surface->surface()->supportsOpenGL())
		return false;

	QSurface::SurfaceClass surfaceClass = surface->surface()->surfaceClass();
	QSize size = surface->surface()->size();

	void *pixelBuffer = NULL;

	if (surfaceClass == QSurface::Window) {
		QHaikuWindow *window = dynamic_cast<QHaikuWindow *>(surface);
		if (window->makeCurrent())
			pixelBuffer = window->openGLBuffer();
	} else {
		QHaikuOffscreenSurface *offscreenSurface = dynamic_cast<QHaikuOffscreenSurface *>(surface);
		if (offscreenSurface->isValid())
			pixelBuffer = offscreenSurface->openGLBuffer();
	}

	if (pixelBuffer == NULL)
		return false;

	if (!OSMesaMakeCurrent( m_mesaContext, pixelBuffer, GL_UNSIGNED_BYTE, size.width(), size.height()))
		return false;

	OSMesaPixelStore(OSMESA_Y_UP, 0);

	glViewport(0, 0, size.width(), size.height());

	return true;
}


void QHaikuGLContext::doneCurrent()
{
	OSMesaMakeCurrent(NULL, NULL, GL_UNSIGNED_BYTE, 0, 0);
}


void QHaikuGLContext::swapBuffers(QPlatformSurface *surface)
{
	QSurface::SurfaceClass surfaceClass = surface->surface()->surfaceClass();

	if (surfaceClass != QSurface::Window)
		return;

	QHaikuWindow *window = dynamic_cast<QHaikuWindow *>(surface);
	if (window == NULL)
		return;

	glFinish();

	QHaikuSurfaceView *view = QHaikuWindow::viewForWinId(window->window()->winId());
	window->swapBuffers();

	if (window->openGLBitmap() != NULL) {
		if (window->window()->isTopLevel()) {
			if (view->LockLooperWithTimeout(1000) == B_OK) {
				QHaikuWindow *topHaikuWin = QHaikuWindow::windowForWinId(window->topLevelWindow()->winId());

				view->SetDrawingMode(B_OP_COPY);
				BRegion region = topHaikuWin->getClippingRegion();
				view->ConstrainClippingRegion(&region);
				view->DrawBitmapAsync(window->openGLBitmap());
				BRegion allregion(BRect(0, 0, window->window()->width(), window->window()->height()));
				allregion.Exclude(&region);
				view->ConstrainClippingRegion(&allregion);

				for (int i = 0; i < topHaikuWin->fakeChildList()->size(); ++i) {
					QHaikuWindow *win = topHaikuWin->fakeChildList()->at(i);
					if (!win->window()->isTopLevel() && win->window()->isVisible()) {
						QPoint origin = win->mapToGlobal(QPoint()) - topHaikuWin->mapToGlobal(QPoint());
						view->DrawBitmapAsync(win->openGLBitmap(), BPoint(origin.x(), origin.y()));
					}
				}

				view->Sync();
				view->UnlockLooper();
		    }
		} else {
			QHaikuWindow *topWindow = QHaikuWindow::windowForWinId(window->topLevelWindow()->winId());
			view = QHaikuWindow::viewForWinId(window->topLevelWindow()->winId());

			QPoint origin = window->window()->mapToGlobal(QPoint()) - topWindow->window()->mapToGlobal(QPoint());
			QRect invalidateRect(origin.x(), origin.y(), window->window()->width(), window->window()->height());
			QWindowSystemInterface::handleExposeEvent(topWindow->window(), invalidateRect);
		}
	}
}


QSurfaceFormat QHaikuGLContext::format() const
{
    return d_format;
}


bool QHaikuGLContext::isSharing() const
{
    return m_shareContext != NULL;
}


bool QHaikuGLContext::isValid() const
{
    return m_mesaContext != NULL;
}

QT_END_NAMESPACE
