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

#include "qhaikuplatformmenu.h"
#include "qhaikuplatformmenuitem.h"
#include "qhaikunativeiconmenuitem.h"

#include <qpa/qplatformtheme.h>

QT_BEGIN_NAMESPACE

QHaikuPlatformMenuLooper::QHaikuPlatformMenuLooper() : QObject(), BLooper("QHaikuPlatformMenuLooper")
{	
}

thread_id 
QHaikuPlatformMenuLooper::Run(void)
{
	thread_id Thread = BLooper::Run();	
	return Thread;
}

void 
QHaikuPlatformMenuLooper::MessageReceived(BMessage* theMessage)
{
	BMessage *mes = new BMessage(*theMessage);
	emit sendHaikuMessage(mes);
	BLooper::MessageReceived(theMessage);
}

QHaikuPlatformMenu::QHaikuPlatformMenu()
{
    m_tag = reinterpret_cast<quintptr>(this);
    m_enabled = true;
    m_isVisible = true;

	menuLooper = new QHaikuPlatformMenuLooper();
	menuLooper->Run();			

	connect((QHaikuPlatformMenuLooper*)menuLooper, SIGNAL(sendHaikuMessage(BMessage *)),
		this, SLOT(haikuMenuEvent(BMessage *)), Qt::QueuedConnection);
}

QHaikuPlatformMenu::~QHaikuPlatformMenu()
{
	disconnect((QHaikuPlatformMenuLooper*)menuLooper,SIGNAL(sendHaikuMessage(BMessage *)), 0, 0);

	if(menuLooper->Lock())
		menuLooper->Quit();	
}

void QHaikuPlatformMenu::insertMenuItem(QPlatformMenuItem *menuItem, QPlatformMenuItem *before)
{
//	qDebug() << "QHaikuPlatformMenu::insertMenuItem " << menuItem;
	
    QMutexLocker lock(&m_menuItemsMutex);
    m_menuItems.insert(std::find(m_menuItems.begin(),
                                 m_menuItems.end(),
                                 static_cast<QHaikuPlatformMenuItem *>(before)),
                       static_cast<QHaikuPlatformMenuItem *>(menuItem));
}

void QHaikuPlatformMenu::removeMenuItem(QPlatformMenuItem *menuItem)
{
//	qDebug() << "QHaikuPlatformMenu::removeMenuItem " << menuItem;
	
    QMutexLocker lock(&m_menuItemsMutex);
    PlatformMenuItemsType::iterator it = std::find(m_menuItems.begin(),
                                                   m_menuItems.end(),
                                                   static_cast<QHaikuPlatformMenuItem *>(menuItem));
    if (it != m_menuItems.end())
        m_menuItems.erase(it);
}

void QHaikuPlatformMenu::syncMenuItem(QPlatformMenuItem *menuItem)
{
//	qDebug() << "QHaikuPlatformMenu::syncMenuItem";
	
    PlatformMenuItemsType::iterator it;
    for (it = m_menuItems.begin(); it != m_menuItems.end(); ++it) {
        if ((*it)->tag() == menuItem->tag())
            break;
    }

    if (it != m_menuItems.end()) {
// 	qDebug() << "QHaikuPlatformMenu::syncMenuItem: SINKING";
    }
}

void QHaikuPlatformMenu::syncSeparatorsCollapsible(bool enable)
{
    Q_UNUSED(enable)
}

void QHaikuPlatformMenu::setTag(quintptr tag)
{
    m_tag = tag;
}

quintptr QHaikuPlatformMenu::tag() const
{
    return m_tag;
}

void QHaikuPlatformMenu::setText(const QString &text)
{
    m_text = text;
}

QString QHaikuPlatformMenu::text() const
{	
    return m_text;
}

void QHaikuPlatformMenu::setIcon(const QIcon &icon)
{
    m_icon = icon;
}

QIcon QHaikuPlatformMenu::icon() const
{
    return m_icon;
}

