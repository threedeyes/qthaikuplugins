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

#include "qhaikutheme.h"

#include "qhaikuplatformdialoghelpers.h"
#include "qhaikusystemsettings.h"
#include "qhaikuintegration.h"
#include "qhaikusystemtrayicon.h"

#include <qstyle.h>
#include <qdebug.h>

#include <Rect.h>
#include <File.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Bitmap.h>

QT_BEGIN_NAMESPACE

QHaikuTheme::QHaikuTheme(const QHaikuIntegration *integration) : m_integration(integration)
{	
}

QHaikuTheme::~QHaikuTheme()
{
    qDeleteAll(m_fonts);
}

bool QHaikuTheme::usePlatformNativeDialog(DialogType type) const
{
	if (type == QPlatformTheme::MessageDialog)
		return true;
    if (type == QPlatformTheme::FileDialog)
        return false;
#if !defined(QT_NO_COLORDIALOG)
    if (type == QPlatformTheme::ColorDialog)
        return false;
#endif
#if !defined(QT_NO_FONTDIALOG)
    if (type == QPlatformTheme::FontDialog)
        return false;
#endif
    return false;
}

QPlatformDialogHelper *QHaikuTheme::createPlatformDialogHelper(DialogType type) const
{
    switch (type) {
	case QPlatformTheme::MessageDialog:
        return new QtHaikuDialogHelpers::QHaikuPlatformMessageDialogHelper;
    case QPlatformTheme::FileDialog:
    case QPlatformTheme::ColorDialog:
    case QPlatformTheme::FontDialog:
    default:
        return 0;
    }
}

QVariant QHaikuTheme::themeHint(ThemeHint hint) const
{
	switch (hint) {
	case QPlatformTheme::SystemIconThemeName:
		return QVariant(QString(QStringLiteral("haiku")));
    case QPlatformTheme::SystemIconFallbackThemeName:
        return QVariant(QString(QStringLiteral("breeze")));
    case QPlatformTheme::IconThemeSearchPaths:
    	{
    		QStringList paths;
    		paths << "/system/data/icons";
    		paths << "/boot/home/config/non-packaged/data/icons";
        	return paths;
    	}
    case QPlatformTheme::DialogButtonBoxButtonsHaveIcons:
        return QVariant(false);
    case QPlatformTheme::StyleNames:
    	{
    		QStringList styles;
    		styles << "haiku" << "fusion";
        	return styles;
    	}
    case QPlatformTheme::IconPixmapSizes:
    	{
        	QList<int> sizes;
        	sizes << 16 << 32;
        	return QVariant::fromValue(sizes);
    	}
    case QPlatformTheme::ContextMenuOnMouseRelease:
        return QVariant(false);
    default:
        return QPlatformTheme::themeHint(hint);
    }	
}


const QFont *QHaikuTheme::font(Font type) const
{
    QPlatformFontDatabase *fontDatabase = m_integration->fontDatabase();

    if (fontDatabase && m_fonts.isEmpty())
        m_fonts = qt_haiku_createRoleFonts(fontDatabase);
    return m_fonts.value(type, 0);
}

const QPalette *QHaikuTheme::palette(Palette type) const
{
    // Return the default palette
    if (type == SystemPalette)
        return &m_defaultPalette;

    return QPlatformTheme::palette(type);
}

QPixmap QHaikuTheme::standardPixmap(StandardPixmap sp, const QSizeF &size) const
{
	return QPlatformTheme::standardPixmap(sp, size);
}

QIcon QHaikuTheme::fileIcon(const QFileInfo &fileInfo, QPlatformTheme::IconOptions iconOptions) const
{
	QString filename = fileInfo.filePath();
	
	if (filename == "/")
		filename = "/boot/system/servers/mount_server";
		
	BFile file(filename.toUtf8(), O_RDONLY);
	BNodeInfo nodeInfo(&file);
	icon_size iconSize = B_LARGE_ICON;
	BRect rect(0, 0, iconSize - 1, iconSize -1);
	BBitmap bitmap(rect, B_RGBA32);
	if (nodeInfo.GetTrackerIcon(&bitmap, iconSize) == B_NO_ERROR) {
		QImage image(iconSize, iconSize, QImage::Format_ARGB32);
		memcpy((uchar*)image.bits(), (uchar*)bitmap.Bits(), iconSize * iconSize * 4);
		QPixmap pixmap = QPixmap::fromImage(image);
		QIcon icon(pixmap);
		return icon;
	}
	return QPlatformTheme::fileIcon(fileInfo, iconOptions);
}

#ifndef QT_NO_SYSTEMTRAYICON
QPlatformSystemTrayIcon *QHaikuTheme::createPlatformSystemTrayIcon() const
{
    return new QHaikuSystemTrayIcon;
}
#endif

QPlatformMenu *QHaikuTheme::createPlatformMenu() const
{
    return new QHaikuPlatformMenu;
}

QPlatformMenuItem *QHaikuTheme::createPlatformMenuItem() const
{
    return new QHaikuPlatformMenuItem;
}

QT_END_NAMESPACE
