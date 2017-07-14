#ifndef DESKBARVIEW_H
#define DESKBARVIEW_H

#include <OS.h>
#include <app/MessageRunner.h>
#include <View.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <Message.h>
#include <Bitmap.h>
#include <String.h>

#define TRAY_MOUSEDOWN 	1
#define TRAY_MOUSEUP	2

class DeskbarView : public BView {
	public:
		DeskbarView(team_id tid);
		~DeskbarView();
		void MouseDown(BPoint point);
		void MouseUp(BPoint point);
		void MouseMoved(BPoint point, uint32 transit,const BMessage *message);
		void Draw(BRect r);
		void MessageReceived(BMessage *message);
		void AttachedToWindow();
		DeskbarView(BMessage *message);
		static DeskbarView *Instantiate(BMessage *data);	
		virtual	status_t Archive(BMessage *data, bool deep = true) const;

	private:
		BPopUpMenu		*RightClickPopUp;
		BBitmap			*fBitmap;
		int32			lastButtons;		
		entry_ref		appref;
		rgb_color		color;
		team_id			team;
		int32			id;
		unsigned int	ticks;
		BBitmap			*icon;
		BMessenger 		ReplyMessenger;		
		const void 		*traysysobject;
		BString			applicationName;
};

#endif
