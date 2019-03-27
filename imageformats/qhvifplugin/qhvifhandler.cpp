/****************************************************************************
**
** Copyright (C) 2019 Gerasim Troeglazov,
** Contact: 3dEyes@gmail.com
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

#include "qhvifhandler.h"
#include <QDebug>
#include <QSize>


QHvifHandler::QHvifHandler(QIODevice *device)
{
	setDevice(device);
}

QHvifHandler::~QHvifHandler()
{
}

bool QHvifHandler::canRead() const
{
    if (canRead(device()))
        return true;
    return false;
}

bool QHvifHandler::canRead(QIODevice *device)
{
    bool can = device->peek(4) == QByteArray::fromHex("6e636966")
    	|| device->peek(8) == QByteArray::fromHex("494d5347484d4631");
    return can;
}

bool QHvifHandler::read(QImage *image)
{
	if (canRead()) {
	    QByteArray data = device()->readAll();
		BMemoryIO *mem = new BMemoryIO(data.constData(), data.size());
		bitmap = BTranslationUtils::GetBitmap(mem);
		delete mem;

	    QImage img = QImage(bitmap->Bounds().Width() + 1, bitmap->Bounds().Height() + 1, QImage::Format_ARGB32);
	    memcpy(img.bits(), bitmap->Bits(), bitmap->BitsLength());
	    *image = img;
	    delete bitmap;

	    return true;
	}
	return false;
}

/* NOT YET IMPLEMENTED */
bool QHvifHandler::supportsOption(ImageOption option) const
{
    Q_UNUSED(option);
    return false;
}

/* NOT YET IMPLEMENTED */
QVariant QHvifHandler::option(ImageOption option) const
{
    Q_UNUSED(option);
    return QVariant();
}

QByteArray QHvifHandler::name() const
{
    return "hvif";
}
