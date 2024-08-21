#ifndef T_ATTRIBUTE_COLUMN_H
#define T_ATTRIBUTE_COLUMN_H


#include <ObjectList.h>
#include <View.h>


class BBox;
class BGroupView;
class BMenu;

namespace BPrivate {

enum class AttributeType;
class BColumn;
class BColumnTitle;
class TAttributeSearchField;
class TFindPanel;

class TAttributeColumn : public BView
{
public:
	virtual						~TAttributeColumn();

			status_t			GetSize(BSize*) const;
			status_t			SetSize(BSize);
			
			status_t			GetColumnTitle(BColumnTitle**) const;
			status_t			SetColumnTitle(BColumnTitle**);
			
			status_t			GetColumn(BColumn**) const;
			status_t			SetColumn(BColumn**);

protected:
								TAttributeColumn(BColumn*, BColumnTitle*, TFindPanel*);
	virtual	void				MessageReceived(BMessage*);

private:
			status_t			HandleColumnResize(float height);
			status_t			HandleColumnWidthResize();
			status_t			HandleColumnMoved();

protected:
			BColumn*			fColumn;
			BColumnTitle*		fColumnTitle;
			BSize				fSize;
			
			TFindPanel* 		fFindPanel;
			
			typedef BView _inherited;
};


class TAttributeSearchColumn : public TAttributeColumn
{
public:
	static	TAttributeSearchColumn*	CreateSearchColumnForAttributeType(const AttributeType,
		BColumn*, BColumnTitle*, TFindPanel*);

	virtual						~TAttributeSearchColumn();

	virtual	TAttributeSearchField*	CreateAttributeSearchField();
			status_t			ResetStateForFirstSearchField();

			status_t			AddSearchField(const int32 index = -1);
			status_t			AddSearchField(const TAttributeSearchField* after);
			status_t			RemoveSearchField(TAttributeSearchField*);

			status_t			GetRequiredSize(BSize*) const;
	virtual	status_t			GetPredicateString(BString*) const;

protected:
								TAttributeSearchColumn(BColumn*, BColumnTitle*, TFindPanel*);
	virtual	void				MessageReceived(BMessage*);

private:
			status_t			SetBoxSize(const BSize);

protected:
			BObjectList<TAttributeSearchField>	fSearchFields;

private:
			BGroupView*			fContainerView;
			BBox*				fBox;

			typedef TAttributeColumn _inherited;
};


class TNumericAttributeSearchColumn : public TAttributeSearchColumn
{
public:
								TNumericAttributeSearchColumn(BColumn*, BColumnTitle*,
									TFindPanel*);
	virtual						~TNumericAttributeSearchColumn();

	virtual	TAttributeSearchField*	CreateAttributeSearchField();
	virtual	status_t			GetPredicateString(BString*) const;
};


class TStringAttributeSearchColumn : public TAttributeSearchColumn
{
public:
								TStringAttributeSearchColumn(BColumn*, BColumnTitle*,
									TFindPanel*);
	virtual						~TStringAttributeSearchColumn();
	
	virtual	TAttributeSearchField*	CreateAttributeSearchField();
	virtual	status_t			GetPredicateString(BString*) const;
};


class TTemporalAttributeSearchColumn : public TAttributeSearchColumn
{
public:
								TTemporalAttributeSearchColumn(BColumn*, BColumnTitle*,
									TFindPanel*);
	virtual						~TTemporalAttributeSearchColumn();
	
	virtual	TAttributeSearchField*	CreateAttributeSearchField();
	virtual	status_t			GetPredicateString(BString*) const;
};


class TDisabledSearchColumn : public TAttributeColumn
{
public:
								TDisabledSearchColumn(BColumn*, BColumnTitle*, TFindPanel*);
	virtual						~TDisabledSearchColumn();

private:
			typedef TAttributeColumn _inherited;
};

}

#endif