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

#include "qhaikucommon.h"
#include "qhaikuwindow.h"
#include "qhaikucursor.h"

#include <QtGui/private/qpixmap_raster_p.h>
#include <QtGui/private/qguiapplication_p.h>

#include <qpa/qplatformcursor.h>
#include <qpa/qplatformwindow.h>

#include <qdebug.h>

QT_BEGIN_NAMESPACE

QHaikuScreen::QHaikuScreen() 
	: QPlatformScreen()
	, m_screen(new BScreen(B_MAIN_SCREEN_ID))
    , m_cursor(new QHaikuCursor)
{
	Q_ASSERT(m_screen->IsValid());
}


QHaikuScreen::~QHaikuScreen()
{
    delete m_cursor;
    delete m_screen;
}


QPlatformCursor *QHaikuScreen::cursor() const
{
    return m_cursor;
}


QRect QHaikuScreen::geometry() const
{	
	const BRect frame = m_screen->Frame();
    return QRect(frame.left, frame.top, frame.Width() + 1, frame.Height() + 1);
}


QPixmap QHaikuScreen::grabWindow(WId id, int x, int y, int width, int height) const
{
    QRect rect(x, y, width, height);
	
    QHaikuWindow *window = QHaikuWindow::windowForWinId(id);

    if (!window || window->window()->type() == Qt::Desktop) {
    	if (width < 0 || height < 0) {
    		width = geometry().width() - x;
    		height = geometry().height() - y;
    	}
    	BBitmap *bitmap = NULL;
    	BRect rect(x, y, x + width, y + height);
    	m_screen->GetBitmap(&bitmap, false, &rect);
    	if (bitmap->IsValid()) {
    		QImage image((uchar*)bitmap->Bits(), \
    			width, height, \
    			bitmap->BytesPerRow(), \
    			QImage::Format_RGB32);
    		image = image.copy(QRect(0, 0, width, height));
    		delete bitmap;
    		return QPixmap::fromImage(image);
    	}
		return QPixmap();
    }

    QHaikuBackingStore *store = QHaikuBackingStore::backingStoreForWinId(id);
    if (store)
        return store->grabWindow(id, rect);

    return QPixmap();
}


QHaikuBackingStore::QHaikuBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
{
	BRect rect(0, 0, window->width() - 1, window->height() - 1);
	m_bitmap = new BBitmap(rect, B_RGB32);	
	m_image = QImage((uchar*)m_bitmap->Bits(), window->width(), window->height(), m_bitmap->BytesPerRow(), QImage::Format_RGB32);
}


QHaikuBackingStore::~QHaikuBackingStore()
{
	m_image = QImage();
	delete m_bitmap;
	
    clearHash();
}


QPaintDevice *QHaikuBackingStore::paintDevice()
{
    return &m_image;
}


void QHaikuBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(region);

    if (m_image.size().isEmpty())
        return;

    QSize imageSize = m_image.size();

    QRegion clipped = QRect(0, 0, window->width(), window->height());
    clipped &= QRect(0, 0, imageSize.width(), imageSize.height()).translated(-offset);

    QRect bounds = clipped.boundingRect().translated(offset);

    if (bounds.isNull())
        return;

    WId id = window->winId();
	QHaikuSurfaceView *view = QHaikuWindow::viewForWinId(id);

    QRect outline = region.boundingRect();
    
    if(view->LockLooper()) {
    	view->SetViewBitmap(m_bitmap);
    	BRect rect(outline.left(), outline.top(), outline.right(), outline.bottom());
    	view->DrawBitmap(m_bitmap, rect, rect);
    	view->UnlockLooper();
    }
        
    m_windowAreaHash[id] = bounds;
    m_backingStoreForWinIdHash[id] = this;
}


void QHaikuBackingStore::resize(const QSize &size, const QRegion &)
{
    WId id = window()->winId();

    QImage::Format format = QGuiApplication::primaryScreen()->handle()->format();
    if (m_image.size() != size) {
		QHaikuSurfaceView *view = QHaikuWindow::viewForWinId(id);
    	if(view->LockLooper()) {
    		m_image = QImage();
	        delete m_bitmap;

			BRect rect(0, 0, size.width() - 1, size.height() - 1);
			m_bitmap = new BBitmap(rect, B_RGB32, true);
	    	view->SetViewBitmap(m_bitmap);
			m_image = QImage((uchar*)m_bitmap->Bits(), size.width(), size.height(), m_bitmap->BytesPerRow(), QImage::Format_RGB32);			
			view->UnlockLooper();
    	}
    }
    clearHash();        
}


bool QHaikuBackingStore::scroll(const QRegion &area, int dx, int dy)
{
    if (m_image.isNull())
        return false;

    const QVector<QRect> rects = area.rects();
    for (int i = 0; i < rects.size(); ++i)
        qt_scrollRectInImage(m_image, rects.at(i), QPoint(dx, dy));

    return true;
}


QPixmap QHaikuBackingStore::grabWindow(WId window, const QRect &rect) const
{
    QRect area = m_windowAreaHash.value(window, QRect());
    if (area.isNull())
        return QPixmap();

    QRect adjusted = rect;
    if (adjusted.width() <= 0)
        adjusted.setWidth(area.width());
    if (adjusted.height() <= 0)
        adjusted.setHeight(area.height());

    adjusted = adjusted.translated(area.topLeft()) & area;

    if (adjusted.isEmpty())
        return QPixmap();

    return QPixmap::fromImage(m_image.copy(adjusted));
}


QHaikuBackingStore *QHaikuBackingStore::backingStoreForWinId(WId id)
{
    return m_backingStoreForWinIdHash.value(id, 0);
}


void QHaikuBackingStore::clearHash()
{
    QList<WId> ids = m_windowAreaHash.keys();
    foreach (WId id, ids) {
        QHash<WId, QHaikuBackingStore *>::iterator it = m_backingStoreForWinIdHash.find(id);
        if (it.value() == this)
            m_backingStoreForWinIdHash.remove(id);
    }
    m_windowAreaHash.clear();
}


QHash<WId, QHaikuBackingStore *> QHaikuBackingStore::m_backingStoreForWinIdHash;


QT_END_NAMESPACE
