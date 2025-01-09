#ifndef T_ATTRIBUTE_SEARCH_FIELD_H
#define T_ATTRIBUTE_SEARCH_FIELD_H


#include <View.h>
#include <Query.h>


class BButton;
class BMenu;
class BMenuItem;
class BMessage;
class BStringView;

namespace BPrivate {

class TAttributeSearchColumn;
class TNumericAttributeSearchColumn;
class TPopUpTextControl;
class TStringAttributeSearchColumn;
class TTemporalAttributeSearchColumn;

class TAttributeSearchField : public BView
{
public:
								TAttributeSearchField(TAttributeSearchColumn*);
	virtual						~TAttributeSearchField();

			status_t			GetAttributeOperator(query_op*) const;
			status_t			GetCombinationOperator(query_op*) const;
			
			status_t			EnableRemoveButton(bool enable = true);
			status_t			EnableAddButton(bool enable = true);
			
			status_t			GetRequiredSize(BSize*) const;
			status_t			SetSize(BSize);

			status_t			UpdateLabel();
			
			TPopUpTextControl*	TextControl() const { return fTextControl; };

protected:
	virtual	void				MessageReceived(BMessage*);
	virtual	void				AttachedToWindow();
	
	// Helper Functions that should be overriden by child classes
	virtual	status_t			PopulateOptionsMenu();
	virtual	query_op			GetAttributeOperatorFromIndex(int32) const;
private:
			bool				IsCombinationOperatorMenuItem(BMenuItem*) const;
			status_t			HandleMenuOptionClicked(BMenuItem*);
			status_t			GetRequiredHeight(float*) const;

protected:
			TPopUpTextControl*	fTextControl;

private:
			BButton*			fAddButton;
			BButton*			fRemoveButton;
			BStringView*		fLabel;
			
			TAttributeSearchColumn*	fAttributeColumn;
			
			BSize				fSize;
			
			typedef BView _inherited;
};


class TNumericAttributeSearchField : public TAttributeSearchField
{
public:
								TNumericAttributeSearchField(TNumericAttributeSearchColumn*);
								~TNumericAttributeSearchField();

protected:
	virtual	status_t			PopulateOptionsMenu();
	virtual	query_op			GetAttributeOperatorFromIndex(int32) const;

private:
			typedef TAttributeSearchField _inherited;
};

class TTemporalAttributeSearchField : public TAttributeSearchField
{
public:
								TTemporalAttributeSearchField(TTemporalAttributeSearchColumn*);
								~TTemporalAttributeSearchField();

protected:
	virtual	status_t			PopulateOptionsMenu();
	virtual	query_op			GetAttributeOperatorFromIndex(int32) const;

private:
			typedef TAttributeSearchField _inherited;
};


class TStringAttributeSearchField : public TAttributeSearchField
{
public:
								TStringAttributeSearchField(TStringAttributeSearchColumn*);
								~TStringAttributeSearchField();

protected:
	virtual	status_t			PopulateOptionsMenu();
	virtual	query_op			GetAttributeOperatorFromIndex(int32) const;

private:
			typedef TAttributeSearchField _inherited;
};


}
#endif