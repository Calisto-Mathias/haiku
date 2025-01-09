#ifndef T_POP_UP_TEXT_CONTROL_H
#define T_POP_UP_TEXT_CONTROL_H

#include <TextControl.h>


class BMenuItem;
class BPopUpMenu;

namespace BPrivate {

class TAttributeSearchField;


class TPopUpTextControl : public BTextControl {
public:
								TPopUpTextControl(TAttributeSearchField*);
			BPopUpMenu*			PopUpMenu() {return fOptionsMenu;}

protected:
	virtual	void				MessageReceived(BMessage*);

private:
	virtual	void				LayoutChanged() override;
	virtual	void				DrawAfterChildren(BRect) override;
	virtual	void				MouseDown(BPoint) override;
private:
			BPopUpMenu*			fOptionsMenu;
			TAttributeSearchField*	fSearchField;
};

}

#endif