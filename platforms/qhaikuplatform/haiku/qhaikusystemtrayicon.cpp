/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2015-2020 Gerasim Troeglazov,
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

#include <QApplication>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QWindow>
#include "QDebug"
#include <QSettings>

#include "qhaikusystemtrayicon.h"
#include "qhaikusettings.h"
#include "qhaikuwindow.h"

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
#include <BeBuild.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

QT_BEGIN_NAMESPACE

#define TRAY_MOUSEDOWN 	1
#define TRAY_MOUSEUP	2

#define DBAR_SIGNATURE 	"application/x-vnd.Be-TSKB"


QSystemTrayIconLooper::QSystemTrayIconLooper() : QObject(), BLooper("QSystemTrayIconLooper")
{	
}

thread_id 
QSystemTrayIconLooper::Run(void)
{
	lastButtons = 0;
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
	if (theMessage->what == 'MOUS') {
		BPoint point;
		uint32 buttons;
		get_mouse(&point, &buttons);
		if (lastButtons != buttons && buttons == 0) {
			BMessage *mes = new BMessage(*theMessage);
			emit sendHaikuMessage(mes);
		}
		lastButtons = buttons;
	}
	BLooper::MessageReceived(theMessage);
} 

QHaikuSystemTrayIcon::QHaikuSystemTrayIcon()
	: looper(NULL)
	, replicantId(-1)
	, qystrayExist(false)
	, pulse(NULL)
	, trayMenuClickCounter(0)
{
	qystrayExist = findTrayExecutable();
}

void
QHaikuSystemTrayIcon::init()
{
	looper = new QSystemTrayIconLooper();
	looper->Run();

	connect((QSystemTrayIconLooper*)looper, SIGNAL(sendHaikuMessage(BMessage *)),
		this, SLOT(haikuEvents(BMessage *)), Qt::QueuedConnection);

	pulse = new BMessageRunner(BMessenger(looper), new BMessage('PULS'), 1000000);
	pulse = new BMessageRunner(BMessenger(looper), new BMessage('MOUS'), 50000);
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
	if (replicantId > 0) {
		deskbar.RemoveItem(replicantId);
		replicantId = -1;
	}
	trayMenuClickCounter = 0;
}

void
QHaikuSystemTrayIcon::updateIcon(const QIcon &qicon)
{
    if (qicon.isNull())
        return;

    currentIcon = qicon;

	BDeskbar deskbar;
	QSize trayIconSize = QSize(deskbar.MaxItemHeight(), deskbar.MaxItemHeight());
    QSize size = qicon.actualSize(trayIconSize);
    QPixmap pixmap = qicon.pixmap(size);
    QPixmap scaledPixmap = pixmap.scaled(trayIconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (scaledPixmap.isNull())
        return;
    QImage image = scaledPixmap.toImage();
	if ( image.isNull() )
		return;
    	
	BBitmap *icon = new BBitmap(BRect(0, 0, trayIconSize.width() - 1, trayIconSize.height() - 1), B_RGBA32);
	if ( icon ) {
		icon->SetBits((const void*)image.bits(), image.sizeInBytes(), 0, B_RGBA32);

		installIcon();

		BMessage bits(B_ARCHIVED_OBJECT);
		icon->Archive(&bits);
		BMessage *message = new BMessage('BITS');
		message->AddMessage("icon", &bits);
		bits.MakeEmpty();

		sendMessageToReplicant(message);

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
		tipStr.SetTo(currentToolTip.toUtf8().constData(), currentToolTip.toUtf8().count());
    }

	BMessage *message = new BMessage('TTIP');
	message->AddString("tooltip",tipStr.String());
	sendMessageToReplicant(message);
}

QPlatformMenu *
QHaikuSystemTrayIcon::createMenu() const
{
	return nullptr;
}

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
	BString stitle(title.toUtf8().constData());
	BString smessage(msg.toUtf8().constData());
	BString smessageId(appFileInfo.fileName().toUtf8().constData());
	BString group(appFileInfo.baseName().toUtf8().constData());

	BFile file(appFileInfo.filePath().toUtf8().constData(), O_RDONLY);
	BNodeInfo nodeInfo(&file);
	BRect rect(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON -1);
	BBitmap bitmap(rect, B_RGBA32);

	notification_type ntype = (iconType == QPlatformSystemTrayIcon::Warning) ? B_IMPORTANT_NOTIFICATION:
		(iconType==QPlatformSystemTrayIcon::Critical) ? B_ERROR_NOTIFICATION : B_INFORMATION_NOTIFICATION;
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
		QImage image = pixmap.toImage().scaled(B_LARGE_ICON, B_LARGE_ICON);
		memcpy(bitmap.Bits(), image.bits(), image.sizeInBytes());
		notification.SetIcon(&bitmap);
	}

	notification.Send(secs * 1000);
}

