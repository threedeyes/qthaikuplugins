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

#if !defined(QT_NO_CLIPBOARD)

#include "qhaikuclipboard.h"

#include <QColor>
#include <QMimeData>
#include <QStringList>
#include <QUrl>
#include <QImage>
#include <QTextDocumentFragment>
#include <QDebug>

#include <Application.h>
#include <String.h>
#include <Clipboard.h>
#include <Messenger.h>

QT_BEGIN_NAMESPACE

QHaikuClipboard::QHaikuClipboard()
    : m_systemMimeData(nullptr)
    , m_userMimeData(nullptr)
    , preventChangedEvent(false)
{
	status_t err;
	err = be_clipboard->StartWatching(be_app_messenger);
	if (err != B_OK)
		qDebug() << "ERROR: be_clipboard->StartWatching: " << err;

	BMessage *msg = new BMessage('CLIP');
	msg->AddPointer("QHaikuClipboard", (void*)this);
	be_app->PostMessage(msg);
}

QHaikuClipboard::~QHaikuClipboard()
{
	be_clipboard->StopWatching(be_app_messenger);
	delete m_userMimeData;
	delete m_systemMimeData;
}

void QHaikuClipboard::setMimeData(QMimeData *mimeData, QClipboard::Mode mode)
{
    if (mode != QClipboard::Clipboard)
        return;

    if (mimeData) {
        if (m_systemMimeData == mimeData)
            return;

        if (m_userMimeData == mimeData)
            return;
    }

	if (be_clipboard->Lock()) {
		be_clipboard->Clear();
		if (mimeData) {
			BMessage* clip = (BMessage *)NULL;
	    	if( (clip = be_clipboard->Data()) != NULL) {
				bool textPlain = false;
				QStringList formats = mimeData->formats();
				for (int format = 0; format < formats.size(); ++format) {
					QString mimeType = formats.at(format);
					clip->AddData(mimeType.toUtf8().constData(), B_MIME_TYPE, mimeData->data(mimeType).data(), mimeData->data(mimeType).count());
					if (mimeType == "text/plain")
						textPlain = true;
				}
				if (!textPlain && mimeData->hasHtml()) {
					QString body = mimeData->html();
					body = body.mid(body.indexOf("<body"));
					body = body.mid(body.indexOf(">") + 1);
					body = body.left(body.indexOf("</body>"));
					body = body.remove("\n");
					QString plainText = QTextDocumentFragment::fromHtml(body).toPlainText();
					clip->AddData("text/plain", B_MIME_TYPE, plainText.toUtf8().constData(), plainText.toUtf8().count() + 1);
				}
			}
		}

		preventChangedEvent = true;
		m_userMimeData = mimeData;

		be_clipboard->Commit();
	   	be_clipboard->Unlock();

		emitChanged(QClipboard::Clipboard);
	}
}

QMimeData *QHaikuClipboard::mimeData(QClipboard::Mode mode)
{
    if (mode != QClipboard::Clipboard)
        return 0;

    if (m_userMimeData)
        return m_userMimeData;

    if (!m_systemMimeData)
        m_systemMimeData = new QMimeData();
    else
        m_systemMimeData->clear();

	BMessage* clip = (BMessage *)NULL;
  	if (be_clipboard->Lock()) {
    	if( (clip = be_clipboard->Data()) != NULL) {
    		BMessage *msg = (BMessage*)(be_clipboard->Data());

			char *name;
			uint32 type;
			int32 count;

			for ( int i = 0; msg->GetInfo(B_ANY_TYPE, i, &name, &type, &count) == B_OK; i++ ) {
				const void *data;
				ssize_t dataLen = 0;
				status_t stat = msg->FindData(name, B_ANY_TYPE, &data, &dataLen);

				if(dataLen && stat==B_OK) {
					QString mime(name);
					if (mime == "text/plain") {
						QString text = QString::fromUtf8((const char*)data, dataLen);
						m_systemMimeData->setText(text);
					} else if (mime == "text/html") {
						QString html = QString::fromUtf8((const char*)data, dataLen);
						m_systemMimeData->setHtml(html);
					} else if (mime == "image/bitmap") {
						BMessage flatten;
						if (msg->FindMessage("image/bitmap", &flatten) == B_OK) {
							BBitmap* bitmap = new(std::nothrow) BBitmap(&flatten);
							if (bitmap) {
								QImage::Format imageFormat = QImage::Format_Invalid;
								switch (bitmap->ColorSpace()) {
									case B_RGB32:
										imageFormat = QImage::Format_RGB32;
										break;
									case B_RGBA32:
										imageFormat = QImage::Format_ARGB32;
										break;
									case B_RGB24:
										imageFormat = QImage::Format_RGB888;
										break;
									case B_RGB16:
										imageFormat = QImage::Format_RGB16;
										break;
									case B_GRAY8:
										imageFormat = QImage::Format_Grayscale8;
										break;
									case B_GRAY1:
										imageFormat = QImage::Format_Mono;
										break;
									default:
										imageFormat = QImage::Format_Invalid;
										break;
								}
								if (imageFormat != QImage::Format_Invalid) {
									QSize bitmapSize(bitmap->Bounds().IntegerWidth() + 1,
										bitmap->Bounds().IntegerHeight() + 1);
									QImage image(bitmapSize, imageFormat);
									memcpy(image.bits(), bitmap->Bits(), image.sizeInBytes());
									m_systemMimeData->setImageData(image);
								}
								delete bitmap;
							}
						}
					} else {
						QByteArray clip_data((const char*)data, dataLen);
						m_systemMimeData->setData(mime, clip_data);
					}
				}
			}
			be_clipboard->Unlock();
    	}
	}
	return m_systemMimeData;
}

bool QHaikuClipboard::supportsMode(QClipboard::Mode mode) const
{
    return (mode == QClipboard::Clipboard);
}

bool QHaikuClipboard::ownsMode(QClipboard::Mode mode) const
{
    Q_UNUSED(mode);
    return false;
}

void QHaikuClipboard::clipboardChanged()
{
	if (!preventChangedEvent) {
		delete m_userMimeData;
		m_userMimeData = nullptr;
		emitChanged(QClipboard::Clipboard);
	}
	preventChangedEvent = false;
}

QT_END_NAMESPACE

#endif //QT_NO_CLIPBOARD
