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

#include "QtCore/qcoreapplication.h"
#include "qhaikusystemtrayicon.h"
#include "qdebug.h"

#include <OS.h>
#include <Application.h>
#include <Window.h>
#include <Message.h>
#include <Deskbar.h>
#include <View.h>
#include <String.h>
#include <Roster.h>
#include <Screen.h>
#include <Resources.h>
#include <Bitmap.h>
#include <File.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Looper.h>
#include <Notification.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

QT_BEGIN_NAMESPACE

#define TRAY_MOUSEDOWN 	1
#define TRAY_MOUSEUP	2

#define maxTipLength 	128

#define DBAR_SIGNATURE 	"application/x-vnd.Be-TSKB"


QSystemTrayIconLooper::QSystemTrayIconLooper() : QObject(), BLooper("QSystemTrayIconLooper")
{	
}

thread_id 
QSystemTrayIconLooper::Run(void)
{
	thread_id Thread = BLooper::Run();	
	return Thread;
}

void 
QSystemTrayIconLooper::MessageReceived(BMessage* theMessage)
{
	if( theMessage->what == 'TRAY' || 
		theMessage->what == 'PULS' || 
		theMessage->what == 'LIVE') {
		BMessage *mes = new BMessage(*theMessage);
		emit sendHaikuMessage(mes);
	}
	BLooper::MessageReceived(theMessage);
} 

QHaikuSystemTrayIcon::QHaikuSystemTrayIcon()
	: looper(0), pulse(0), ReplicantId(-1), qystrayExist(false)
{
	qystrayExist = FindTrayExecutable();
}

void
QHaikuSystemTrayIcon::init()
{
	looper = new QSystemTrayIconLooper();
	looper->Run();		
	
	connect((QSystemTrayIconLooper*)looper, SIGNAL(sendHaikuMessage(BMessage *)),
		this, SLOT(HaikuEvent(BMessage *)), Qt::QueuedConnection);

	pulse = new BMessageRunner(BMessenger(looper), new BMessage('PULS'), 1000000);

	InstallIcon();
}

void
QHaikuSystemTrayIcon::cleanup()
{
	if (pulse != NULL) {
		delete pulse;
		pulse = NULL;
	}

	if (looper != NULL) {
		if (looper->Lock()) {
			disconnect((QSystemTrayIconLooper*)looper, SIGNAL(sendHaikuMessage(BMessage *)), 0, 0);
			looper->Quit();
			looper = NULL;
		}
	}

	BDeskbar deskbar;
	if (ReplicantId > 0)
		deskbar.RemoveItem(ReplicantId);
}

void
QHaikuSystemTrayIcon::updateIcon(const QIcon &qicon)
{
    if (qicon.isNull())
        return;
        
    currentIcon = qicon;

    QSize size = qicon.actualSize(QSize(16, 16));
    QPixmap pm = qicon.pixmap(size);
    if (pm.isNull())
        return;
    QImage img = pm.toImage();
    if(img.isNull())
    	return;
    	
	BBitmap *icon = new BBitmap(BRect(0, 0, size.width()-1, size.height()-1), B_RGBA32);
	icon->SetBits((const void*)img.bits(), img.byteCount(), 0, B_RGBA32); 
	if(icon) {
		BMessage	bits(B_ARCHIVED_OBJECT);
		icon->Archive(&bits);	
		BMessage *mes = new BMessage('BITS');
		mes->AddMessage("icon",&bits);
		bits.MakeEmpty();
		SendMessageToReplicant(mes);
		delete icon;
	}	
	updateToolTip(currentToolTip);
}

void
QHaikuSystemTrayIcon::updateToolTip(const QString &tip)
{
	currentToolTip = tip;

    BString tipStr("");
    
    if (!currentToolTip.isNull()) {
    	currentToolTip.remove(QRegExp("<[^>]*>"));
    	currentToolTip.replace("&nbsp;", " ");
    	const char *str = (const char *)(currentToolTip.toUtf8());
    	tipStr.SetTo(str);
    }
	
	BMessage *mes = new BMessage('TTIP');		
	mes->AddString("tooltip",tipStr.String());	
	SendMessageToReplicant(mes);	
}

