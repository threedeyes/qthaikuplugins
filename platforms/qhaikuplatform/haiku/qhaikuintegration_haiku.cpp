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

#include "qhaikuintegration.h"

#include <QtWidgets/qapplication.h>
#include <private/qguiapplication_p.h>
#include <qevent.h>
#include <qdebug.h>

#include <Application.h>
#include <kernel/OS.h>
#include <Application.h>
#include <Path.h>
#include <Entry.h>
#include <String.h>

#include <stdio.h>

class HQApplication : public BApplication
{
public:
	HQApplication(const char*);
	~HQApplication();

	virtual void MessageReceived(BMessage *message);
	void	ArgvReceived(int32 argc, char **argv);
	void	RefsReceived(BMessage *pmsg);
	virtual bool QuitRequested();
	bool	RefHandled;
	entry_ref Ref;
private:
	BPath 	refReceived;
	BMessenger  fTrackerMessenger;
};

namespace {
static HQApplication* haikuApplication = NULL;
}

HQApplication::HQApplication(const char* signature)
		: BApplication(signature)
{
	RefHandled = false;	
	qDebug() << "HQApplication";
}

HQApplication::~HQApplication()
{
}

void HQApplication::MessageReceived(BMessage* msg)
{
	BApplication::MessageReceived(msg);
}

void 
HQApplication::RefsReceived(BMessage *pmsg)
{
	if (pmsg->HasMessenger("TrackerViewToken")) {
		pmsg->FindMessenger("TrackerViewToken", &fTrackerMessenger);
	}

	uint32 type;
	int32 count;
	status_t ret = pmsg->GetInfo("refs", &type, &count);
	if (ret != B_OK || type != B_REF_TYPE)
		return;

	entry_ref ref;
	for (int32 i = 0; i < count; i++) {
   		if (pmsg->FindRef("refs", i, &ref) == B_OK) {   			
   			refReceived.SetTo(&ref);
   			Ref = ref;
   			if (RefHandled) {
   				QCoreApplication::postEvent(qApp, new QFileOpenEvent(refReceived.Path()));
   			}
   			RefHandled = true;
   		}
   	}
}

void
HQApplication::ArgvReceived(int32 argc, char **argv)
{
}

bool HQApplication::QuitRequested()
{
	bool quit = true;

	if (!QGuiApplicationPrivate::instance()->modalWindowList.isEmpty()) {
		int visible = 0;
		const QWindowList windows = QGuiApplication::topLevelWindows();
		for (int i = 0; i < windows.size(); ++i) {
			if (windows.at(i)->isVisible())
				++visible;
		}
		quit = (visible <= 1);
	}

	if (quit) {
		QCloseEvent event;
		QGuiApplication::sendEvent(QCoreApplication::instance(), &event);
		if (event.isAccepted())
			return true;
	}
    return false;
}


int32 AppThread(void *data)
{
	HQApplication *app = (HQApplication*)data;
	app->LockLooper();
	app->Run();	
}


QHaikuIntegration *QHaikuIntegration::createHaikuIntegration(const QStringList& parameters, int &argc, char **argv)
{
	thread_id my_thread;
	QString appSignature = QLatin1String("application/x-vnd.qt5-") + QCoreApplication::applicationName().remove("_x86");
	haikuApplication = new HQApplication(appSignature.toUtf8());
	be_app = haikuApplication;

	my_thread = spawn_thread(AppThread, "BApplication_thread", B_NORMAL_PRIORITY, (void*)haikuApplication);
	resume_thread(my_thread);
		
	haikuApplication->UnlockLooper();
	
	if (argc == 1) {
		for (int i = 0; i < 100; i++) {
			if (haikuApplication->RefHandled) {
				BPath path(&haikuApplication->Ref);
				argc = 2;
				argv[1] = strdup(path.Path());
				argv[2] = 0;
				break;
			}
			snooze(1000);
		}
	}
	
    return new QHaikuIntegration(parameters, argc, argv);
}
