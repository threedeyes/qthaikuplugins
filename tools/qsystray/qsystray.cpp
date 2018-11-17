#include <OS.h>
#include <Application.h>
#include <Window.h>
#include <Deskbar.h>
#include <View.h>
#include <Roster.h>
#include <Point.h>
#include <Resources.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "qsystray.h"

#ifndef APP_SIGNATURE
#define APP_SIGNATURE	"application/x-vnd.QtSystrayManager"
#endif

#define REPL_NAME		"QtTrayItem"
#define DBAR_SIGNATURE 	"application/x-vnd.Be-TSKB"

DeskbarView::DeskbarView(team_id tid, BRect rect) : BView(rect, REPL_NAME, B_FOLLOW_NONE, B_WILL_DRAW)
{
	id = -1;
	team = tid;
	ticks = 0;
	icon = NULL;
	traysysobject = NULL;
	lastButtons = 0;
}

DeskbarView::DeskbarView(BMessage *message) : BView(message)
{
	const void* data;
	ssize_t numBytes;
	message->FindData("color", B_ANY_TYPE, &data, &numBytes);
	color = *((rgb_color*)data);
	message->FindData("team", B_ANY_TYPE, &data, &numBytes);
	team = *((team_id*)data);
	id = -1;
	ticks = 0;
	icon = NULL;
	traysysobject = NULL;	
	lastButtons = 0;
}

DeskbarView *DeskbarView::Instantiate(BMessage *data) {
	if (!validate_instantiation(data, REPL_NAME))
		return NULL;
	return new DeskbarView(data);
}

status_t DeskbarView::Archive(BMessage *data, bool deep) const {
	BView::Archive(data, deep);
	data->AddString("add_on", APP_SIGNATURE);
	data->AddString("class", REPL_NAME);
	
	data->AddData("color",B_ANY_TYPE,&color,sizeof(rgb_color));
	data->AddData("team",B_ANY_TYPE,&team,sizeof(team_id));
	return B_OK;
}		

void DeskbarView::AttachedToWindow()
{
	ticks = 0;	
	BMessage* tickMsg = new BMessage('LIVE');
	BMessageRunner *runner = new BMessageRunner( this, tickMsg, 1000000 );
	color = Parent()->ViewColor();	
	BView::AttachedToWindow();
}

void DeskbarView::Draw(BRect r)
{
	SetDrawingMode(B_OP_COPY);
	SetHighColor(Parent()->ViewColor());
	FillRect(Bounds());

	if (ticks>3 && !icon) {
		SetHighColor(32,32,32,100);
		SetDrawingMode(B_OP_ALPHA);
		DrawChar('?',BPoint(4,12));
	}

	if (icon) {
		float dx = (Bounds().Width() - icon->Bounds().Width()) / 2;
		float dy = (Bounds().Height() - icon->Bounds().Height()) / 2;
		SetDrawingMode(B_OP_ALPHA);
		DrawBitmap(icon, BPoint(dx,dy));
	}
}

void DeskbarView::MouseMoved(BPoint point, uint32 transit,const BMessage *message)
{
}

void DeskbarView::MouseUp(BPoint point)
{
   uint32 buttons = lastButtons;

   BMessage *mes = new BMessage('TRAY');
   mes->AddInt32("event", TRAY_MOUSEUP);
   mes->AddPoint("point", ConvertToScreen(point));
   mes->AddInt32("buttons", buttons);
   mes->AddInt32("clicks", 1);
   mes->AddData("qtrayobject", B_ANY_TYPE, &traysysobject, sizeof(void*));
   ReplyMessenger.SendMessage(mes);
}

void DeskbarView::MouseDown(BPoint point)
{
   uint32 buttons = Window()->CurrentMessage()->FindInt32("buttons");
   int32 clicks = Window()->CurrentMessage()->FindInt32("clicks");
   lastButtons = buttons;

   BMessage *mes = new BMessage('TRAY');
   mes->AddInt32("event", TRAY_MOUSEDOWN);
   mes->AddPoint("point", ConvertToScreen(point));
   mes->AddInt32("buttons", buttons);
   mes->AddInt32("clicks", clicks);
   mes->AddData("qtrayobject", B_ANY_TYPE, &traysysobject, sizeof(void*));
   ReplyMessenger.SendMessage(mes);
}

