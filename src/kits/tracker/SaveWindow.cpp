/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Messenger.h>
#include <TextControl.h>

#include "SaveWindow.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Find Panel"

BSaveWindow::BSaveWindow(BWindow *parentWindow)
	:
	BWindow(BRect(), B_TRANSLATE("Save Query"), B_TITLED_WINDOW_LOOK, 
		B_FLOATING_APP_WINDOW_FEEL, B_NOT_ZOOMABLE
			| B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS)
{

	// Initializing All Private Member Variables
	fMessenger = new BMessenger(parentWindow);
	fQueryName = new BTextControl("Query Name", B_TRANSLATE("Query Name"), "", NULL);
	fIncludeInFavorites = new BCheckBox("Include in Templates", 
		B_TRANSLATE("Include in Favorites"), NULL);
	fSaveInDefaultDirectory = new BCheckBox("Save In Default Directory", 
		B_TRANSLATE("Save in Default Directory"), NULL);
	fButton = new BButton(B_TRANSLATE("Save"), new BMessage(kOpenSaveQueryPanel));
	
	// Setting Initial States
	fQueryName->MakeFocus();
	fQueryName->SetModificationMessage(new BMessage(kNameEdited));
	fButton->SetEnabled(false);
	
	// Setting Targets
	fQueryName->SetTarget(this);
	fButton->SetTarget(this);
	
	// Laying Out All the Elements
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fQueryName)
		.AddGroup(B_HORIZONTAL)
			.AddGroup(B_VERTICAL)
				.Add(fIncludeInFavorites)
				.Add(fSaveInDefaultDirectory)
			.End()
			.Add(fButton)
		.End()
	.End();
	
	CenterOnScreen();
}

BSaveWindow::~BSaveWindow()
{
	// Empty Destructor
}

bool
BSaveWindow::QuitRequested()
{
	return true;
}

void
BSaveWindow::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
		case kNameEdited:
		{
			if (strcmp(fQueryName->Text(),""))
				fButton->SetEnabled(true);
			else
				fButton->SetEnabled(false);
			break;	
		}
		
		case kOpenSaveQueryPanel:
		{
			Hide();
			
			const char *queryName = fQueryName->Text();
			bool includeInFavorites = fIncludeInFavorites->Value() == B_CONTROL_ON;
			bool saveInDefaultDirectory = fSaveInDefaultDirectory->Value() == B_CONTROL_ON;
			
			BMessage *closeMessage = new BMessage(kCloseSaveQueryPanel);
			closeMessage->AddString("Query Name", queryName);
			closeMessage->AddBool("Include In Favorites", includeInFavorites);
			closeMessage->AddBool("Save In Default Directory", saveInDefaultDirectory);
			
			fMessenger->SendMessage(closeMessage);
			Quit();
			
			break;
		}
	}
}