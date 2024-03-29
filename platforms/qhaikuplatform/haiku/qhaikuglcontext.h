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

#ifndef QHAIKUGLCONTEXT_H
#define QHAIKUGLCONTEXT_H

#include <qpa/qplatformopenglcontext.h>

#include "qhaikuapplication.h"
#include "qhaikuwindow.h"
#include "qhaikubackingstore.h"
#include "qhaikuoffscreensurface.h"
#include "qhaikuscreen.h"

#include "GL/gl.h"
#include "GL/glu.h"
#include "GL/osmesa.h"

QT_BEGIN_NAMESPACE

struct QHaikuNativeGLContext
{
    QHaikuNativeGLContext()
        : m_context(0)
    { }

    QHaikuNativeGLContext(OSMesaContext ctx)
        : m_context(ctx)
    { }

    OSMesaContext context() const { return m_context; }

private:
    OSMesaContext m_context;
};


class QHaikuGLContext : public QPlatformOpenGLContext
{
public:
    QHaikuGLContext(QOpenGLContext *context);
    ~QHaikuGLContext();

    bool makeCurrent(QPlatformSurface *surface) override;
    void doneCurrent() override;
    void swapBuffers(QPlatformSurface *surface) override;
    QFunctionPointer getProcAddress(const char *procName) override;

    QSurfaceFormat format() const override;
    bool isSharing() const override;
    bool isValid() const override;

    OSMesaContext nativeContext() const { return m_mesaContext; }

	OSMesaContext m_mesaContext;
	OSMesaContext m_shareContext;
   
private:
	QSurfaceFormat d_format;
};

Q_DECLARE_METATYPE(QHaikuNativeGLContext)

QT_END_NAMESPACE

#endif // QHAIKUGLCONTEXT_H
