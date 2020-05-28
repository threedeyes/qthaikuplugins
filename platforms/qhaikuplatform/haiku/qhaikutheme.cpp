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
#include "qhaikusettings.h"
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
#include <InterfaceKit.h>

QT_BEGIN_NAMESPACE

// convert Haiku rgb_color to QColor
static QColor mkQColor(rgb_color rgb)
{
	return QColor(rgb.red, rgb.green, rgb.blue);
}

static void nativeColorSettings(QPalette &palette)
{
    rgb_color panel_background_color = ui_color(B_PANEL_BACKGROUND_COLOR);
    rgb_color control_text = ui_color(B_CONTROL_TEXT_COLOR);
	rgb_color control_text_disabled = control_text;
	control_text_disabled.red = (uint8)(((int32)panel_background_color.red + control_text_disabled.red + 1) / 2);
	control_text_disabled.green = (uint8)(((int32)panel_background_color.green + control_text_disabled.green + 1) / 2);
	control_text_disabled.blue = (uint8)(((int32)panel_background_color.blue + control_text_disabled.blue + 1) / 2);

    palette.setBrush(QPalette::Disabled, QPalette::WindowText, mkQColor(ui_color(B_PANEL_TEXT_COLOR)));
    palette.setBrush(QPalette::Disabled, QPalette::Button, mkQColor(ui_color(B_CONTROL_BACKGROUND_COLOR)));
    palette.setBrush(QPalette::Disabled, QPalette::Light, QColor(QRgb(0xffffffff)));
    palette.setBrush(QPalette::Disabled, QPalette::Midlight, QColor(QRgb(0xffffffff)));
    palette.setBrush(QPalette::Disabled, QPalette::Dark, QColor(QRgb(0xff555555)));
    palette.setBrush(QPalette::Disabled, QPalette::Mid, QColor(QRgb(0xffc7c7c7)));
    palette.setBrush(QPalette::Disabled, QPalette::Text, QColor(QRgb(0xffc7c7c7)));
    palette.setBrush(QPalette::Disabled, QPalette::BrightText, QColor(QRgb(0xffffffff)));
    palette.setBrush(QPalette::Disabled, QPalette::ButtonText, mkQColor(control_text_disabled));
    palette.setBrush(QPalette::Disabled, QPalette::Base, QColor(QRgb(0xffefefef)));
    palette.setBrush(QPalette::Disabled, QPalette::AlternateBase, palette.color(QPalette::Disabled, QPalette::Base).darker(110));
    palette.setBrush(QPalette::Disabled, QPalette::Window, mkQColor(panel_background_color));
    palette.setBrush(QPalette::Disabled, QPalette::Shadow, mkQColor(ui_color(B_SHADOW_COLOR)));
    palette.setBrush(QPalette::Disabled, QPalette::Highlight, mkQColor(ui_color(B_PANEL_BACKGROUND_COLOR)));
    palette.setBrush(QPalette::Disabled, QPalette::HighlightedText, mkQColor(ui_color(B_PANEL_TEXT_COLOR)));
    palette.setBrush(QPalette::Disabled, QPalette::Link, QColor(QRgb(0xff0000ee)));
    palette.setBrush(QPalette::Disabled, QPalette::LinkVisited, QColor(QRgb(0xff52188b)));

    palette.setBrush(QPalette::Active, QPalette::WindowText, mkQColor(ui_color(B_PANEL_TEXT_COLOR)));
    palette.setBrush(QPalette::Active, QPalette::Button, mkQColor(ui_color(B_CONTROL_BACKGROUND_COLOR)));
    palette.setBrush(QPalette::Active, QPalette::Light, mkQColor(ui_color(B_SHINE_COLOR)));
    palette.setBrush(QPalette::Active, QPalette::Midlight, QColor(QRgb(0xffffffff)));
    palette.setBrush(QPalette::Active, QPalette::Dark, QColor(QRgb(0xff555555)));
    palette.setBrush(QPalette::Active, QPalette::Mid, QColor(QRgb(0xffc7c7c7)));
    palette.setBrush(QPalette::Active, QPalette::Text, mkQColor(ui_color(B_DOCUMENT_TEXT_COLOR)));
    palette.setBrush(QPalette::Active, QPalette::BrightText, mkQColor(ui_color(B_SHINE_COLOR)));
    palette.setBrush(QPalette::Active, QPalette::ButtonText, mkQColor(control_text));
    palette.setBrush(QPalette::Active, QPalette::Base, mkQColor(ui_color(B_LIST_BACKGROUND_COLOR)));
    palette.setBrush(QPalette::Active, QPalette::AlternateBase, mkQColor(ui_color(B_LIST_BACKGROUND_COLOR)));
    palette.setBrush(QPalette::Active, QPalette::Window, mkQColor(panel_background_color));
    palette.setBrush(QPalette::Active, QPalette::Shadow, mkQColor(ui_color(B_SHADOW_COLOR)));
    palette.setBrush(QPalette::Active, QPalette::Highlight, mkQColor(ui_color(B_LIST_SELECTED_BACKGROUND_COLOR)));
    palette.setBrush(QPalette::Active, QPalette::HighlightedText, mkQColor(ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR)));
    palette.setBrush(QPalette::Active, QPalette::Link, mkQColor(ui_color(B_LINK_TEXT_COLOR)));
    palette.setBrush(QPalette::Active, QPalette::LinkVisited, mkQColor(ui_color(B_LINK_VISITED_COLOR)));
    palette.setBrush(QPalette::Active, QPalette::ToolTipBase, mkQColor(ui_color(B_TOOL_TIP_BACKGROUND_COLOR)));
    palette.setBrush(QPalette::Active, QPalette::ToolTipText, mkQColor(ui_color(B_TOOL_TIP_TEXT_COLOR)));

    palette.setBrush(QPalette::Inactive, QPalette::WindowText, mkQColor(ui_color(B_PANEL_TEXT_COLOR)));
    palette.setBrush(QPalette::Inactive, QPalette::Button, mkQColor(ui_color(B_CONTROL_BACKGROUND_COLOR)));
    palette.setBrush(QPalette::Inactive, QPalette::Light, mkQColor(ui_color(B_SHINE_COLOR)));
    palette.setBrush(QPalette::Inactive, QPalette::Midlight, QColor(QRgb(0xffffffff)));
    palette.setBrush(QPalette::Inactive, QPalette::Dark, QColor(QRgb(0xff555555)));
    palette.setBrush(QPalette::Inactive, QPalette::Mid, QColor(QRgb(0xffc7c7c7)));
    palette.setBrush(QPalette::Inactive, QPalette::Text, mkQColor(ui_color(B_DOCUMENT_TEXT_COLOR)));
    palette.setBrush(QPalette::Inactive, QPalette::BrightText, mkQColor(ui_color(B_SHINE_COLOR)));
    palette.setBrush(QPalette::Inactive, QPalette::ButtonText, mkQColor(control_text));
    palette.setBrush(QPalette::Inactive, QPalette::Base, mkQColor(ui_color(B_LIST_BACKGROUND_COLOR)));
    palette.setBrush(QPalette::Inactive, QPalette::AlternateBase, mkQColor(ui_color(B_LIST_BACKGROUND_COLOR)));
    palette.setBrush(QPalette::Inactive, QPalette::Window, mkQColor(panel_background_color));
    palette.setBrush(QPalette::Inactive, QPalette::Shadow, mkQColor(ui_color(B_SHADOW_COLOR)));
    palette.setBrush(QPalette::Inactive, QPalette::Highlight, mkQColor(ui_color(B_LIST_SELECTED_BACKGROUND_COLOR)));
    palette.setBrush(QPalette::Inactive, QPalette::HighlightedText, mkQColor(ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR)));
    palette.setBrush(QPalette::Inactive, QPalette::Link, mkQColor(ui_color(B_LINK_TEXT_COLOR)));
    palette.setBrush(QPalette::Inactive, QPalette::LinkVisited, mkQColor(ui_color(B_LINK_VISITED_COLOR)));
}

