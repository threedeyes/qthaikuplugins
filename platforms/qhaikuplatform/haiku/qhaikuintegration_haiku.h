/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2015-2019 Gerasim Troeglazov,
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

#ifndef QHAIKUINTEGRATION_APP_H
#define QHAIKUINTEGRATION_APP_H

#include "qhaikuintegration.h"
#include "qhaikusettings.h"

#include "simplecrypt.h"

#include <QApplication>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QClipboard>
#include <QEvent>
#include <QDebug>

#include <private/qguiapplication_p.h>

#include <Application.h>
#include <kernel/OS.h>
#include <Application.h>
#include <File.h>
#include <AppFileInfo.h>
#include <Path.h>
#include <Entry.h>
#include <String.h>
#include <Locale.h>
#include <LocaleRoster.h>
#include <Roster.h>
#include <Clipboard.h>

#include <stdio.h>

class HQApplication : public QObject, public BApplication
{
	Q_OBJECT
public:
	HQApplication(const char*);
	~HQApplication();

	virtual void MessageReceived(BMessage *message);
	void	RefsReceived(BMessage *pmsg);
	virtual bool QuitRequested();
	bool	RefHandled;
	entry_ref Ref;
private:
	BPath 	refReceived;
	BMessenger  fTrackerMessenger;
	QHaikuClipboard *fClipboard;
Q_SIGNALS:
	bool appQuit();
};

#endif