void
QHaikuSystemTrayIcon::updateMenu(QPlatformMenu *menu)
{
	currentMenu = menu;
}

#if 0
QPlatformMenu *
QHaikuSystemTrayIcon::createMenu() const
{
	qDebug() << "QHaikuSystemTrayIcon::createMenu()";
	return NULL;
}
#endif

QRect
QHaikuSystemTrayIcon::geometry() const
{
	return shelfRect;
}


void
QHaikuSystemTrayIcon::showMessage(const QString &title, const QString &msg,
	const QIcon& icon, MessageIcon iconType, int secs)
{
	QFileInfo appFileInfo(QCoreApplication::applicationFilePath());
	BString stitle((const char *)(title.toUtf8()));
	BString smessage((const char *)(msg.toUtf8()));
	BString smessageId((const char *)(appFileInfo.fileName().toUtf8()));
	BString group((const char*)(appFileInfo.baseName().toUtf8()));

	BFile file(appFileInfo.filePath().toUtf8(), O_RDONLY);
	BNodeInfo nodeInfo(&file);
	BRect rect(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON -1);
	BBitmap bitmap(rect, B_RGBA32);

	notification_type ntype = (iconType == QSystemTrayIcon::Warning) ? B_IMPORTANT_NOTIFICATION:
		(iconType==QSystemTrayIcon::Critical) ? B_ERROR_NOTIFICATION : B_INFORMATION_NOTIFICATION;
	BNotification notification(ntype);
	notification.SetGroup(group);
	notification.SetTitle(stitle);
	notification.SetMessageID(smessageId);
	notification.SetContent(smessage);
	
	if (icon.isNull()) {
		if (nodeInfo.GetTrackerIcon(&bitmap, B_LARGE_ICON) == B_NO_ERROR)
			notification.SetIcon(&bitmap);
	} else {
		QPixmap pixmap = icon.pixmap(B_LARGE_ICON);
		QImage image = pixmap.toImage();
		memcpy(bitmap.Bits(), image.bits(), image.byteCount());
		notification.SetIcon(&bitmap);
	}

	notification.Send(secs * 1000);	
}

bool
QHaikuSystemTrayIcon::isSystemTrayAvailable() const
{
	return true;
}

bool
QHaikuSystemTrayIcon::supportsMessages() const
{
	return true;
}

void
QHaikuSystemTrayIcon::HaikuEvent(BMessage *m)
{	
	if(m->what == 'PULS') {
		LiveFactor--;
		if(LiveFactor < -5) {		//Reinstallation time
			LiveFactor = 0;
			ReplicantId = 0;
			InstallIcon();
			LiveFactor = 0;
			updateIcon(currentIcon);
			updateToolTip(currentToolTip);
		}
	}
	if(m->what == 'LIVE') {
		LiveFactor++;		
		BRect rect;
		if(m->FindRect("rect",&rect)==B_OK) {
			shelfRect.setRect(rect.left, rect.top, rect.Width(), rect.Height());
		}		
	}
	if(m->what == 'TRAY') {
		int32 event = 0;
		BPoint point(0,0);
		int32 buttons = 0,
			  clicks = 0;
	
		m->FindInt32("event",&event);
		m->FindPoint("point",&point);
		m->FindInt32("buttons",&buttons);
		m->FindInt32("clicks",&clicks);
		
		switch(event) {
			case TRAY_MOUSEUP:
				{				                
					if (buttons == B_PRIMARY_MOUSE_BUTTON) {
					if (ignoreNextMouseRelease)
	                    ignoreNextMouseRelease = false;
	                else
	                    emit activated(QPlatformSystemTrayIcon::Trigger);
						break;
					}
					if (buttons == B_TERTIARY_MOUSE_BUTTON) {
						emit activated(QPlatformSystemTrayIcon::MiddleClick);
						break;
					}
					if (buttons == B_SECONDARY_MOUSE_BUTTON) {
						QPoint gpos = QPoint(point.x,point.y);
		                if (currentMenu)
		                	currentMenu->showPopup(NULL, shelfRect, NULL);
		                emit activated(QPlatformSystemTrayIcon::Context);		
		             	break;   			
					}
				}
				break;
			case TRAY_MOUSEDOWN:
				{				
					if (buttons == B_PRIMARY_MOUSE_BUTTON && clicks == 2) {
						ignoreNextMouseRelease = true;
						emit activated(QPlatformSystemTrayIcon::DoubleClick);
						break;
					}
				}
				break;
			default:
				break;
		}
	}
}