void DeskbarView::MessageReceived(BMessage *message) {
	switch (message->what)
	{
		case 'LIVE':
			{
				ticks++;
				Invalidate();
				team_info teamInfo;
				status_t error = get_team_info(team, &teamInfo);
				if (error != B_OK && id>0) {
					BDeskbar deskbar;
					deskbar.RemoveItem(id);
				} else {
					BMessage *mes=new BMessage(*message);
					mes->AddRect("rect",ConvertToScreen(Bounds()));
					ReplyMessenger.SendMessage(mes);
				}
				break;
			}
		case B_SET_PROPERTY:
			{
				switch( message->FindInt32("what2") ) {
					case 'TTIP':
						{
							const char *tip = NULL;
							status_t res = message->FindString("tooltip", &tip);

							if (!tip || res!=B_OK)
								tip = applicationName.String();
							if (strlen(tip) == 0)
								tip = applicationName.String();
							if (strlen(tip) != 0)
								SetToolTip(tip);
							break;
						}
					case 'BITS':
						{
							BBitmap *oldicon = icon;
							icon = NULL;
							delete oldicon;
							BMessage bits;
							message->FindMessage("icon", &bits);
							icon = new BBitmap(&bits);
							bits.MakeEmpty();
							Invalidate();
							break;
						}
					case '_ID_':
						{
							message->FindInt32("ReplicantID", &id);
							break;
						}
					case 'MSGR':
						{
							ssize_t numBytes;
							const char *name = NULL;
							message->FindMessenger("messenger", &ReplyMessenger);
							message->FindData("qtrayobject", B_ANY_TYPE, &traysysobject, &numBytes);
							if(message->FindString("application_name",&name) == B_OK)
								applicationName.SetTo(name);
							break;
						}
				}
			}
			break;
		default:
			BView::MessageReceived(message);
			break;
	}
}

DeskbarView::~DeskbarView()
{
}

BMessenger GetMessenger(void)
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

	if (aDeskbar.SendMessage(&aMessage, &aReply, 1000000, 1000000) == B_OK)
		aReply.FindMessenger("result", &aResult);
	return aResult;
}

status_t SendMessageToReplicant(int32 index, BMessage *msg)
{
	BMessage aReply;
	status_t aErr = B_OK;

	msg->AddInt32( "what2", msg->what );
	msg->what = B_SET_PROPERTY;

	BMessage uid_specifier(B_ID_SPECIFIER);

	msg->AddSpecifier("View");
	uid_specifier.AddInt32("id", index);
	uid_specifier.AddString("property", "Replicant");
	msg->AddSpecifier(&uid_specifier);

	aErr = GetMessenger().SendMessage( msg, (BHandler*)NULL, 1000000 );
	return aErr;
}

int32 LoadIcon(team_id tid)
{
	BDeskbar deskbar;

	int32 id = -1;

	deskbar.AddItem(new DeskbarView(tid, BRect(0,0, deskbar.MaxItemHeight() - 1, deskbar.MaxItemHeight() - 1)), &id);

	if (id > 0) {
		BMessage msg('_ID_');
		msg.AddInt32("ReplicantID", id);
		SendMessageToReplicant(id, &msg);
	}

	return id;
}

int32 LoadIcon(void)
{
	thread_info threadInfo;
	status_t error = get_thread_info(find_thread(NULL), &threadInfo);

	if (error != B_OK)
		return 0;

	team_id sTeam = threadInfo.team;

	return LoadIcon(sTeam);
}

void RemoveIcon(int32 id)
{
	BDeskbar deskbar;
	deskbar.RemoveItem(id);
}

int main(int argc, char *argv[])
{
	BApplication(APP_SIGNATURE);

	if (argc < 2) {
		printf("QtSystrayManager for Haiku v0.2\n\tqsystray [team_id]\n\n");
		exit(0);
	}

	int32 team_id = atoi(argv[1]);
	int32 id = LoadIcon(team_id);

	printf("%d\n",id);

	return id;
}