void
QHaikuSystemTrayIcon::haikuEvents(BMessage *message)
{
	if(message->what == 'PULS') {
		liveFactor--;
		if(liveFactor < -5) {		//Reinstallation time
			liveFactor = 0;
			replicantId = -1;
			installIcon();
			liveFactor = 0;
			updateIcon(currentIcon);
			updateToolTip(currentToolTip);
		}
	}
	if(message->what == 'MOUS' && trayMenuClickCounter > 0) {
		if (trayMenuClickCounter == 1) {
			while (QApplication::activePopupWidget())
				QApplication::activePopupWidget()->close();
		}
		trayMenuClickCounter--;
	}
	if(message->what == 'LIVE') {
		liveFactor++;
		BRect rect;
		if(message->FindRect("rect", &rect)==B_OK)
			shelfRect.setRect(rect.left, rect.top, rect.Width(), rect.Height());
	}
	if(message->what == 'TRAY') {
		int32 event = 0;
		BPoint point(0, 0);
		int32 buttons = 0,
			  clicks = 0;

		message->FindInt32("event", &event);
		message->FindPoint("point", &point);
		message->FindInt32("buttons", &buttons);
		message->FindInt32("clicks", &clicks);

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
						const QPoint globalPos = QPoint(point.x, point.y);
						emit contextMenuRequested(globalPos, nullptr);
		                emit activated(QPlatformSystemTrayIcon::Context);
		                trayMenuClickCounter = 2;
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
QHaikuSystemTrayIcon::findTrayExecutable(void)
{
	sysTrayExecutable.setFile("/bin/qsystray");
	if (sysTrayExecutable.exists() && sysTrayExecutable.isFile())
		return true;
	return false;
}

status_t 
QHaikuSystemTrayIcon::sendMessageToReplicant(BMessage *message)
{
	if(replicantId <= 0)
		return B_ERROR;

	BMessage aReply;
	status_t aErr = B_OK;

	message->AddInt32("what2", message->what);
	message->what = B_SET_PROPERTY;

	BMessage uid_specifier(B_ID_SPECIFIER);

	message->AddSpecifier("View");
	uid_specifier.AddInt32("id", replicantId);
	uid_specifier.AddString("property", "Replicant");
	message->AddSpecifier(&uid_specifier);

	aErr = getShelfMessenger().SendMessage(message, (BHandler*)NULL, 500000);
	return aErr;
}

void
QHaikuSystemTrayIcon::installIcon(void)
{
	if (replicantId <= 0)
		replicantId = deskBarLoadIcon();

	QString appName = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
	BString app_name(appName.toUtf8().constData());

	BMessage message('MSGR');
	QHaikuSystemTrayIcon *sysTray = this;
	message.AddMessenger("messenger", BMessenger(NULL, looper));
	message.AddData("qtrayobject", B_ANY_TYPE, &sysTray, sizeof(void*));
	message.AddString("application_name", app_name);

	sendMessageToReplicant(&message);

	QHaikuWindow::syncDeskBarVisible();

	liveFactor = 0;
}

BMessenger 
QHaikuSystemTrayIcon::getShelfMessenger(void)
{
	BMessenger aResult;
	status_t aErr = B_OK;
	BMessenger aDeskbar(DBAR_SIGNATURE, -1, &aErr);
	if (aErr != B_OK)
		return aResult;

	BMessage message(B_GET_PROPERTY);

	message.AddSpecifier("Messenger");
	message.AddSpecifier("Shelf");
	message.AddSpecifier("View", "Status");
	message.AddSpecifier("Window", "Deskbar");

	BMessage aReply;

	if (aDeskbar.SendMessage(&message, &aReply, 500000, 500000) == B_OK)
		aReply.FindMessenger("result", &aResult);
	return aResult;
}

int32	
QHaikuSystemTrayIcon::executeCommand(const char *command)
{
   FILE *fpipe;
   char line[256];
   if ( !(fpipe = (FILE*)popen(command, "r")) )
   		return -1;

   fgets( line, sizeof line, fpipe);
   pclose(fpipe);
   
   int res = atoi(line);
   return res;
}


int32 
QHaikuSystemTrayIcon::deskBarLoadIcon(team_id tid)
{
	BString cmd(sysTrayExecutable.absoluteFilePath().toUtf8().constData());
	cmd << " " << (int)tid;
	int32 id = executeCommand(cmd.String());
	return id;
}

int32 
QHaikuSystemTrayIcon::deskBarLoadIcon(void)
{
	thread_info threadInfo;
	status_t error = get_thread_info(find_thread(NULL), &threadInfo);
	if (error != B_OK) {
		fprintf(stderr, "Failed to get info for the current thread: %s\n", strerror(error));
		return -1;	
	}
	team_id sTeam = threadInfo.team;
	
	return deskBarLoadIcon(sTeam);
}

QT_END_NAMESPACE
