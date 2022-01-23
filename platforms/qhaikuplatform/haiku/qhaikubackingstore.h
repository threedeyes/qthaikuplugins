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

#ifndef QHAIKUBACKINGSTORE_H
#define QHAIKUBACKINGSTORE_H

#include "qhaikucursor.h"

#include <qpa/qplatformbackingstore.h>
#include <qpa/qplatformdrag.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformwindow.h>

#include <qscopedpointer.h>
#include <qimage.h>
#include <qbitmap.h>
#include <qpainter.h>
#include <qhash.h>

#include <View.h>
#include <Bitmap.h>

extern void qt_scrollRectInImage(QImage &img, const QRect &rect, const QPoint &offset);

QT_BEGIN_NAMESPACE

class QHaikuDrag : public QPlatformDrag
{
public:
    QMimeData *platformDropData() { return 0; }
    Qt::DropAction drag(QDrag *) { return Qt::IgnoreAction; }
};

class QHaikuBackingStore : public QPlatformBackingStore
{
public:
    QHaikuBackingStore(QWindow *window);
    ~QHaikuBackingStore();

    QPaintDevice *paintDevice() override;
    void flush(QWindow *window, const QRegion &region, const QPoint &offset) override;
    void resize(const QSize &size, const QRegion &staticContents);
    bool scroll(const QRegion &area, int dx, int dy) override;
    void composeAndFlush(QWindow *window, const QRegion &region, const QPoint &offset,
                                       QPlatformTextureList *textures,
                                       bool translucentBackground) override;
	QImage toImage() const override { return m_image; }

    QPixmap grabWindow(WId window, const QRect &rect) const;

    static QHaikuBackingStore *backingStoreForWinId(WId id);

private:
    void clearHash();

    QImage m_image;
    BBitmap *m_bitmap;
    
    QHash<WId, QRect> m_windowAreaHash;

    static QHash<WId, QHaikuBackingStore *> m_backingStoreForWinIdHash;
};

QT_END_NAMESPACE

#endif // QHAIKUBACKINGSTORE_H
