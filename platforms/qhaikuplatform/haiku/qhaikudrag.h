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

#ifndef QHAIKUDRAGSTORE_H
#define QHAIKUDRAGSTORE_H

#include "qhaikucursor.h"
#include "qhaikuwindow.h"

#include <qpa/qplatformbackingstore.h>
#include <qpa/qplatformdrag.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformwindow.h>
#include <qpa/qplatformdrag.h>

#include <QtGui/private/qsimpledrag_p.h>
#include <private/qdnd_p.h>

#include <qscopedpointer.h>
#include <qimage.h>
#include <qbitmap.h>
#include <qpainter.h>
#include <qhash.h>

#include <View.h>
#include <Bitmap.h>
#include <Rect.h>
#include <Region.h>

QT_BEGIN_NAMESPACE

class QShapedPixmapWindow;

class QHaikuDrag : public QSimpleDrag
{
public:
    QHaikuDrag() {};

protected:
	virtual void startDrag() override;
	virtual void move(const QPoint &globalPos, Qt::MouseButtons b, Qt::KeyboardModifiers mods) override;
};

QT_END_NAMESPACE

#endif // QHAIKUDRAGSTORE_H
