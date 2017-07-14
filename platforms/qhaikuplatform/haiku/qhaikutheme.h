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

#ifndef QHAIKUTHEME_H
#define QHAIKUTHEME_H

#include <qpa/qplatformtheme.h>

#include <QtGui/qfont.h>
#include <QtCore/qhash.h>
#include <QtCore/qstring.h>
#include <QtGui/QColor>
#include <QtGui/QPalette>

#include "qhaikuplatformmenu.h"
#include "qhaikuplatformmenuitem.h"

QT_BEGIN_NAMESPACE

class QHaikuIntegration;

class QHaikuTheme : public QPlatformTheme
{
public:
    explicit QHaikuTheme(const QHaikuIntegration *);
    ~QHaikuTheme();

    static QString name() { return QStringLiteral("haiku"); }

    bool usePlatformNativeDialog(DialogType type) const;
    QPlatformDialogHelper *createPlatformDialogHelper(DialogType type) const;
    
    virtual QVariant themeHint(ThemeHint hint) const;

    const QFont *font(Font type = SystemFont) const;

    const QPalette *palette(Palette type = SystemPalette) const;

    virtual QPixmap standardPixmap(StandardPixmap sp, const QSizeF &size) const;
    virtual QIcon fileIcon(const QFileInfo &fileInfo, QPlatformTheme::IconOptions iconOptions = 0) const;

#ifndef QT_NO_SYSTEMTRAYICON
    QPlatformSystemTrayIcon *createPlatformSystemTrayIcon() const;
#endif
    virtual QPlatformMenu *createPlatformMenu() const;
    virtual QPlatformMenuItem *createPlatformMenuItem() const;
private:
    mutable QHash<QPlatformTheme::Font, QFont*> m_fonts;
    const QHaikuIntegration *m_integration;
    QPalette m_defaultPalette;
};

QT_END_NAMESPACE

#endif // QHAIKUTHEME_H
