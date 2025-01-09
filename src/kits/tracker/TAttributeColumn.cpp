#include "TAttributeColumn.h"


#include <fs_attr.h>

#include <Box.h>
#include <GroupView.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <StringView.h>

#include "TAttributeSearchField.h"
#include "TFindPanel.h"
#include "TFindPanelConstants.h"
#include "TitleView.h"
#include "TPopUpTextControl.h"
#include "ViewState.h"


namespace BPrivate {

TAttributeColumn::TAttributeColumn(BColumn* column, BColumnTitle* columnTitle,
	TFindPanel* findPanel)
	:
	BView("attribute-column", B_WILL_DRAW),
	fColumn(column),
	fColumnTitle(columnTitle),
	fFindPanel(findPanel)
{
	BRect titleBounds = columnTitle->Bounds();
	BSize size(titleBounds.Width(), B_SIZE_UNSET);
	SetSize(size);
}


TAttributeColumn::~TAttributeColumn()
{
}


void
TAttributeColumn::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kResizeHeight:
		{
			float height;
			if (message->FindFloat("height", &height) != B_OK)
				break;
			
			HandleColumnResize(height);
			break;
		}
		
		case kResizeColumn:
		{
			HandleColumnWidthResize();
		}
		
		case kMoveColumn:
		{
			HandleColumnMoved();
			break;
		}

		default:
		{
			_inherited::MessageReceived(message);
			break;
		}
	}
}


status_t
TAttributeColumn::GetSize(BSize* size) const
{
	if (size == NULL)
		return B_BAD_VALUE;
	
	*size = fSize;
	return B_OK;
}


status_t
TAttributeColumn::SetSize(BSize size)
{
	fSize = size;
	SetExplicitPreferredSize(size);
	SetExplicitSize(size);
	ResizeTo(fSize);
	Invalidate();
	return B_OK;
}

//TODO: Implement checks to see whether the ColumnTitleView is still valid

status_t
TAttributeColumn::GetColumnTitle(BColumnTitle** titleView) const
{
	if (titleView == NULL)
		return B_BAD_VALUE;
	
	*titleView = fColumnTitle;
	return B_OK;
}


status_t
TAttributeColumn::SetColumnTitle(BColumnTitle** titleView)
{
	if (titleView == NULL)
		return B_BAD_VALUE;
	
	fColumnTitle = *titleView;
	BRect bounds = fColumnTitle->Bounds();
	BSize size(bounds.Width(), B_SIZE_UNSET);
	return SetSize(size);
}


status_t
TAttributeColumn::GetColumn(BColumn** column) const
{
	if (column == NULL)
		return B_OK;
	
	*column = fColumn;
	return B_OK;
}


status_t
TAttributeColumn::SetColumn(BColumn** column)
{
	if (column == NULL)
		return B_BAD_VALUE;
	
	fColumn = *column;
	return B_OK;
}


status_t
TAttributeColumn::HandleColumnResize(float parentHeight)
{
	BRect bounds = fColumnTitle->Bounds();
	BSize size(bounds.Width(), parentHeight);
	
	status_t error;
	if ((error = SetSize(size)) != B_OK)
		return error;
	
	MoveTo(bounds.left, 0);
	return B_OK;
}


status_t
TAttributeColumn::HandleColumnWidthResize()
{
	BRect bounds = fColumnTitle->Bounds();
	
	BSize previousSize;
	status_t error;
	if ((error = GetSize(&previousSize)) != B_OK)
		return error;
	
	BSize newSize(bounds.Width(), previousSize.Height());
	if ((error = SetSize(newSize)) != B_OK)
		return error;
	
	return B_OK;
}



status_t
TAttributeColumn::HandleColumnMoved()
{
	BRect bounds = fColumnTitle->Bounds();
	MoveTo(bounds.left, 0);
	return B_OK;
}


TAttributeSearchColumn::TAttributeSearchColumn(BColumn* column, BColumnTitle* columnTitle,
	TFindPanel* findPanel)
	:
	TAttributeColumn(column, columnTitle, findPanel),
	fContainerView(new BGroupView(B_VERTICAL, 2.0f)),
	fBox(new BBox("container-box"))
{
	fContainerView->GroupLayout()->SetInsets(2.0f, 2.0f, 2.0f, 4.0f);
	fBox->AddChild(fContainerView);
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0.0f)
		.AddGlue()
		.Add(fBox)
	.End();
}


TAttributeSearchColumn::~TAttributeSearchColumn()
{
}


