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

#ifndef QHAIKUSYSTEMTRAYICON_H
#define QHAIKUSYSTEMTRAYICON_H

#include <QtCore/qglobal.h>

#ifndef QT_NO_SYSTEMTRAYICON

#include "QString"
#include "QSystemTrayIcon"
#include "QMenu"

#include "QtGui/qpa/qplatformsystemtrayicon.h"
#include "QtGui/qpa/qplatformmenu.h"

#include "qfileinfo.h"

#include <Application.h>
#include <Bitmap.h>
#include <Message.h>
#include <Looper.h>
#include <OS.h>
#include <MessageRunner.h>

QT_BEGIN_NAMESPACE

class QSystemTrayIconLooper : public QObject, public BLooper
{
	Q_OBJECT
public:
	QSystemTrayIconLooper();
	virtual void MessageReceived(BMessage* theMessage);
	thread_id Run(void);
signals:
	void sendHaikuMessage(BMessage *);
private:
	uint32 lastButtons;
};

class QHaikuSystemTrayIcon : public QPlatformSystemTrayIcon
{
	Q_OBJECT
public:
    QHaikuSystemTrayIcon();

    void init() override;
    void cleanup() override;
    void updateIcon(const QIcon &icon) override;
    void updateToolTip(const QString &tooltip) override;
	void updateMenu(QPlatformMenu *) override {}
    QPlatformMenu *createMenu() const override;
    QRect geometry() const override;
    void showMessage(const QString &title, const QString &msg,
                     const QIcon &icon, MessageIcon iconType, int msecs) override;

    bool isSystemTrayAvailable() const override { return true; }
    bool supportsMessages() const override { return true; }

    bool 	findTrayExecutable(void);
    status_t sendMessageToReplicant(BMessage *msg);

	QSystemTrayIconLooper* looper;

	QRect	shelfRect;

	QFileInfo sysTrayExecutable;

	QIcon 	currentIcon;
	QString currentToolTip;

public slots:
    void	haikuEvents(BMessage *m);

private:
	BMessenger getShelfMessenger(void);
	int32	executeCommand(const char *command);
	int32 	deskBarLoadIcon(team_id tid);
	int32 	deskBarLoadIcon(void);
	void 	installIcon(void);

	int32	replicantId;
	int32	liveFactor;
	
	bool 	ignoreNextMouseRelease;
	bool	qystrayExist;
	int32	trayMenuClickCounter;

	BMessageRunner *pulse;
};

QT_END_NAMESPACE

#endif // QT_NO_SYSTEMTRAYICON

#endif // QHAIKUSYSTEMTRAYICON_H