QHaikuTheme::QHaikuTheme(const QHaikuIntegration *integration) : m_integration(integration)
{
	nativeColorSettings(m_defaultPalette);
}

QHaikuTheme::~QHaikuTheme()
{
    qDeleteAll(m_fonts);
}

bool QHaikuTheme::usePlatformNativeDialog(DialogType type) const
{
	QSettings settings(QT_SETTINGS_FILENAME, QSettings::NativeFormat);
	settings.beginGroup("QPA");

	if (type == QPlatformTheme::MessageDialog)
		return settings.value("native_messages", true).toBool();
    if (type == QPlatformTheme::FileDialog)
        return settings.value("native_filepanel", false).toBool();
#if !defined(QT_NO_COLORDIALOG)
    if (type == QPlatformTheme::ColorDialog)
        return settings.value("native_colorpicker", false).toBool();
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
	QSettings settings(QT_SETTINGS_FILENAME, QSettings::NativeFormat);
	settings.beginGroup("Style");

	switch (hint) {
	case QPlatformTheme::SystemIconThemeName:
		return QVariant(settings.value("icons_iconset", "haiku").toString());
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
			styles << settings.value("widget_style", "haiku").toString() << "fusion";
        	return styles;
    	}
    case QPlatformTheme::IconPixmapSizes:
    	{
        	QList<int> sizes;
			sizes << settings.value("icons_small_size", 16).toInt();
			sizes << settings.value("icons_large_size", 32).toInt();
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
		
	BFile file(filename.toUtf8().constData(), O_RDONLY);
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