TAttributeSearchColumn*
TAttributeSearchColumn::CreateSearchColumnForAttributeType(AttributeType type, BColumn* column,
	BColumnTitle* columnTitle, TFindPanel* findPanel)
{
	switch (type) {
		case AttributeType::NUMERIC:
		{
			return new TNumericAttributeSearchColumn(column, columnTitle, findPanel);
			break;
		}
		
		case AttributeType::TEMPORAL:
		{
			return new TTemporalAttributeSearchColumn(column, columnTitle, findPanel);
			break;
		}
		
		case AttributeType::STRING:
		{
			return new TStringAttributeSearchColumn(column, columnTitle, findPanel);
			break;
		}
		
		default:
		{
			return NULL;
			break;
		}
	}
}


TAttributeSearchField*
TAttributeSearchColumn::CreateAttributeSearchField()
{
	// Child Classes must implement their own version of this function!
	return NULL;
}


status_t
TAttributeSearchColumn::ResetStateForFirstSearchField()
{
	int32 numberOfSearchFields = fSearchFields.CountItems();
	if (numberOfSearchFields == 0)
		return B_BAD_VALUE;
	TAttributeSearchField* firstSearchField = fSearchFields.ItemAt(0);
	
	if (firstSearchField == NULL)
		return B_BAD_VALUE;
	
	firstSearchField->EnableRemoveButton(numberOfSearchFields > 1);
	for (int32 i = 0; i < combinationOperatorsLength; ++i) {
		BMenuItem* combinationMenuItem = firstSearchField->TextControl()->PopUpMenu()->ItemAt(i);
		combinationMenuItem->SetEnabled(false);
		combinationMenuItem->SetMarked(false);
	}
	
	return firstSearchField->UpdateLabel();
}


status_t
TAttributeSearchColumn::AddSearchField(const TAttributeSearchField* after)
{
	if (after == NULL)
		return AddSearchField();
	
	int32 index = fSearchFields.IndexOf(after);
	if (index == -1)
		return B_BAD_VALUE;
	
	return AddSearchField(index + 1);
}


status_t
TAttributeSearchColumn::AddSearchField(int32 index)
{
	if (index < -1)
		return B_BAD_VALUE;

	int32 numberOfPresentSearchFields = fSearchFields.CountItems();

	if (index == -1)
		index = numberOfPresentSearchFields;
	
	TAttributeSearchField* searchField = CreateAttributeSearchField();
	
	fSearchFields.AddItem(searchField, index);
	if (fContainerView && fContainerView->GroupLayout())
		fContainerView->GroupLayout()->AddView(index, searchField);
	
	++numberOfPresentSearchFields;
	
	ResetStateForFirstSearchField();
	
	BSize requiredSize;
	GetRequiredSize(&requiredSize);
	SetBoxSize(requiredSize);

	return B_OK;
}


status_t
TAttributeSearchColumn::RemoveSearchField(TAttributeSearchField* searchField)
{
	if (searchField == NULL)
		return B_BAD_VALUE;
	
	if (fSearchFields.RemoveItem(searchField, false) == false)
		return B_BAD_VALUE;
	
	searchField->RemoveSelf();
	delete searchField;
	searchField = NULL;
	
	BSize refreshedSize;
	GetRequiredSize(&refreshedSize);
	SetBoxSize(refreshedSize);
	
	if (fSearchFields.CountItems() == 1)
		ResetStateForFirstSearchField();
	
	return B_OK;
}


status_t
TAttributeSearchColumn::GetRequiredSize(BSize* size) const
{
	if (size == NULL)
		return B_BAD_VALUE;
	
	if (fSearchFields.CountItems() == 0)
		return B_ENTRY_NOT_FOUND;
	
	float height = 0.0f;
	BSize searchFieldSize;
	status_t error;
	if ((error = fSearchFields.ItemAt(0)->GetRequiredSize(&searchFieldSize)) != B_OK)
		return error;

	int32 numberOfSearchFields = fSearchFields.CountItems();
	const int32 PADDING_BETWEEN_SEARCH_FIELDS = 3.0f;
	const int32 OVERALL_PADDING = 4.0f;
	height += numberOfSearchFields * searchFieldSize.Height();
	height += numberOfSearchFields * PADDING_BETWEEN_SEARCH_FIELDS;
	height += OVERALL_PADDING;
	
	*size = BSize(B_SIZE_UNSET, height);
	return B_OK;
}


status_t
TAttributeSearchColumn::SetBoxSize(BSize size)
{
	if (size.Height() < 0)
		return B_BAD_VALUE;
	
	fBox->ResizeTo(size);
	fBox->SetExplicitSize(size);
	fBox->SetExplicitPreferredSize(size);
	return B_OK;
}


status_t
TAttributeSearchColumn::GetPredicateString(BString* predicateString) const
{
	// Must be implemented by derived class!
	return B_OK;
}


