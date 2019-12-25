/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2015-2019 Gerasim Troeglazov,
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

#ifndef QHAIKUINTEGRATION_H
#define QHAIKUINTEGRATION_H

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformopenglcontext.h>
#include <qscopedpointer.h>

//OpenGL support disabled for now
#define QT_NO_OPENGL

#include "qhaikuclipboard.h"
#include "qhaikucommon.h"

#if !defined(QT_NO_OPENGL)
#include <GLView.h>
#endif

QT_BEGIN_NAMESPACE

class QSimpleDrag;
class QHaikuBackendData;
class QHaikuSystemLocale;

#if !defined(QT_NO_OPENGL)
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
private:
	QSurfaceFormat d_format;
	BGLView *glview;
};
#endif

class QHaikuIntegration : public QObject, public QPlatformIntegration
{
	Q_OBJECT
public:
    QHaikuIntegration(const QStringList &parameters, int &argc, char **argv);
    ~QHaikuIntegration();

    bool hasCapability(QPlatformIntegration::Capability cap) const;

    QPlatformWindow *createPlatformWindow(QWindow *window) const;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const;
#if !defined(QT_NO_OPENGL)    
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const;
#endif    
    
    QPlatformClipboard *clipboard() const;

    QPlatformDrag *drag() const;
    QPlatformServices *services() const;

    QPlatformFontDatabase *fontDatabase() const;
    QAbstractEventDispatcher *createEventDispatcher() const;

    static QHaikuIntegration *createHaikuIntegration(const QStringList& parameters, int &argc, char **argv);

    QStringList themeNames() const;
    QPlatformTheme *createPlatformTheme(const QString &name) const; 

private:
    QPlatformFontDatabase *m_fontDatabase;
    QSimpleDrag *m_drag;
    QPlatformServices *m_services;
    QHaikuSystemLocale *m_haikuSystemLocale;
    QHaikuScreen *m_screen;
    mutable QHaikuClipboard* m_clipboard;
private Q_SLOTS:
	bool platformAppQuit();
};

QT_END_NAMESPACE

#endif
