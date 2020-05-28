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

#include <QtEventDispatcherSupport/private/qgenericunixeventdispatcher_p.h>
#include <QtFontDatabaseSupport/private/qgenericunixfontdatabase_p.h>
#include <QtFontDatabaseSupport/private/qfreetypefontdatabase_p.h>
#include <QtGui/private/qpixmap_raster_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <private/qsimpledrag_p.h>

#include <qpa/qplatformfontdatabase.h>
#include <qpa/qplatformservices.h>
#include <qpa/qplatformopenglcontext.h>

#include "qhaikuintegration.h"
#include "qhaikuapplication.h"
#include "qhaikuwindow.h"
#include "qhaikucommon.h"
#include "qhaikutheme.h"
#include "qhaikuclipboard.h"
#include "qhaikuservices.h"
#include "qhaikuplatformfontdatabase.h"
#include "qhaikusystemlocale.h"

#if !defined(QT_NO_OPENGL)
#include <GLView.h>
#endif

QT_BEGIN_INCLUDE_NAMESPACE
extern char **environ;
QT_END_INCLUDE_NAMESPACE

QT_BEGIN_NAMESPACE

QHaikuIntegration::QHaikuIntegration(const QStringList &parameters, int &argc, char **argv)
	: QObject(), QPlatformIntegration()
{
	Q_UNUSED(parameters);
	Q_UNUSED(argc);
	Q_UNUSED(argv);
	m_screen = new QHaikuScreen();
	QWindowSystemInterface::handleScreenAdded(m_screen);
    m_fontDatabase = new QHaikuPlatformFontDatabase();
	m_services = new QHaikuServices();
	m_clipboard = new QHaikuClipboard();
	m_haikuSystemLocale = new QHaikuSystemLocale;
	m_drag = new QSimpleDrag();
}

QHaikuIntegration::~QHaikuIntegration()
{
	delete m_fontDatabase;
	delete m_haikuSystemLocale;
	delete m_clipboard;
	delete m_drag;
	delete m_services;

	QWindowSystemInterface::handleScreenRemoved(m_screen);

	HQApplication *haikuApplication = static_cast<HQApplication*>(be_app);
	if (haikuApplication->QtFlags() & Q_KILL_ON_EXIT)
		kill(::getpid(), SIGKILL);
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

	// Inject system environment (hack for QuickLaunch)
	QProcess proc;
	QStringList envList = QProcess::systemEnvironment();

	if ( envList.filter("HOME=").size() == 0 ) {
		proc.start("/bin/sh", QStringList() << "-c" << "source /boot/system/boot/SetupEnvironment; env ");
		proc.waitForFinished();
		QString resultSystemEnv(proc.readAllStandardOutput());
		envList << resultSystemEnv.split("\n");

		proc.start("/bin/sh", QStringList() << "-c" << "source /boot/home/config/settings/boot/UserSetupEnvironment; env ");
		proc.waitForFinished();
		QString resultUserEnv(proc.readAllStandardOutput());
		envList << resultUserEnv.split("\n");

		// Set XDG variables
		envList << "XDG_CONFIG_HOME=/boot/home/config/settings";
		envList << "XDG_CONFIG_DIRS=/boot/system/settings";
		envList << "XDG_CACHE_HOME=/boot/home/config/cache";
		envList << "XDG_DATA_HOME=/boot/home/config/non-packaged/data";
		envList << "XDG_DATA_DIRS=/boot/system/non-packaged/data:/boot/system/data";
	}

	thread_id my_thread;
	HQApplication *haikuApplication = NULL;

	if (be_app == NULL) {
		haikuApplication = new HQApplication(appSignature.toUtf8().constData());
		
		uint32 qtFlags = 0;
		BResources *appResource = BApplication::AppResources();
		if (appResource != NULL) {
			size_t qtFlagsTextSize = 0;
			const char *qtFlagsText = (const char*)appResource->LoadResource(B_STRING_TYPE, "QT:QPA_FLAGS", &qtFlagsTextSize);			
			if (qtFlagsText != NULL && qtFlagsTextSize > 0) {
				BString qtFlagsString(qtFlagsText);
				if (qtFlagsString.FindFirst("Q_REF_TO_ARGV") != B_ERROR)
					qtFlags |= Q_REF_TO_ARGV;
				if (qtFlagsString.FindFirst("Q_REF_TO_FORK") != B_ERROR)
					qtFlags |= Q_REF_TO_FORK;
				if (qtFlagsString.FindFirst("Q_KILL_ON_EXIT") != B_ERROR)
					qtFlags |= Q_KILL_ON_EXIT;
			}
		}
		haikuApplication->SetQtFlags(qtFlags);		

		my_thread = spawn_thread(haikuApplicationThread, "BApplication_thread", B_NORMAL_PRIORITY, (void*)haikuApplication);
		resume_thread(my_thread);

		if (settings.value("hide_from_deskbar", true).toBool()) {
			BMessage message;
			message.what = 'BRAQ';
			message.AddInt32("be:team", ::getpid());
			BMessenger("application/x-vnd.Be-TSKB").SendMessage(&message);
		}

		haikuApplication->UnlockLooper();
		haikuApplication->waitForRun();

		if (haikuApplication->openFiles().size() > 0) {
			// Replace argv data
			argc = 1;
			for ( const auto& fileName : haikuApplication->openFiles() ) {
				if (haikuApplication->QtFlags() & Q_REF_TO_ARGV)
					argv[argc++] = strdup(fileName.toUtf8().data());
				else
					QCoreApplication::postEvent(QCoreApplication::instance(), new QFileOpenEvent(fileName));
			}
		}

		// Rebuild environment arrays
		clearenv();
		for ( const auto& envValue : envList )
			putenv(envValue.toUtf8().data());
	}

	// Enable software rendering for QML
	if (settings.value("qml_softwarecontext", true).toBool())
		setenv("QMLSCENE_DEVICE", "softwarecontext", 0);
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
			putenv(http_proxy.toUtf8().data());
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
			putenv(https_proxy.toUtf8().data());
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
			putenv(ftp_proxy.toUtf8().data());
		}
		QString no_proxy = settings.value("no_proxy_list", QString("")).toString();
		if (!no_proxy.isEmpty()) {
			no_proxy = "no_proxy=\"" + no_proxy + "\"";
			putenv(no_proxy.toUtf8().data());
		}
	}
	settings.endGroup();

    QHaikuIntegration *newHaikuIntegration = new QHaikuIntegration(parameters, argc, argv);
    connect(haikuApplication, SIGNAL(applicationQuit()), newHaikuIntegration, SLOT(platformAppQuit()), Qt::BlockingQueuedConnection);

    return newHaikuIntegration;
}