void QHaikuPlatformMenu::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

bool QHaikuPlatformMenu::isEnabled() const
{
    return m_enabled;
}

void QHaikuPlatformMenu::setVisible(bool visible)
{
    m_isVisible = visible;
}

bool QHaikuPlatformMenu::isVisible() const
{
    return m_isVisible;
}

BPopUpMenu* QHaikuPlatformMenu::makeBPopUpMenu(QHaikuPlatformMenu *menu)
{
	BPopUpMenu *haikuMenu = new BPopUpMenu("Menu", false, false);
	PlatformMenuItemsType menuItems = menu->menuItems();

    PlatformMenuItemsType::iterator it;

    bool iconPressent = false;
    for (it = menuItems.begin(); it != menuItems.end(); ++it) {
    	if (!(*it)->icon().isNull()) {
    		iconPressent = true;
    		break;
    	}
    }
    
    for (it = menuItems.begin(); it != menuItems.end(); ++it) {
    	QString text = QPlatformTheme::removeMnemonics((*it)->text()).trimmed();

    	if (!(*it)->isSeparator()) {
    		BBitmap *icon = NULL;
    		
    		if(iconPressent) {
	    		QSize size = (*it)->icon().actualSize(QSize(16, 16));
	    		qDebug() << text << size;
	    		QPixmap pm = (*it)->icon().pixmap(size);
	    		if (!pm.isNull()) {
				    QImage img = pm.toImage();
	    			if(!img.isNull()) {
						icon = new BBitmap(BRect(0, 0, size.width() - 1, size.height() - 1), B_RGBA32);
						icon->SetBits((const void*)img.bits(), img.byteCount(), 0, B_RGBA32);    				
	    			}
	    		}
    		}
			BMessage *msg = new BMessage('STMI');
			int64 val = (*it)->tag();
			msg->AddInt64("tag", val);
			BMenuItem *item = (icon != NULL)||iconPressent ? new IconMenuItem(text.toUtf8().constData(), icon, msg) :
					new BMenuItem(text.toUtf8().constData(), msg);
			item->SetEnabled((*it)->isEnabled());
			item->SetTarget(menuLooper);
			haikuMenu->AddItem(item);
    	} else
    		haikuMenu->AddSeparatorItem();
    }
    return haikuMenu;
}

void QHaikuPlatformMenu::showPopup(const QWindow *parentWindow,
	const QRect &targetRect, const QPlatformMenuItem *item)
{
    Q_UNUSED(item);
    nativeMenu = makeBPopUpMenu(this);
    QPoint pos =  QPoint(targetRect.left(), targetRect.top() + targetRect.height());
	nativeMenu->Go(BPoint(pos.x(), pos.y()), true, true, true);
}

void
QHaikuPlatformMenu::haikuMenuEvent(BMessage *msg)
{
	if (msg->what != 'STMI')
		return;
	
	int64 ptr = 0;
	quintptr tag = 0;
	if (msg->FindInt64("tag", &ptr) == B_OK) {
		tag = ptr;
		QPlatformMenuItem *qitem = menuItemForTag(tag);
		if(qitem)
			qitem->activated();
		delete nativeMenu;
	}
}

QPlatformMenuItem *QHaikuPlatformMenu::menuItemAt(int position) const
{
    if (position < m_menuItems.size())
        return m_menuItems[position];
    return 0;
}

QPlatformMenuItem *QHaikuPlatformMenu::menuItemForTag(quintptr tag) const
{
    foreach (QPlatformMenuItem *menuItem, m_menuItems) {
        if (menuItem->tag() == tag)
            return menuItem;
    }
    return 0;
}

QHaikuPlatformMenu::PlatformMenuItemsType QHaikuPlatformMenu::menuItems() const
{
    return m_menuItems;
}

QMutex *QHaikuPlatformMenu::menuItemsMutex()
{
    return &m_menuItemsMutex;
}

QT_END_NAMESPACE
