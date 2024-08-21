#include "TAttributeSearchField.h"


#include <fs_attr.h>

#include <Button.h>
#include <Catalog.h>
#include <Debug.h>
#include <LayoutBuilder.h>
#include <PopUpMenu.h>
#include <StringView.h>

#include "TAttributeColumn.h"
#include "TFindPanelConstants.h"
#include "TPopUpTextControl.h"

namespace BPrivate {

TAttributeSearchField::TAttributeSearchField(TAttributeSearchColumn* column)
	:
	BView("search-field", B_WILL_DRAW),
	fAttributeColumn(column)
{
	fTextControl = new TPopUpTextControl(this);
	fLabel = new BStringView("label", NULL);
	
	fAddButton = new BButton(B_TRANSLATE("+"), NULL);
	BMessage* message = new BMessage(kAddSearchField);
	message->AddPointer("pointer", this);
	fAddButton->SetMessage(message);
	
	fRemoveButton = new BButton(B_TRANSLATE("-"), NULL);
	message = new BMessage(kRemoveSearchField);
	message->AddPointer("pointer", this);
	fRemoveButton->SetMessage(message);
	
	BSize textControlSize = fTextControl->PreferredSize();
	BSize buttonSize(textControlSize.Height(), textControlSize.Height());
	fAddButton->ResizeTo(buttonSize);
	fRemoveButton->ResizeTo(buttonSize);
	fAddButton->SetExplicitSize(buttonSize);
	fRemoveButton->SetExplicitSize(buttonSize);
	fAddButton->SetExplicitPreferredSize(buttonSize);
	fRemoveButton->SetExplicitPreferredSize(buttonSize);
	
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0.0f)
		.Add(fLabel)
		.AddGroup(B_HORIZONTAL, 0.0f)
			.Add(fTextControl)
			.Add(fAddButton)
			.Add(fRemoveButton)
		.End()
	.End();
	
	BSize size;
	GetRequiredSize(&size);
	SetSize(size);
}


TAttributeSearchField::~TAttributeSearchField()
{
}


void
TAttributeSearchField::AttachedToWindow()
{
	fAddButton->SetTarget(this);
	fRemoveButton->SetTarget(this);
	
	fTextControl->PopUpMenu()->SetTargetForItems(this);
	fTextControl->SetTarget(this);
}


query_op
TAttributeSearchField::GetAttributeOperatorFromIndex(int32 index) const
{
	// Must be overridden by child function.
	return B_AND;
}


status_t
TAttributeSearchField::SetSize(BSize size)
{
	fSize = size;
	ResizeTo(fSize);
	SetExplicitSize(fSize);
	SetExplicitPreferredSize(fSize);
	return B_OK;
}


status_t
TAttributeSearchField::PopulateOptionsMenu()
{
	if (fTextControl->PopUpMenu() == NULL)
		return B_BAD_VALUE;

	for (int32 i = 0; i < combinationOperatorsLength; ++i) {
		BMenuItem* item = new BMenuItem(combinationOperators[i], NULL);
		BMessage* message = new BMessage(kMenuOptionClicked);
		status_t error;
		if ((error = message->AddPointer("pointer", item)) != B_OK) {
			delete item;
			delete message;
			return error;
		}
		
		item->SetMessage(message);
		fTextControl->PopUpMenu()->AddItem(item);
	}
	
	fTextControl->PopUpMenu()->AddSeparatorItem();
	
	return B_OK;
}


bool
TAttributeSearchField::IsCombinationOperatorMenuItem(BMenuItem* item) const
{
	if (item == NULL)
		return false;
	
	for (int32 i = 0; i < combinationOperatorsLength; ++i) {
		if (item == fTextControl->PopUpMenu()->ItemAt(i))
			return true;
	}
	
	return false;
}


status_t
TAttributeSearchField::HandleMenuOptionClicked(BMenuItem* item)
{
	if (item == NULL)
		return B_BAD_VALUE;
	
	BMenu* menu = fTextControl->PopUpMenu();
	if (menu == NULL)
		return B_BAD_VALUE;
	
	bool isCombinationOperator = IsCombinationOperatorMenuItem(item);
	int32 startingIndex = isCombinationOperator ? 0 : combinationOperatorsLength + 1;
	int32 endingIndex = isCombinationOperator ? combinationOperatorsLength + 1: menu->CountItems();
	
	for (int32 i = startingIndex; i < endingIndex; ++i)
		menu->ItemAt(i)->SetMarked(false);
	
	item->SetMarked(true);
	return UpdateLabel();
}


