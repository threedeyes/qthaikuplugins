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

#include "qhaikuintegration.h"
#include "qhaikuintegration_haiku.h"
#include "qhaikusettings.h"

namespace {
static HQApplication* haikuApplication = NULL;
static bool haikuApplicationQuitAccepted = false;
static bool haikuApplicationQuitChecked = false;
}

HQApplication::HQApplication(const char* signature)
		: QObject()
		, BApplication(signature)
{
	RefHandled = false;
	fClipboard = NULL;
}

HQApplication::~HQApplication()
{
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

bool HQApplication::QuitRequested()
{
	haikuApplicationQuitAccepted = false;
	haikuApplicationQuitChecked = false;

	Q_EMIT appQuit();

	while(!haikuApplicationQuitChecked)
		snooze(100);

	return haikuApplicationQuitAccepted;
}

static int32 haikuAppThread(void *data)
{
	HQApplication *app = static_cast<HQApplication*>(data);
	app->LockLooper();
	app->Run();
	return B_OK;
}

void QHaikuIntegration::platformAppQuit()
{
	QCloseEvent event;
	QGuiApplication::sendEvent(QCoreApplication::instance(), &event);
	haikuApplicationQuitAccepted = event.isAccepted();
	haikuApplicationQuitChecked = true;
}

QHaikuIntegration *QHaikuIntegration::createHaikuIntegration(const QStringList& parameters, int &argc, char **argv)
{
	SimpleCrypt crypt(Q_UINT64_C(0x3de48151623423de));
	QSettings settings(QT_SETTINGS_FILENAME, QSettings::NativeFormat);
	settings.beginGroup("QPA");

	QString appSignature;

	char signature[B_MIME_TYPE_LENGTH];
	signature[0] = '\0';

	QString appPath = QCoreApplication::applicationFilePath();

	BFile appFile(appPath.toUtf8().constData(), B_READ_ONLY);
	if (appFile.InitCheck() == B_OK) {
		BAppFileInfo info(&appFile);
		if (info.InitCheck() == B_OK) {
			if (info.GetSignature(signature) != B_OK)
				signature[0] = '\0';
		}
	}

	if (signature[0] != '\0')
		appSignature = QLatin1String(signature);
	else
		appSignature = QLatin1String("application/x-vnd.qt5-") +
			QCoreApplication::applicationName().remove("_x86");

	thread_id my_thread;	

	if (be_app == NULL) {
		haikuApplication = new HQApplication(appSignature.toUtf8().constData());
		be_app = haikuApplication;

		my_thread = spawn_thread(haikuAppThread, "BApplication_thread", B_NORMAL_PRIORITY, (void*)haikuApplication);
		resume_thread(my_thread);

		if (settings.value("hide_from_deskbar", true).toBool()) {
			BMessage message;
			message.what = 'BRAQ';
			message.AddInt32("be:team", ::getpid());
			BMessenger("application/x-vnd.Be-TSKB").SendMessage(&message);
		}

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
	}

	// Dirty hack for environment initialisation
	QProcess proc;
	QStringList env = QProcess::systemEnvironment();

	if (env.count() == 0) {
		proc.start("/bin/sh", QStringList() << "-c" <<"source /boot/system/boot/SetupEnvironment; env ");
		proc.waitForFinished();
		QString resultSystemEnv(proc.readAllStandardOutput());
		env += resultSystemEnv.split("\n");

		proc.start("/bin/sh", QStringList() << "-c" <<"source /boot/home/config/settings/boot/UserSetupEnvironment; env ");
		proc.waitForFinished();
		QString resultUserEnv(proc.readAllStandardOutput());
		env += resultUserEnv.split("\n");

		foreach (const QString &line, env) {
			putenv(line.toUtf8().constData());
		}
	}
	// Enable software rendering for QML
	if (settings.value("qml_softwarecontext", true).toBool())
		putenv("QMLSCENE_DEVICE=softwarecontext");
	settings.endGroup();

	// Proxy settings
	settings.beginGroup("Network");
	if (settings.value("use_proxy", false).toBool()) {
		QString emptyString = crypt.encryptToString(QString(""));
		if (settings.value("http_proxy_enable", false).toBool()) {
			QString username = settings.value("http_proxy_username", QString("")).toString();
			QString password = crypt.decryptToString(settings.value("http_proxy_password", emptyString).toString());
			QString scheme = settings.value("http_proxy_scheme", QString("http://")).toString();
			QString http_proxy("http_proxy=");
			http_proxy += scheme;
			if (!username.isEmpty() && !password.isEmpty())
				http_proxy += username + ":" + password + "@";
			http_proxy += settings.value("http_proxy_address", QString("")).toString() + ":";
			http_proxy += QString::number(settings.value("http_proxy_port", 8080).toInt()) + "/";
			putenv(http_proxy.toUtf8().constData());
		}
		if (settings.value("https_proxy_enable", false).toBool()) {
			QString username = settings.value("https_proxy_username", QString("")).toString();
			QString password = crypt.decryptToString(settings.value("https_proxy_password", emptyString).toString());
			QString scheme = settings.value("https_proxy_scheme", QString("http://")).toString();
			QString https_proxy("https_proxy=");
			https_proxy += scheme;
			if (!username.isEmpty() && !password.isEmpty())
				https_proxy += username + ":" + password + "@";
			https_proxy += settings.value("https_proxy_address", QString("")).toString() + ":";
			https_proxy += QString::number(settings.value("https_proxy_port", 8080).toInt()) + "/";
			putenv(https_proxy.toUtf8().constData());
		}
		if (settings.value("ftp_proxy_enable", false).toBool()) {
			QString username = settings.value("ftp_proxy_username", QString("")).toString();
			QString password = crypt.decryptToString(settings.value("ftp_proxy_password", emptyString).toString());
			QString scheme = settings.value("ftp_proxy_scheme", QString("http://")).toString();
			QString ftp_proxy("ftp_proxy=");
			ftp_proxy += scheme;
			if (!username.isEmpty() && !password.isEmpty())
				ftp_proxy += username + ":" + password + "@";
			ftp_proxy += settings.value("ftp_proxy_address", QString("")).toString() + ":";
			ftp_proxy += QString::number(settings.value("ftp_proxy_port", 8080).toInt()) + "/";
			putenv(ftp_proxy.toUtf8().constData());
		}
		QString no_proxy = settings.value("no_proxy_list", QString("")).toString();
		if (!no_proxy.isEmpty()) {
			no_proxy = "no_proxy=\"" + no_proxy + "\"";
			putenv(no_proxy.toUtf8().constData());
		}
	}
	settings.endGroup();
	
	// Set XDG variables
	putenv("XDG_CONFIG_HOME=/boot/home/config/settings");
	putenv("XDG_CONFIG_DIRS=/boot/system/settings");
	putenv("XDG_CACHE_HOME=/boot/home/config/cache");
	putenv("XDG_DATA_HOME=/boot/home/config/non-packaged/data");
	putenv("XDG_DATA_DIRS=/boot/system/non-packaged/data:/boot/system/data");

    QHaikuIntegration *newHaikuIntegration = new QHaikuIntegration(parameters, argc, argv);
    connect(haikuApplication, SIGNAL(appQuit()), newHaikuIntegration, SLOT(platformAppQuit()));
    return newHaikuIntegration;
}
