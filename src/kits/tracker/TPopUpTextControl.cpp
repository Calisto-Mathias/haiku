#include "TPopUpTextControl.h"


#include <Catalog.h>
#include <ControlLook.h>
#include <TextControl.h>
#include <MenuItem.h>
#include <PopUpMenu.h>

#include "TAttributeSearchField.h"
#include "TFindPanelConstants.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TPopUpTextControl"


namespace BPrivate {

TPopUpTextControl::TPopUpTextControl(TAttributeSearchField* searchField)
	:
	BTextControl("text-control", NULL, NULL, NULL, B_WILL_DRAW | B_NAVIGABLE | B_DRAW_ON_CHILDREN),
	fOptionsMenu(new BPopUpMenu("options-menu",false, false)),
	fSearchField(searchField)
{
	// Empty Constructor
}


void
TPopUpTextControl::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMenuOptionClicked:
		{
			BMessenger(fSearchField).SendMessage(message);
			break;
		}
		default:
			BTextControl::MessageReceived(message);
			break;
	}
}


void
TPopUpTextControl::LayoutChanged()
{
	BRect frame = TextView()->Frame();
	frame.left = BControlLook::ComposeSpacing(B_USE_WINDOW_SPACING) + 2;
	TextView()->MoveTo(frame.left, frame.top);
	TextView()->ResizeTo(Bounds().Width() - frame.left - 2, frame.Height());
	
	BTextControl::LayoutChanged();
}


void
TPopUpTextControl::DrawAfterChildren(BRect update)
{
	BRect frame = Bounds();
	frame.right = frame.left + BControlLook::ComposeSpacing(B_USE_WINDOW_SPACING);
	frame.top++;
	frame.bottom --;
	be_control_look->DrawMenuFieldBackground(this, frame, update,
		ui_color(B_PANEL_BACKGROUND_COLOR), true);
}


void
TPopUpTextControl::MouseDown(BPoint where)
{
	BRect frame = Bounds();
	if (where.x < BControlLook::ComposeSpacing(B_USE_WINDOW_SPACING)) {
		BMenuItem* item = fOptionsMenu->Go(TextView()->ConvertToScreen(
			TextView()->Bounds().LeftTop()));
		if (item) {
			BMessage* message = item->Message();
			MessageReceived(message);
		}
	} else {
		BTextControl::MouseDown(where);
	}
}

}