status_t
TAttributeSearchField::GetAttributeOperator(query_op* attributeOperator) const
{
	if (attributeOperator == NULL)
		return B_BAD_VALUE;
	
	ASSERT(fTextControl != NULL);
	BPopUpMenu* optionsMenu = fTextControl->PopUpMenu();
	int32 attributeOperatorIndex = -1;
	for (int32 i = combinationOperatorsLength + 1; i < optionsMenu->CountItems(); ++i) {
		if (optionsMenu->ItemAt(i)->IsMarked()) {
			attributeOperatorIndex = i - combinationOperatorsLength - 1;
			break;
		}
	}
	
	if (attributeOperatorIndex == -1)
		return B_ENTRY_NOT_FOUND;
	
	*attributeOperator = GetAttributeOperatorFromIndex(attributeOperatorIndex);
	return B_OK;
}


status_t
TAttributeSearchField::GetCombinationOperator(query_op* combinationOperator) const
{
	if (combinationOperator == NULL)
		return B_BAD_VALUE;
	
	BPopUpMenu* optionsMenu = fTextControl->PopUpMenu();
	int32 combinationOperatorIndex = -1;
	for (int32 i = 0; i < combinationOperatorsLength; ++i) {
		if (optionsMenu->ItemAt(i)->IsMarked()) {
			combinationOperatorIndex = i;
			break;
		}
	}
	
	if (combinationOperatorIndex == -1)
		return B_ENTRY_NOT_FOUND;
	
	*combinationOperator = combinationQueryOps[combinationOperatorIndex];
	return B_OK;
}


status_t
TAttributeSearchField::UpdateLabel()
{
	BPopUpMenu* menu = fTextControl->PopUpMenu();
	if (menu == NULL)
		return B_BAD_VALUE;
	
	BString combinationOperatorLabel = "";
	BString actionOperatorLabel = "";
	
	for (int32 i = combinationOperatorsLength + 1; i < menu->CountItems(); ++i) {
		BMenuItem* item = menu->ItemAt(i);
		if (item && item->IsMarked())
			actionOperatorLabel = item->Label();
	}
	
	for (int32 i = 0; i < combinationOperatorsLength + 1; ++i) {
		BMenuItem* item = menu->ItemAt(i);
		if (item && item->IsMarked())
			combinationOperatorLabel = item->Label();
	}
	
	if (combinationOperatorLabel == "" && actionOperatorLabel == "")
		return B_ENTRY_NOT_FOUND;
	
	if (combinationOperatorLabel == "") {
		fLabel->SetText(actionOperatorLabel.String());
	} else {
		combinationOperatorLabel.Append(" ").Append(actionOperatorLabel);
		fLabel->SetText(combinationOperatorLabel);
	}

	return B_OK;
}


status_t
TAttributeSearchField::EnableAddButton(bool enable)
{
	if (fAddButton == NULL)
		return B_ERROR;
	
	fAddButton->SetEnabled(enable);
	return B_OK;
}


status_t
TAttributeSearchField::EnableRemoveButton(bool enable)
{
	if (fRemoveButton == NULL)
		return B_ERROR;
	
	fRemoveButton->SetEnabled(enable);
	return B_OK;
}


status_t
TAttributeSearchField::GetRequiredSize(BSize* size) const
{
	if (size == NULL)
		return B_BAD_VALUE;

	float height;
	GetRequiredHeight(&height);
	
	*size = BSize(B_SIZE_UNSET, height);
	return B_OK;
}


status_t
TAttributeSearchField::GetRequiredHeight(float* height) const
{
	if (height == NULL)
		return B_BAD_VALUE;
		
	float heightSetter = 0.0f;
	heightSetter += fTextControl->PreferredSize().Height();
	heightSetter += fTextControl->PreferredSize().Height();
	*height = heightSetter;
	return B_OK;
}


void
TAttributeSearchField::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMenuOptionClicked:
		{
			BMenuItem* source = NULL;
			if (message->FindPointer("pointer", reinterpret_cast<void**>(&source)) != B_OK
				|| source == NULL) {
				break;
			}
			
			HandleMenuOptionClicked(source);
			break;
		}
	
		default:
		{
			_inherited::MessageReceived(message);
			break;
		}
	}
}


