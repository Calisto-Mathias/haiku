#ifndef T_FIND_PANEL_CONSTANTS_H
#define T_FIND_PANEL_CONSTANTS_H

#include <Query.h>

#include "Catalog.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Find Panel"

namespace BPrivate {

static const char* combinationOperators[] = {
	B_TRANSLATE_MARK("and"),
	B_TRANSLATE_MARK("or"),
};

static const int32 combinationOperatorsLength = sizeof(combinationOperators) / sizeof(
	combinationOperators[0]);

static const query_op combinationQueryOps[] = {
	B_AND,
	B_OR
};

static const char* stringAttributeOperators[] = {
	B_TRANSLATE_MARK("contains"),
	B_TRANSLATE_MARK("is"),
	B_TRANSLATE_MARK("is not"),
	B_TRANSLATE_MARK("starts with"),
	B_TRANSLATE_MARK("ends with")
};

static const query_op stringQueryOps[] = {
	B_CONTAINS,
	B_EQ,
	B_NE,
	B_BEGINS_WITH,
	B_ENDS_WITH
};

static const int32 stringAttributeOperatorsLength = sizeof(stringAttributeOperators) / 
	sizeof(stringAttributeOperators[0]);

static const char* numericAttributeOperators[] = {
	B_TRANSLATE_MARK("greater than"),
	B_TRANSLATE_MARK("less than"),
	B_TRANSLATE_MARK("is")
};

static const query_op numericQueryOps[] = {
	B_GT,
	B_LT,
	B_EQ
};

static const int32 numericAttributeOperatorsLength = sizeof(numericAttributeOperators) / sizeof(
	numericAttributeOperators[0]);

static const char* temporalAttributeOperators[] = {
	B_TRANSLATE_MARK("before"),
	B_TRANSLATE_MARK("after")
};

static const query_op temporalQueryOps[] = {
	B_LT,
	B_GT
};

static const int32 temporalAttributeOperatorsLength = sizeof(temporalAttributeOperators) /
	sizeof(temporalAttributeOperators[0]);

enum class AttributeType
{
	STRING,
	NUMERIC,
	TEMPORAL
};

// Message static constants used by TFindPanel.cpp
static const uint32 kPauseOrSearch = 'Fbnp';
static const uint32 kTVolumeItem = 'Fvol';
static const uint32 kAddColumn = 'Facl';
static const uint32 kRemoveColumn = 'Frcl';
static const uint32 kMoveColumn = 'Fmcl';
static const uint32 kResizeColumn = 'Frsc';
static const uint32 kRefreshQueryResults = 'Frqr';
static const uint32 kResizeColumnsContainer = 'Frcc';
static const uint32 kPauseSearchResults = 'Fpsr';

// Message static constants used by TAttributeColumn.cpp
static const uint32 kResizeHeight = 'Frht';

// Message static constants used by TAttributeSearchField.cpp
static const uint32 kAddSearchField = 'Fasf';
static const uint32 kRemoveSearchField = 'Frsf';
static const uint32 kMenuOptionClicked = 'Focl';

// Message static constants used by QueryColumnResizeState
static const uint32 kRefreshColumns ='Frfc';

// Message static constants used by QueryPoseView.cpp
static const uint32 kResetPauseButton = 'Frpb';
static const uint32 kScrollView = 'Fscv';

}

#endif