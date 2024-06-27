/*
 * Copyright 2004-2007, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Calisto Abel Mathias, calisto.mathias.25@gmail.com
 */
#ifndef _SAVE_WINDOW_H
#define _SAVE_WINDOW_H

#include <Window.h>


class BButton;
class BCheckBox;
class BMessenger;
class BTextControl;

namespace BPrivate {

const uint32 kOpenSaveQueryPanel = 'Fosv';
const uint32 kCloseSaveQueryPanel = 'Fcsv';
const uint32 kNameEdited ='FNmE';

class BSaveWindow : public BWindow{
public:
								BSaveWindow(BMessenger*);
	virtual						~BSaveWindow();
	virtual	bool				QuitRequested();

protected:
	virtual	void				MessageReceived(BMessage*);

private:
			BTextControl*		fQueryName;
			BCheckBox*			fIncludeInTemplates;
			BCheckBox*			fSaveInDefaultDirectory;
			BButton*			fButton;
			BMessenger*			fMessenger;
};

} // namespace BPrivate

using namespace BPrivate;

#endif
