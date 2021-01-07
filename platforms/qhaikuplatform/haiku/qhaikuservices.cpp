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

#include "qhaikuservices.h"

#include <qdebug.h>

#include <QtCore/QUrl>
#include <QtCore/QDir>
#include <QtCore/QCoreApplication>
#include <QProcess>
#include <Roster.h>

#include <stdlib.h>

QT_BEGIN_NAMESPACE

QHaikuServices::QHaikuServices()
{
}

QHaikuServices::~QHaikuServices()
{
}

bool QHaikuServices::openUrl(const QUrl &url)
{
	QString argument;
	QString scheme = url.scheme();
	
	qDebug() << "openUrl" << url;

    if (scheme == QLatin1String("file")) {
		const QString nativeFilePath = url.isLocalFile() && !url.hasFragment() && !url.hasQuery()
        ? QDir::toNativeSeparators(url.toLocalFile())
        : url.toString(QUrl::FullyEncoded);
        argument = nativeFilePath;
    } else
		argument = url.toString();

    QByteArray urlData = argument.toLocal8Bit();
    char *rawUrlData = urlData.data();

	entry_ref ref;
	if (get_ref_for_path("/boot/system/apps/WebPositive", &ref))
		return false;

	const char* args[] = { "/boot/system/apps/WebPositive", rawUrlData, NULL };
	if (be_roster->Launch(&ref, 2, args) != B_OK)
		return false;

    return true;
}

bool QHaikuServices::openDocument(const QUrl &url)
{
	QString argument;
	QString scheme = url.scheme();
	
	qDebug() << "openDocument" << url;

    if (scheme == QLatin1String("file")) {
		const QString nativeFilePath = url.isLocalFile() && !url.hasFragment() && !url.hasQuery()
        ? QDir::toNativeSeparators(url.toLocalFile())
        : url.toString(QUrl::FullyEncoded);
        argument = nativeFilePath;
    } else
		argument = url.toString();

    QByteArray urlData = argument.toLocal8Bit();
    char *rawUrlData = urlData.data();

	entry_ref ref;
	if (get_ref_for_path("/bin/open", &ref))
		return false;

	const char* args[] = { "/bin/open", rawUrlData, NULL };
	if (be_roster->Launch(&ref, 2, args) != B_OK)
		return false;

    return true;
}

QByteArray QHaikuServices::desktopEnvironment() const
{
    return QByteArray("GNOME");
}

QT_END_NAMESPACE
