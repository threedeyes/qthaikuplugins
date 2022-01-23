/****************************************************************************
**
** Copyright (C) 2021 Gerasim Troeglazov,
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

#include "qhaikunativeinterface.h"

#include "qhaikubackingstore.h"
#include "qhaikuwindow.h"
#include "qhaikuintegration.h"

#include <QtGui/QOpenGLContext>

#include <QtGui/QScreen>
#include <QtGui/QWindow>

QT_BEGIN_NAMESPACE

QHaikuNativeInterface::QHaikuNativeInterface(QHaikuIntegration *integration)
    : m_integration(integration)
{
	Q_UNUSED(integration)
}

void *QHaikuNativeInterface::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
	Q_UNUSED(resource)
	Q_UNUSED(window)
    return 0;
}

void *QHaikuNativeInterface::nativeResourceForScreen(const QByteArray &resource, QScreen *screen)
{
	Q_UNUSED(resource)
	Q_UNUSED(screen)
    return 0;
}

void *QHaikuNativeInterface::nativeResourceForIntegration(const QByteArray &resource)
{
	Q_UNUSED(resource)
    return 0;
}

void *QHaikuNativeInterface::nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context)
{
	Q_UNUSED(resource)
	Q_UNUSED(context)
    return 0;
}

void QHaikuNativeInterface::setWindowProperty(QPlatformWindow *window, const QString &name, const QVariant &value)
{
	Q_UNUSED(window)
	Q_UNUSED(name)
	Q_UNUSED(value)
}

QPlatformNativeInterface::NativeResourceForIntegrationFunction QHaikuNativeInterface::nativeResourceFunctionForIntegration(const QByteArray &resource)
{
    Q_UNUSED(resource)
    return 0;
}

QT_END_NAMESPACE
