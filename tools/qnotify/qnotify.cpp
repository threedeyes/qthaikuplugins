/* 
** Copyright (c) 2021 Gerasim Troeglazov, 3dEyes@gmail.com
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
*/

#include <stdlib.h>
#include <stdio.h>

#include <OS.h>
#include <Application.h>
#include <Message.h>
#include <String.h>
#include <StringList.h>

#define NOTIFY_PORT_NAME "tg_notify"
#define NOTIFY_MESSAGE 'QNTF'

#define NOTIFY_MODE_PORT 		1
#define NOTIFY_MODE_MESSENGER 	2

class QNotifyLauncherApp : public BApplication {
	public:
		QNotifyLauncherApp(int argc, char **argv);
		void	ArgvReceived(int32 argc, char **argv);
		virtual void ReadyToRun();
};

QNotifyLauncherApp::QNotifyLauncherApp(int argc, char **argv)
	: BApplication(APP_SIGNATURE)
{
}


void
QNotifyLauncherApp::ArgvReceived(int32 argc, char **argv)
{
	if (argc == 2) {
		BMessage argsMsg(NOTIFY_MESSAGE);
		uint32 targetMode = 0;
		BStringList argsList;
		BString targetValue;
		BString args(argv[1]);		
		args.Split(" ", true, argsList);

		for (int i = 0; i < argsList.CountStrings(); i++) {
			BStringList argValue;
			argsList.StringAt(i).Split(":", false, argValue);
			if (argValue.CountStrings() == 2) {
				if (argValue.StringAt(0) == "mode") {
					if (argValue.StringAt(1) == "port")
						targetMode = NOTIFY_MODE_PORT;
					if (argValue.StringAt(1) == "messenger")
						targetMode = NOTIFY_MODE_MESSENGER;
				}
				if (argValue.StringAt(0) == "what" && argValue.StringAt(1).Length() == 4) {
					uint32 what = (((uint32)argValue.StringAt(1)[0]) << 24)
						| (((uint32)argValue.StringAt(1)[1]) << 16)
						| (((uint32)argValue.StringAt(1)[2]) << 8)
						| (((uint32)argValue.StringAt(1)[3]));
					argsMsg.what = what;
				}
				if (argValue.StringAt(0) == "target")
					targetValue = argValue.StringAt(1).String();

				argsMsg.AddString(argValue.StringAt(0).String(), argValue.StringAt(1));
			}
			if (argValue.CountStrings() == 3) {				
				if (argValue.StringAt(1) == "string")
					argsMsg.AddString(argValue.StringAt(0).String(), argValue.StringAt(2));
				if (argValue.StringAt(1) == "bool")
					argsMsg.AddBool(argValue.StringAt(0).String(), argValue.StringAt(2) == "true");
				if (argValue.StringAt(1) == "float")
					argsMsg.AddFloat(argValue.StringAt(0).String(), strtof(argValue.StringAt(2).String(), NULL));
				if (argValue.StringAt(1) == "double")
					argsMsg.AddDouble(argValue.StringAt(0).String(), strtod(argValue.StringAt(2).String(), NULL));
				if (argValue.StringAt(1) == "int32")
					argsMsg.AddInt32(argValue.StringAt(0).String(), atoi(argValue.StringAt(2).String()));
				if (argValue.StringAt(1) == "uint32")
					argsMsg.AddUInt32(argValue.StringAt(0).String(), atoui(argValue.StringAt(2).String()));
				if (argValue.StringAt(1) == "int64")
					argsMsg.AddInt64(argValue.StringAt(0).String(), strtoll(argValue.StringAt(2).String(), NULL, 10));
				if (argValue.StringAt(1) == "uint64")
					argsMsg.AddUInt64(argValue.StringAt(0).String(), strtoull(argValue.StringAt(2).String(), NULL, 10));
			}
		}

		if (targetMode == NOTIFY_MODE_PORT) {
			ssize_t size = size = argsMsg.FlattenedSize();
			if (size < 0)
				return;

			char* buffer = new(std::nothrow) char[size];
			if (!buffer)
				return;
	
			if (argsMsg.Flatten(buffer, size) == B_OK) {
				port_id portId = find_port(targetValue.String());
				if (portId > 0)
					write_port(portId, argsMsg.what, buffer, size);
			}

			delete[] buffer;
		}

		if (targetMode == NOTIFY_MODE_MESSENGER) {
			BMessenger targetMessenger(targetValue.String());
			if (targetMessenger.IsValid()) {
				targetMessenger.SendMessage(&argsMsg);
			}
		}
	}
}

void
QNotifyLauncherApp::ReadyToRun()
{
    Quit();
}

int main(int argc, char **argv)
{	
	QNotifyLauncherApp application(argc, argv);

	if (argc < 2) {
		printf("BNotification action redirector\n qnotify \"message string\"\n\n");
	}

	application.Run();
	return 0;
}