void
TAttributeSearchColumn::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kAddSearchField:
		{
			TAttributeSearchField* source = NULL;
		
			status_t error;
			if ((error = message->FindPointer("pointer", reinterpret_cast<void**>(&source)))
				!= B_OK) {
				source = NULL;
			}
			
			AddSearchField(source);
			BMessenger(fFindPanel).SendMessage(kResizeHeight);
			break;
		}
		
		case kRemoveSearchField:
		{
			TAttributeSearchField* source = NULL;
			status_t error;
			if ((error = message->FindPointer("pointer", reinterpret_cast<void**>(&source)))
				!= B_OK) {
				source = NULL;
			}
			
			RemoveSearchField(source);
			BMessenger(fFindPanel).SendMessage(kResizeHeight);
			break;
		}
		
		default:
		{
			_inherited::MessageReceived(message);
			break;
		}
	}
}


TNumericAttributeSearchColumn::TNumericAttributeSearchColumn(BColumn* column,
	BColumnTitle* columnTitle, TFindPanel* findPanel)
	:
	TAttributeSearchColumn(column, columnTitle, findPanel)
{
}


TNumericAttributeSearchColumn::~TNumericAttributeSearchColumn()
{
}


TAttributeSearchField*
TNumericAttributeSearchColumn::CreateAttributeSearchField()
{
	TNumericAttributeSearchField* searchField = new TNumericAttributeSearchField(this);
	return searchField;
}


status_t
TNumericAttributeSearchColumn::GetPredicateString(BString* predicateString) const
{
	if (predicateString == NULL)
		return B_BAD_VALUE;
	
	BString attributeName = fColumn->AttrName();
	attributeName.ReplaceFirst("_stat/", "");
	
	BString columnPredicateString = "";
	int32 numberOfSearchFields = fSearchFields.CountItems();
	for (int32 i = 0; i < numberOfSearchFields; ++i) {
		TAttributeSearchField* searchField = fSearchFields.ItemAt(i);
		BString searchFieldText(searchField->TextControl()->Text());
		if (searchFieldText == "")
			continue;
		
		if (i != 0) {
			status_t error;
			query_op combinationOperator;
			if ((error = searchField->GetCombinationOperator(&combinationOperator)) != B_OK)
				return error;
			
			BString temporary = "(";
			temporary.Append(columnPredicateString);
			columnPredicateString = temporary;
			
			if (combinationOperator == B_AND)
				columnPredicateString.Append("&&");
			else
				columnPredicateString.Append("||");
		}
		
		BQuery searchFieldPredicateStringGenerator;
		
		status_t error;
		if ((error = searchFieldPredicateStringGenerator.PushAttr(attributeName.String())) != B_OK)
			return error;
		
		int32 value = atoi(searchFieldText.String());
		if ((error = searchFieldPredicateStringGenerator.PushInt32(value)) != B_OK)
			return error;
		
		query_op attributeOperator;
		if ((error = searchField->GetAttributeOperator(&attributeOperator)) != B_OK)
			return error;
		if ((error = searchFieldPredicateStringGenerator.PushOp(attributeOperator)) != B_OK)
			return error;
		
		BString searchFieldPredicateString;
		if ((error = 
				searchFieldPredicateStringGenerator.GetPredicate(&searchFieldPredicateString))
			!= B_OK)
			return error;
		
		columnPredicateString.Append(searchFieldPredicateString);
		if (i != 0)
			columnPredicateString.Append(")");
	}
	
	*predicateString = columnPredicateString;
	return B_OK;
}


TTemporalAttributeSearchColumn::TTemporalAttributeSearchColumn(BColumn* column,
	BColumnTitle* columnTitle, TFindPanel* findPanel)
	:
	TAttributeSearchColumn(column, columnTitle, findPanel)
{
}


TTemporalAttributeSearchColumn::~TTemporalAttributeSearchColumn()
{
}


TAttributeSearchField*
TTemporalAttributeSearchColumn::CreateAttributeSearchField()
{
	TTemporalAttributeSearchField* searchField = new TTemporalAttributeSearchField(this);
	return searchField;
}