TNumericAttributeSearchField::TNumericAttributeSearchField(
	TNumericAttributeSearchColumn* column)
	:
	TAttributeSearchField(column)
{
	PopulateOptionsMenu();
	fTextControl->PopUpMenu()->ItemAt(0)->SetMarked(true);
	fTextControl->PopUpMenu()->ItemAt(combinationOperatorsLength + 1)->SetMarked(true);
	UpdateLabel();
}


TNumericAttributeSearchField::~TNumericAttributeSearchField()
{
}


status_t
TNumericAttributeSearchField::PopulateOptionsMenu()
{
	_inherited::PopulateOptionsMenu();

	BPopUpMenu* optionsMenu = fTextControl->PopUpMenu();
	for (int32 i = 0; i < numericAttributeOperatorsLength; ++i) {
		BMenuItem* item = new BMenuItem(numericAttributeOperators[i], NULL);
		BMessage* message = new BMessage(kMenuOptionClicked);
		status_t error;
		if ((error = message->AddPointer("pointer", item)) != B_OK) {
			delete item;
			delete message;
			return error;
		}
		item->SetMessage(message);
		optionsMenu->AddItem(item);
	}
	
	return B_OK;
}


query_op
TNumericAttributeSearchField::GetAttributeOperatorFromIndex(int32 index) const
{
	if (index < 0 || index >= numericAttributeOperatorsLength)
		return B_AND;
	
	return numericQueryOps[index];
}


TTemporalAttributeSearchField::TTemporalAttributeSearchField(
	TTemporalAttributeSearchColumn* searchColumn)
	:
	TAttributeSearchField(searchColumn)
{
	PopulateOptionsMenu();
	fTextControl->PopUpMenu()->ItemAt(0)->SetMarked(true);
	fTextControl->PopUpMenu()->ItemAt(combinationOperatorsLength + 1)->SetMarked(true);
	UpdateLabel();
}


TTemporalAttributeSearchField::~TTemporalAttributeSearchField()
{
}


status_t
TTemporalAttributeSearchField::PopulateOptionsMenu()
{
	_inherited::PopulateOptionsMenu();

	BPopUpMenu* optionsMenu = fTextControl->PopUpMenu();
	if (optionsMenu == NULL)
		return B_BAD_VALUE;
	
	for (int32 i = 0; i < temporalAttributeOperatorsLength; ++i) {
		BMenuItem* item = new BMenuItem(temporalAttributeOperators[i], NULL);
		BMessage* message = new BMessage(kMenuOptionClicked);
		status_t error;
		if ((error = message->AddPointer("pointer", item)) != B_OK) {
			delete item;
			delete message;
			return error;
		}
		item->SetMessage(message);
		optionsMenu->AddItem(item);
	}

	return B_OK;
}


query_op
TTemporalAttributeSearchField::GetAttributeOperatorFromIndex(int32 index) const
{
	if (index < 0 || index >= temporalAttributeOperatorsLength)
		return B_AND;
	
	return temporalQueryOps[index];
}


TStringAttributeSearchField::TStringAttributeSearchField(TStringAttributeSearchColumn* column)
	:
	TAttributeSearchField(column)
{
	PopulateOptionsMenu();
	fTextControl->PopUpMenu()->ItemAt(0)->SetMarked(true);
	fTextControl->PopUpMenu()->ItemAt(combinationOperatorsLength + 1)->SetMarked(true);
	UpdateLabel();
}


TStringAttributeSearchField::~TStringAttributeSearchField()
{
}


status_t
TStringAttributeSearchField::PopulateOptionsMenu()
{
	_inherited::PopulateOptionsMenu();

	BPopUpMenu* menu = fTextControl->PopUpMenu();
	if (menu == NULL)
		return B_BAD_VALUE;
	
	for (int32 i = 0; i < stringAttributeOperatorsLength; ++i) {
		BMenuItem* item = new BMenuItem(stringAttributeOperators[i], NULL);
		BMessage* message = new BMessage(kMenuOptionClicked);
		status_t error;
		if ((error = message->AddPointer("pointer", item)) != B_OK) {
			delete item;
			delete message;
			return error;
		}
		item->SetMessage(message);
		menu->AddItem(item);
	}
	
	return B_OK;
}


query_op
TStringAttributeSearchField::GetAttributeOperatorFromIndex(int32 index) const
{
	if (index < 0 || index >= stringAttributeOperatorsLength)
		return B_AND;
	
	return stringQueryOps[index];
}


}