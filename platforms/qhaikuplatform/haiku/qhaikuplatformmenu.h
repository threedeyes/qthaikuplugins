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

#ifndef QHAIKUPLATFORMMENU_H
#define QHAIKUPLATFORMMENU_H

#include <qwindow.h>
#include <qpa/qplatformmenu.h>
#include <qvector.h>
#include <qmutex.h>

#include <Application.h>
#include <Bitmap.h>
#include <Message.h>
#include <Looper.h>
#include <OS.h>
#include <MessageRunner.h>
#include <PopUpMenu.h>
#include <MenuItem.h>

QT_BEGIN_NAMESPACE

class QHaikuPlatformMenuItem;

class QHaikuPlatformMenuLooper : public QObject, public BLooper
{
	Q_OBJECT
public:
	QHaikuPlatformMenuLooper();
	virtual void MessageReceived(BMessage* theMessage);
	thread_id Run(void);
signals:
	void sendHaikuMessage(BMessage *);
};

class QHaikuPlatformMenu: public QPlatformMenu
{
	Q_OBJECT
public:
    typedef QVector<QHaikuPlatformMenuItem *> PlatformMenuItemsType;

public:
    QHaikuPlatformMenu();
    ~QHaikuPlatformMenu();

    void insertMenuItem(QPlatformMenuItem *menuItem, QPlatformMenuItem *before);
    void removeMenuItem(QPlatformMenuItem *menuItem);
    void syncMenuItem(QPlatformMenuItem *menuItem);
    void syncSeparatorsCollapsible(bool enable);

    void setTag(quintptr tag);
    quintptr tag() const;
    void setText(const QString &text);
    QString text() const;
    void setIcon(const QIcon &icon);
    QIcon icon() const;
    void setEnabled(bool enabled);
    bool isEnabled() const;
    void setVisible(bool visible);
    bool isVisible() const;
    void showPopup(const QWindow *parentWindow, const QRect &targetRect, const QPlatformMenuItem *item);

    QPlatformMenuItem *menuItemAt(int position) const;
    QPlatformMenuItem *menuItemForTag(quintptr tag) const;

    PlatformMenuItemsType menuItems() const;
    QMutex *menuItemsMutex();

public slots:
    void haikuMenuEvent(BMessage *msg);
    
private:
	BPopUpMenu* makeBPopUpMenu(QHaikuPlatformMenu *menu);
	
    PlatformMenuItemsType m_menuItems;
    quintptr m_tag;
    QString m_text;
    QIcon m_icon;
    bool m_enabled;
    bool m_isVisible;
    QMutex m_menuItemsMutex;
    
    QHaikuPlatformMenuLooper* menuLooper;
    BPopUpMenu *nativeMenu;
};

QT_END_NAMESPACE

#endif // QHaikuPlatformMenu_H