status_t
TTemporalAttributeSearchColumn::GetPredicateString(BString* predicateString) const
{
	if (predicateString == NULL)
		return B_BAD_VALUE;
	
	BString attributeName = fColumn->AttrName();
	attributeName.ReplaceFirst("_stat/", "");
	
	BString columnPredicateString = "";
	int32 numberOfSearchFields = fSearchFields.CountItems();
	for (int32 i = 0; i < numberOfSearchFields; ++i) {
		TAttributeSearchField* searchField = fSearchFields.ItemAt(i);
		BString searchFieldText(searchField->TextControl()->Text());
		if (searchFieldText == "")
			continue;
		
		if (i != 0) {
			status_t error;
			query_op combinationOperator;
			if ((error = searchField->GetCombinationOperator(&combinationOperator)) != B_OK)
				return error;
			
			BString temporary = "(";
			temporary.Append(columnPredicateString);
			columnPredicateString = temporary;
			
			if (combinationOperator == B_AND)
				columnPredicateString.Append("&&");
			else
				columnPredicateString.Append("||");
		}
		
		BQuery searchFieldPredicateStringGenerator;
		
		status_t error;
		if ((error = searchFieldPredicateStringGenerator.PushAttr(attributeName.String())) != B_OK)
			return error;
		
		if ((error = searchFieldPredicateStringGenerator.PushDate(searchFieldText.String()))
			!= B_OK) {
			return error;
		}
		
		query_op attributeOperator;
		if ((error = searchField->GetAttributeOperator(&attributeOperator)) != B_OK)
			return error;
		if ((error = searchFieldPredicateStringGenerator.PushOp(attributeOperator)) != B_OK)
			return error;
		
		BString searchFieldPredicateString;
		if ((error = 
				searchFieldPredicateStringGenerator.GetPredicate(&searchFieldPredicateString))
			!= B_OK)
			return error;
		
		columnPredicateString.Append(searchFieldPredicateString);
		if (i != 0)
			columnPredicateString.Append(")");
	}
	*predicateString = columnPredicateString;
	return B_OK;
}


TStringAttributeSearchColumn::TStringAttributeSearchColumn(BColumn* column,
	BColumnTitle* columnTitle, TFindPanel* findPanel)
	:
	TAttributeSearchColumn(column, columnTitle, findPanel)
{
}


TStringAttributeSearchColumn::~TStringAttributeSearchColumn()
{
}


TAttributeSearchField*
TStringAttributeSearchColumn::CreateAttributeSearchField()
{
	TStringAttributeSearchField* searchField = new TStringAttributeSearchField(this);
	return searchField;
}


status_t
TStringAttributeSearchColumn::GetPredicateString(BString* predicateString) const
{
	if (predicateString == NULL)
		return B_BAD_VALUE;
	
	BString attributeName = fColumn->AttrName();
	attributeName.ReplaceFirst("_stat/", "");
	
	BString columnPredicateString = "";
	int32 numberOfSearchFields = fSearchFields.CountItems();
	for (int32 i = 0; i < numberOfSearchFields; ++i) {
		TAttributeSearchField* searchField = fSearchFields.ItemAt(i);
		BString searchFieldText(searchField->TextControl()->Text());
		if (searchFieldText == "")
			continue;
		
		if (i != 0 && columnPredicateString != "") {
			status_t error;
			query_op combinationOperator;
			if ((error = searchField->GetCombinationOperator(&combinationOperator)) != B_OK)
				return error;
			
			BString temporary = "(";
			temporary.Append(columnPredicateString);
			columnPredicateString = temporary;
			
			if (combinationOperator == B_AND)
				columnPredicateString.Append("&&");
			else
				columnPredicateString.Append("||");
		}
		
		BQuery searchFieldPredicateStringGenerator;
		
		status_t error;
		if ((error = searchFieldPredicateStringGenerator.PushAttr(attributeName.String())) != B_OK)
			return error;
		
		if ((error = searchFieldPredicateStringGenerator.PushString(searchFieldText.String(), true))
			!= B_OK) {
			return error;
		}
		
		query_op attributeOperator;
		if ((error = searchField->GetAttributeOperator(&attributeOperator)) != B_OK)
			return error;
		if ((error = searchFieldPredicateStringGenerator.PushOp(attributeOperator)) != B_OK)
			return error;
		
		BString searchFieldPredicateString;
		if ((error = 
				searchFieldPredicateStringGenerator.GetPredicate(&searchFieldPredicateString))
			!= B_OK)
			return error;
		
		columnPredicateString.Append(searchFieldPredicateString);
		if (i != 0)
			columnPredicateString.Append(")");
	}
	*predicateString = columnPredicateString;
	return B_OK;
}


TDisabledSearchColumn::TDisabledSearchColumn(BColumn* column, BColumnTitle* columnTitle,
	TFindPanel* findPanel)
	:
	TAttributeColumn(column, columnTitle, findPanel)
{
	BBox* box = new BBox("container-box");
	BGroupView* view = new BGroupView(B_VERTICAL, 0.0f);
	BLayoutBuilder::Group<>(view)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(new BStringView("label", B_TRANSLATE("Can't be queried!")))
			.AddGlue()
		.End()
	.End();
	box->AddChild(view);
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.Add(box)
	.End();
}


TDisabledSearchColumn::~TDisabledSearchColumn()
{
}

}
