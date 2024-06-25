/*
 * Copyright 2004-2007, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Calisto Abel Mathias, calisto.mathias.25@gmail.com
 */

#include "SaveWindow.h"

#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Messenger.h>
#include <TextControl.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Find Panel"


BSaveWindow::BSaveWindow(BMessenger* receiver)
	:
	BWindow(BRect(), B_TRANSLATE("Save Query"), B_TITLED_WINDOW_LOOK,
		B_FLOATING_APP_WINDOW_FEEL, B_NOT_ZOOMABLE |
		B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fMessenger(new BMessenger(*receiver))
{

	// Initializing All Private Member Variables
	fQueryName = new BTextControl("Query Name", B_TRANSLATE("Query Name"), "", NULL);
	fIncludeInTemplates = new BCheckBox("Include in Templates",
		B_TRANSLATE("Include in Templates"), NULL);
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
			.AddGroup(B_VERTICAL, 0.0f)
				.Add(fIncludeInTemplates)
				.Add(fSaveInDefaultDirectory)
			.End()
			.AddGlue()
			.Add(fButton)
		.End()
	.End();

	CenterOnScreen();

	fQueryName->MakeFocus();
	fButton->MakeDefault(true);
}


BSaveWindow::~BSaveWindow()
{
	delete fMessenger;
}


bool
BSaveWindow::QuitRequested()
{
	return true;
}


void
BSaveWindow::MessageReceived(BMessage* message)
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

			const char* queryName = fQueryName->Text();
			bool includeInTemplates = fIncludeInTemplates->Value() == B_CONTROL_ON;
			bool saveInDefaultDirectory = fSaveInDefaultDirectory->Value() == B_CONTROL_ON;

			BMessage* closeMessage = new BMessage(kCloseSaveQueryPanel);
			closeMessage->AddString("Query Name", queryName);
			closeMessage->AddBool("Include In Templates", includeInTemplates);
			closeMessage->AddBool("Save In Default Directory", saveInDefaultDirectory);

			fMessenger->SendMessage(closeMessage);
			Quit();

			break;
		}
	}
}
