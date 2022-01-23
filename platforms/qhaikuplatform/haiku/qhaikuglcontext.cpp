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

QHaikuGLContext::QHaikuGLContext(QOpenGLContext *context)
	: QPlatformOpenGLContext()  
{
	d_format = context->format();

    if (d_format.renderableType() == QSurfaceFormat::DefaultRenderableType)
		d_format.setRenderableType(QSurfaceFormat::OpenGL);

	if (d_format.renderableType() != QSurfaceFormat::OpenGL)
		return;

	glview = new BGLView(BRect(0, 0, 1, 1), "bglview",
		B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS, BGL_RGB | BGL_DOUBLE | BGL_DEPTH);
}

QHaikuGLContext::~QHaikuGLContext()
{
}

QFunctionPointer QHaikuGLContext::getProcAddress(const char *procName)
{
	void *ptr = glview->GetGLProcAddress(procName);
	return (QFunctionPointer)ptr;
}


bool QHaikuGLContext::makeCurrent(QPlatformSurface *surface)
{
	QSize size = surface->surface()->size();
	QHaikuWindow *window = static_cast<QHaikuWindow *>(surface);
	if (!window)
		return false;

	if (window->m_window->fGLView == NULL) {
		window->m_window->fGLView = glview;
		window->m_window->Lock();
		window->m_window->AddChild(glview);
		glview->LockGL();
		glview->ResizeTo(size.width(),size.height());
		window->m_window->Unlock();
	}

	return true;
}

void QHaikuGLContext::doneCurrent()
{
}

void QHaikuGLContext::swapBuffers(QPlatformSurface *surface)
{
	QHaikuWindow *window = static_cast<QHaikuWindow *>(surface);
	if (window) {
		glview->UnlockGL();
		glview->SwapBuffers();
		glview->LockGL();
	}
}

QSurfaceFormat QHaikuGLContext::format() const
{
    return d_format;
}

bool QHaikuGLContext::isSharing() const
{
    return false;
}

bool QHaikuGLContext::isValid() const
{
    return true;
}

QT_END_NAMESPACE
