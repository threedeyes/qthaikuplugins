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

#include "qhaikubackingstore.h"
#include "qhaikuwindow.h"
#include "qhaikucursor.h"
#include "qhaikuintegration.h"
#include "qhaikudrag.h"

#include <QtGui/private/qpixmap_raster_p.h>
#include <QtGui/private/qguiapplication_p.h>

#include <qpa/qplatformcursor.h>
#include <qpa/qplatformwindow.h>
#include <qpa/qplatformdrag.h>

#include <private/qguiapplication_p.h>
#include <private/qdnd_p.h>

#include <private/qshapedpixmapdndwindow_p.h>
#include <private/qhighdpiscaling_p.h>

#include <QDesktopWidget>

QT_BEGIN_NAMESPACE

void QHaikuDrag::startDrag()
{
	QSimpleDrag::startDrag();

	QSimpleDrag::moveShapedPixmapWindow(QGuiApplication::primaryScreen()->availableGeometry().bottomRight() + drag()->hotSpot());

	QHaikuSurfaceView *view = QHaikuWindow::viewForWinId(m_sourceWindow->winId());

	BMessage dragMessage(B_SIMPLE_DATA);
	dragMessage.AddPointer("be:originator", view);
	if (drag()->supportedActions() & Qt::CopyAction)
		dragMessage.AddInt32("be:actions", B_COPY_TARGET);
	if (drag()->supportedActions() & Qt::MoveAction)
		dragMessage.AddInt32("be:actions", B_MOVE_TARGET);
	if (drag()->supportedActions() & Qt::LinkAction)
		dragMessage.AddInt32("be:actions", B_LINK_TARGET);

	dragMessage.AddInt32("qt:pid", getpid());

	for(const QString& format : drag()->mimeData()->formats()) {
		dragMessage.AddData(format.toUtf8().data(), B_MIME_TYPE,
			drag()->mimeData()->data(format).data(),
			drag()->mimeData()->data(format).size());
	}

	if (drag()->mimeData()->hasUrls()) {
		const auto urls = drag()->mimeData()->urls();
        for (const QUrl &url : urls) {
            if (url.isLocalFile()) {
            	BEntry path(url.toLocalFile().toUtf8().data());
            	entry_ref ref;
            	path.GetRef(&ref);
            	dragMessage.AddRef("refs", &ref);
            }
        }
	}

	if (view->LockLooperWithTimeout(10000)==B_OK) {
		view->SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
		if (drag()->pixmap().isNull()) {
			QPoint pos= QCursor::pos() + drag()->hotSpot();
			BRect r = BRect(pos.x(), pos.y(), pos.x() + 80, pos.y() + 24);
			view->SetMouseEventMask(B_POINTER_EVENTS);
			view->DragMessage(&dragMessage, view->ConvertFromScreen(r), view->Window());
		} else {
			QImage image = drag()->pixmap().toImage();
			BBitmap *bitmap = new BBitmap(BRect(0, 0, image.width() - 1, image.height() - 1), B_RGBA32);
			bitmap->SetBits((void*)image.bits(), image.sizeInBytes(), 0, B_RGBA32);
			view->SetMouseEventMask(B_POINTER_EVENTS);
			view->DragMessage(&dragMessage, bitmap, B_OP_ALPHA,
				BPoint(drag()->hotSpot().x(), drag()->hotSpot().y()), view->Window());
		}
		view->UnlockLooper();
	}
}

void QHaikuDrag::move(const QPoint &nativeGlobalPos, Qt::MouseButtons buttons,
                       Qt::KeyboardModifiers modifiers)
{
	QSimpleDrag::move(nativeGlobalPos, buttons, modifiers);
	QSimpleDrag::moveShapedPixmapWindow(QGuiApplication::primaryScreen()->availableGeometry().bottomRight() + drag()->hotSpot());
}


QT_END_NAMESPACE
