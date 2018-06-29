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

#include "qhaikusystemsettings.h"

#include <QFont>
#include <qpa/qplatformfontdatabase.h>
#include <qdebug.h>

#include <View.h>
#include <Font.h>
#include <Menu.h>

QT_BEGIN_NAMESPACE

extern status_t _get_system_font_(const char *which, font_family family, font_style style, float *size);


QHash<QPlatformTheme::Font, QFont *> qt_haiku_createRoleFonts(QPlatformFontDatabase *fontDatabase)
{
	QFontDatabase db;

    font_family plainFontFamily;
    font_style plainFontStyle;
	BFont haikuPlainFont = *be_plain_font;
	haikuPlainFont.GetFamilyAndStyle(&plainFontFamily, &plainFontStyle);

    font_family boldFontFamily;
    font_style boldFontStyle;
	BFont haikuBoldFont = *be_bold_font;
	haikuBoldFont.GetFamilyAndStyle(&boldFontFamily, &boldFontStyle);

    font_family fixedFontFamily;
    font_style fixedFontStyle;
	BFont haikuFixedFont = *be_fixed_font;
	haikuFixedFont.GetFamilyAndStyle(&fixedFontFamily, &fixedFontStyle);

	menu_info haikuMenuFontInfo;
	get_menu_info(&haikuMenuFontInfo);

	float kDPI = 70.0 / 96.0;
	float kTitleBold = 0.96;

    QFont baseFont = db.font(plainFontFamily, plainFontStyle, haikuPlainFont.Size() * kDPI);
    if (haikuPlainFont.Size() >= 0)
		baseFont.setPointSizeF(haikuPlainFont.Size() * kDPI);
    baseFont.setStretch(QFont::Unstretched);

    QFont boldFont = db.font(boldFontFamily, boldFontStyle, haikuBoldFont.Size() * kDPI * kTitleBold);
    if (haikuBoldFont.Size() >= 0)
		boldFont.setPointSizeF(haikuBoldFont.Size() * kDPI * kTitleBold);
    boldFont.setStretch(QFont::Unstretched);
    boldFont.setBold(true);

    QFont monoFont = db.font(fixedFontFamily, fixedFontStyle, haikuFixedFont.Size() * kDPI);
    if (haikuFixedFont.Size() >= 0)
		monoFont.setPointSizeF(haikuFixedFont.Size() * kDPI);
    monoFont.setStretch(QFont::Unstretched);

    QFont menuFont = db.font(haikuMenuFontInfo.f_family, haikuMenuFontInfo.f_style, haikuMenuFontInfo.font_size * kDPI);
    if (haikuMenuFontInfo.font_size >= 0)
		menuFont.setPointSizeF(haikuMenuFontInfo.font_size * kDPI);
    menuFont.setStretch(QFont::Unstretched);

    QHash<QPlatformTheme::Font, QFont *> fonts;
    fonts.insert(QPlatformTheme::SystemFont, new QFont(baseFont));
    fonts.insert(QPlatformTheme::PushButtonFont, new QFont(baseFont));
    fonts.insert(QPlatformTheme::ListViewFont, new QFont(baseFont));
    fonts.insert(QPlatformTheme::ListBoxFont, new QFont(baseFont));
    fonts.insert(QPlatformTheme::TitleBarFont, new QFont(boldFont));
    fonts.insert(QPlatformTheme::GroupBoxTitleFont, new QFont(boldFont));
    fonts.insert(QPlatformTheme::MdiSubWindowTitleFont, new QFont(boldFont));
    fonts.insert(QPlatformTheme::MenuFont, new QFont(menuFont));
    fonts.insert(QPlatformTheme::MenuBarFont, new QFont(menuFont));
    fonts.insert(QPlatformTheme::ComboMenuItemFont, new QFont(menuFont));
    fonts.insert(QPlatformTheme::HeaderViewFont, new QFont(baseFont));
    fonts.insert(QPlatformTheme::TipLabelFont, new QFont(baseFont));
    fonts.insert(QPlatformTheme::LabelFont, new QFont(baseFont));
    fonts.insert(QPlatformTheme::ToolButtonFont, new QFont(baseFont));
    fonts.insert(QPlatformTheme::MenuItemFont, new QFont(menuFont));
    fonts.insert(QPlatformTheme::ComboLineEditFont, new QFont(baseFont));
    fonts.insert(QPlatformTheme::FixedFont, new QFont(monoFont));

    QFont smallFont(baseFont);
    smallFont.setPointSizeF((haikuPlainFont.Size() * kDPI) * 0.75);
    fonts.insert(QPlatformTheme::SmallFont, new QFont(smallFont));
    fonts.insert(QPlatformTheme::MiniFont, new QFont(smallFont));

    return fonts;
}

QT_END_NAMESPACE