bool
QHaikuSystemTrayIcon::FindTrayExecutable(void)
{
	sysTrayExecutable.setFile("/bin/qsystray");
	if (sysTrayExecutable.exists() && sysTrayExecutable.isFile())
		return true;
	return false;
}

status_t 
QHaikuSystemTrayIcon::SendMessageToReplicant(BMessage *msg)
{
	if(ReplicantId<=0)
		return B_ERROR;
		
	BMessage aReply;
	status_t aErr = B_OK;
	
	msg->AddInt32( "what2", msg->what );
	msg->what = B_SET_PROPERTY;

	BMessage	uid_specifier(B_ID_SPECIFIER);
	
	msg->AddSpecifier("View");
	uid_specifier.AddInt32("id", ReplicantId);
	uid_specifier.AddString("property", "Replicant");
	msg->AddSpecifier(&uid_specifier);
		
	aErr = GetShelfMessenger().SendMessage( msg, (BHandler*)NULL, 500000 );
	return aErr;
}

void
QHaikuSystemTrayIcon::InstallIcon(void)
{
	ReplicantId = DeskBarLoadIcon();

	QString appName = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
	BString app_name((const char *)(appName.toUtf8()));

	BMessage mes('MSGR');
	QHaikuSystemTrayIcon *sys = this;
	mes.AddMessenger("messenger", BMessenger(NULL, looper));
	mes.AddData("qtrayobject", B_ANY_TYPE, &sys, sizeof(void*));
	mes.AddString( "application_name", app_name);

	SendMessageToReplicant(&mes);
}

BMessenger 
QHaikuSystemTrayIcon::GetShelfMessenger(void)
{
	BMessenger aResult;
	status_t aErr = B_OK;
	BMessenger aDeskbar(DBAR_SIGNATURE, -1, &aErr);
	if (aErr != B_OK)return aResult;

	BMessage aMessage(B_GET_PROPERTY);
	
	aMessage.AddSpecifier("Messenger");
	aMessage.AddSpecifier("Shelf");
	aMessage.AddSpecifier("View", "Status");
	aMessage.AddSpecifier("Window", "Deskbar");
	
	BMessage aReply;

	if (aDeskbar.SendMessage(&aMessage, &aReply, 500000, 500000) == B_OK)
		aReply.FindMessenger("result", &aResult);
	return aResult;
}

int32	
QHaikuSystemTrayIcon::ExecuteCommand(const char *command)
{
   FILE *fpipe;
   char line[256];
   if ( !(fpipe = (FILE*)popen(command,"r")) )
   		return -1;

   fgets( line, sizeof line, fpipe);
   pclose(fpipe);
   
   int res = atoi(line);
   return res;
}


int32 
QHaikuSystemTrayIcon::DeskBarLoadIcon(team_id tid)
{
	BString cmd((const char *)(sysTrayExecutable.absoluteFilePath().toUtf8()));
	cmd << " " << (int)tid;
	int32 id = ExecuteCommand(cmd.String());
	return id;
}

int32 
QHaikuSystemTrayIcon::DeskBarLoadIcon(void)
{
	thread_info threadInfo;
	status_t error = get_thread_info(find_thread(NULL), &threadInfo);
	if (error != B_OK) {
		fprintf(stderr, "Failed to get info for the current thread: %s\n", strerror(error));
		return -1;	
	}
	team_id sTeam = threadInfo.team;
	
	return DeskBarLoadIcon(sTeam);
}

QT_END_NAMESPACE
