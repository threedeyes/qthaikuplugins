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

#include "qhaikuintegration.h"
#include "qhaikuapplication.h"
#include "qhaikusettings.h"


HQApplication::HQApplication(const char* signature)
	: QObject()
	, BApplication(signature)
	, fClipboard(NULL)
	, qtFlags(0)
{
	readyForRunSem = create_sem(0, "readyForRunSem");
}

HQApplication::~HQApplication()
{
}

void HQApplication::waitForRun(void)
{
	if (readyForRunSem != B_BAD_SEM_ID) {
		acquire_sem(readyForRunSem);
		delete_sem(readyForRunSem);
		readyForRunSem = B_BAD_SEM_ID;
	}
}

void HQApplication::ReadyToRun()
{
	if (readyForRunSem != B_BAD_SEM_ID)
		release_sem(readyForRunSem);
}

void HQApplication::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_SILENT_RELAUNCH:
			be_roster->ActivateApp(be_app->Team());
			break;
		case 'CLIP':
			{
			if (message->FindPointer("QHaikuClipboard", (void**)(&fClipboard)) != B_OK)
				fClipboard = NULL;
			break;
			}
		case B_CLIPBOARD_CHANGED:
			{
			if (fClipboard != NULL)
				fClipboard->clipboardChanged();
			break;
			}
		default:
			BApplication::MessageReceived(message);
			break;
	}
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
	BPath refReceived;
	QStringList refList;
	for (int32 i = 0; i < count; i++) {
		if (pmsg->FindRef("refs", i, &ref) == B_OK) {
			refReceived.SetTo(&ref);
			refList << refReceived.Path();
		}
	}

	if (IsLaunching()) {
		openFileList = refList;
		return;
	} else {
		if (qtFlags & Q_REF_TO_FORK) {
			QProcess proc;
			proc.start("/bin/sh", refList);
			return;
		}
	}

	for ( const auto& refValue : refList )
		QCoreApplication::postEvent(QCoreApplication::instance(), new QFileOpenEvent(refValue));
}

bool HQApplication::QuitRequested()
{
	return Q_EMIT applicationQuit();
}
