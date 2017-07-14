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

#ifndef QHAIKUPLATFORMMENUITEM_H
#define QHAIKUPLATFORMMENUITEM_H

#include <qpa/qplatformmenu.h>
#include "qdebug.h"

QT_BEGIN_NAMESPACE

class QHaikuPlatformMenu;

class QHaikuPlatformMenuItem: public QPlatformMenuItem
{
public:
    QHaikuPlatformMenuItem();
    void setTag(quintptr tag);
    quintptr tag() const;

    void setText(const QString &text);
    QString text() const;

    void setIcon(const QIcon &icon);
    QIcon icon() const;

    void setMenu(QPlatformMenu *menu);
    QHaikuPlatformMenu *menu() const;

    void setVisible(bool isVisible);
    bool isVisible() const;

    void setIsSeparator(bool isSeparator);
    bool isSeparator() const;

    void setFont(const QFont &font);

    void setRole(MenuRole role);
    MenuRole role() const;

    void setCheckable(bool checkable);
    bool isCheckable() const;

    void setChecked(bool isChecked);
    bool isChecked() const;

    void setShortcut(const QKeySequence &shortcut);

    void setEnabled(bool enabled);
    bool isEnabled() const;

    void setIconSize(int size);

private:
    quintptr m_tag;
    QString m_text;
    QIcon m_icon;
    QHaikuPlatformMenu *m_menu;
    bool m_isVisible;
    bool m_isSeparator;
    MenuRole m_role;
    bool m_isCheckable;
    bool m_isChecked;
    bool m_isEnabled;
};

QT_END_NAMESPACE

#endif // QHAIKUPLATFORMMENUITEM_H