int32 QHaikuIntegration::haikuApplicationThread(void *data)
{
	HQApplication *app = static_cast<HQApplication*>(data);
	app->LockLooper();
	app->Run();
	return B_OK;
}

bool QHaikuIntegration::platformAppQuit()
{
	if (QGuiApplicationPrivate::instance()->threadData.loadRelaxed()->eventLoops.isEmpty())
		return true;
	return QWindowSystemInterface::handleApplicationTermination<QWindowSystemInterface::SynchronousDelivery>();
}

bool QHaikuIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case OpenGL: return true;
    case ThreadedOpenGL: return true;
    case MultipleWindows: return true;
  //  case RasterGLSurface: return true;
    case AllGLFunctionsQueryable: return true;

    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow *QHaikuIntegration::createPlatformWindow(QWindow *window) const
{
    QPlatformWindow *w = new QHaikuWindow(window);
    w->requestActivateWindow();
    return w;
}

QStringList QHaikuIntegration::themeNames() const
{
    return QStringList(QHaikuTheme::name());
}

QPlatformTheme *QHaikuIntegration::createPlatformTheme(const QString &name) const
{
    if (name == QHaikuTheme::name())
        return new QHaikuTheme(this);
    return NULL;
}

QPlatformBackingStore *QHaikuIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QHaikuBackingStore(window);
}

#if !defined(QT_NO_OPENGL)
QPlatformOpenGLContext *QHaikuIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
	// Ugly temporary solution for opengl applications blacklisting
	QStringList blaskList;
	blaskList << "qupzilla" << "otter-browser" << "otter" << "browser";
	QString appName = QCoreApplication::applicationName().remove("_x86");
   	if (blaskList.contains(appName, Qt::CaseInsensitive))
   		return NULL;

   	return new QHaikuGLContext(context);
}
#endif

QAbstractEventDispatcher *QHaikuIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}

QPlatformFontDatabase *QHaikuIntegration::fontDatabase() const
{
    return m_fontDatabase;
}

QPlatformDrag *QHaikuIntegration::drag() const
{
    return m_drag;
}

QPlatformClipboard *QHaikuIntegration::clipboard() const
{
    return m_clipboard;
}

QPlatformServices *QHaikuIntegration::services() const
{
    return m_services;
}

#if !defined(QT_NO_OPENGL)
QHaikuGLContext::QHaikuGLContext(QOpenGLContext *context)
	: QPlatformOpenGLContext()  
{
	d_format = context->format();

    if (d_format.renderableType() == QSurfaceFormat::DefaultRenderableType)
        d_format.setRenderableType(QSurfaceFormat::OpenGL);
    if (d_format.renderableType() != QSurfaceFormat::OpenGL)
        return;

	glview = new BGLView(BRect(0,0,640,480), "bglview", B_FOLLOW_NONE, 0, BGL_RGB);
	qDebug() << "QHaikuGLContext";
}

QHaikuGLContext::~QHaikuGLContext()
{
	qDebug() << "~QHaikuGLContext";
//	delete glview;
}

QFunctionPointer QHaikuGLContext::getProcAddress(const char *procName)
{
	void *ptr = glview->GetGLProcAddress(procName);
	return (QFunctionPointer)ptr;
}


bool QHaikuGLContext::makeCurrent(QPlatformSurface *surface)
{
	QSize size = surface->surface()->size();
	QHaikuWindow *window = static_cast<QHaikuWindow *>(surface);
    if (!window)
        return false;
        	
	if (window->m_window->fGLView == NULL) {
		window->m_window->fGLView = glview;
		window->m_window->Lock();
		window->m_window->AddChild(glview);
		glview->ResizeTo(size.width(), size.height());
		glViewport(0, 0, size.width(), size.height());
		window->m_window->Unlock();
	}
	
//	QPlatformOpenGLContext::makeCurrent(surface);
//	window->m_window->fGLView->LockGL();
	return true;
}

void QHaikuGLContext::doneCurrent()
{
//	QPlatformOpenGLContext::doneCurrent();
//	glview->UnlockGL();
}

void QHaikuGLContext::swapBuffers(QPlatformSurface *surface)
{
	QHaikuWindow *window = static_cast<QHaikuWindow *>(surface);

	//window->m_window->fGLView->LockGL();
	glview->SwapBuffers();
	//window->m_window->fGLView->UnlockGL();
}

QSurfaceFormat QHaikuGLContext::format() const
{
    return d_format;
}

bool QHaikuGLContext::isSharing() const
{
    return false;
}

bool QHaikuGLContext::isValid() const
{
    return true;
}

#endif

QT_END_NAMESPACE
