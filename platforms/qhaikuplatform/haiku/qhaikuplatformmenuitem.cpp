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

#include "qhaikuplatformmenuitem.h"
#include "qhaikuplatformmenu.h"

QT_BEGIN_NAMESPACE

QHaikuPlatformMenuItem::QHaikuPlatformMenuItem()
{
    m_tag = reinterpret_cast<quintptr>(this);
    m_menu = 0;
    m_isVisible = true;
    m_isSeparator = false;
    m_role = NoRole;
    m_isCheckable = false;
    m_isChecked = false;
    m_isEnabled = true;
}

void QHaikuPlatformMenuItem::setTag(quintptr tag)
{
    m_tag = tag;
}

quintptr QHaikuPlatformMenuItem::tag() const
{
    return m_tag;
}

void QHaikuPlatformMenuItem::setText(const QString &text)
{
    m_text = text;
    if (m_menu)
        m_menu->setText(m_text);
}

QString QHaikuPlatformMenuItem::text() const
{
    return m_text;
}

void QHaikuPlatformMenuItem::setIcon(const QIcon &icon)
{
    m_icon = icon;
    if (m_menu)
        m_menu->setIcon(m_icon);
}

QIcon QHaikuPlatformMenuItem::icon() const
{
    return m_icon;
}

void QHaikuPlatformMenuItem::setMenu(QPlatformMenu *menu)
{
    m_menu = static_cast<QHaikuPlatformMenu *>(menu);
    if (!m_menu)
        return;

    m_menu->setText(m_text);
    m_menu->setIcon(m_icon);
    m_menu->setVisible(m_isVisible);
    m_menu->setEnabled(m_isEnabled);
}

QHaikuPlatformMenu *QHaikuPlatformMenuItem::menu() const
{
    return m_menu;
}

void QHaikuPlatformMenuItem::setVisible(bool isVisible)
{
    m_isVisible = isVisible;
    if (m_menu)
        m_menu->setVisible(m_isVisible);
}

bool QHaikuPlatformMenuItem::isVisible() const
{
    return m_isVisible;
}

void QHaikuPlatformMenuItem::setIsSeparator(bool isSeparator)
{
    m_isSeparator = isSeparator;
}

bool QHaikuPlatformMenuItem::isSeparator() const
{
    return m_isSeparator;
}

void QHaikuPlatformMenuItem::setFont(const QFont &font)
{
    Q_UNUSED(font)
}

void QHaikuPlatformMenuItem::setRole(QPlatformMenuItem::MenuRole role)
{
    m_role = role;
}

QPlatformMenuItem::MenuRole QHaikuPlatformMenuItem::role() const
{
    return m_role;
}

void QHaikuPlatformMenuItem::setCheckable(bool checkable)
{
    m_isCheckable = checkable;
}

bool QHaikuPlatformMenuItem::isCheckable() const
{
    return m_isCheckable;
}

void QHaikuPlatformMenuItem::setChecked(bool isChecked)
{
    m_isChecked = isChecked;
}

bool QHaikuPlatformMenuItem::isChecked() const
{
    return m_isChecked;
}

void QHaikuPlatformMenuItem::setShortcut(const QKeySequence &shortcut)
{
    Q_UNUSED(shortcut)
}

void QHaikuPlatformMenuItem::setEnabled(bool enabled)
{
    m_isEnabled = enabled;
    if (m_menu)
        m_menu->setEnabled(m_isEnabled);
}

bool QHaikuPlatformMenuItem::isEnabled() const
{
    return m_isEnabled;
}

void QHaikuPlatformMenuItem::setIconSize(int size)
{
    Q_UNUSED(size)
}

QT_END_NAMESPACE
