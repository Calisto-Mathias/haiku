#include "TFindPanel.h"


#include <string>

#include <fs_attr.h>

#include <Button.h>
#include <ControlLook.h>
#include <Debug.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <Message.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <SeparatorView.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include "Attributes.h"
#include "Catalog.h"
#include "Commands.h"
#include "IconMenuItem.h"
#include "MimeTypes.h"
#include "MimeTypeList.h"
#include "Model.h"
#include "QueryContainerWindow.h"
#include "QueryPoseView.h"
#include "TAttributeColumn.h"
#include "TFindPanelConstants.h"
#include "Tracker.h"
#include "Utilities.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Find Panel"


namespace BPrivate {

static const char* kAllMimeTypes = "mimes/ALLTYPES";

class TMostUsedNames {
public:
								TMostUsedNames(const char* fileName, const char* directory,
									int32 maxCount = 5);
								~TMostUsedNames();

			bool				ObtainList(BList* list);
			void				ReleaseList();

			void 				AddName(const char*);

protected:
			struct list_entry {
				char* name;
				int32 count;
			};

		static int CompareNames(const void* a, const void* b);
		void LoadList();
		void UpdateList();

		const char*	fFileName;
		const char*	fDirectory;
		bool		fLoaded;
		mutable Benaphore fLock;
		BList		fList;
		int32		fCount;
};

TMostUsedNames gTMostUsedMimeTypes("MostUsedMimeTypes", "Tracker");


TFindPanel::TFindPanel(BQueryContainerWindow* window, BQueryPoseView* poseView)
	:
	BView("incremental-find-panel", B_WILL_DRAW),
	fButton(new BButton(B_TRANSLATE("Pause"), new BMessage(kPauseOrSearch))),
	fColumnsContainer(new BView("columns-container", B_WILL_DRAW)),
	fQueryContainerWindow(window),
	fQueryPoseView(poseView)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	SetLowUIColor(ViewUIColor());
	
	SetTemporaryFileHandle();
	BuildMimeTypeMenu();
	BuildVolumeMenu();
	BuildFindPanelLayout();
	ResizeMenuFields();
}


status_t
CreateTemporaryDirectory(BPath* parentDirectoryPath)
{
	if (parentDirectoryPath == NULL)
		return B_BAD_VALUE;

	BDirectory parentDirectory(parentDirectoryPath->Path());
	status_t error = parentDirectory.InitCheck();
	if (error != B_OK)
		return error;
	
	error = parentDirectory.CreateDirectory("temporary", NULL);
	return error;
}


status_t
GetTemporaryFilePath(BPath* filePath)
{
	if (filePath == NULL)
		return B_BAD_VALUE;
	
	status_t error;
	
	BPath homeDirectoryPath;
	if (find_directory(B_USER_DIRECTORY, &homeDirectoryPath) != B_OK)
		homeDirectoryPath.SetTo("/boot/home");
	
	BPath queriesDirectoryPath = homeDirectoryPath;
	queriesDirectoryPath.Append("queries");
	if ((error = queriesDirectoryPath.InitCheck()) != B_OK)
		return error;
	
	BPath temporaryQueriesDirectoryPath = queriesDirectoryPath;
	temporaryQueriesDirectoryPath.Append("temporary");
	if (BEntry(temporaryQueriesDirectoryPath.Path()).Exists() == false) {
		error = CreateTemporaryDirectory(&queriesDirectoryPath);
	}

	if (error != B_OK)
		return error;
	
	BDirectory temporaryQueriesDirectory(temporaryQueriesDirectoryPath.Path());
	if ((error = temporaryQueriesDirectory.InitCheck()) != B_OK)
		return error;

	int32 numberOfFiles = temporaryQueriesDirectory.CountEntries();

	BString temporaryFileName = "temporary";
	temporaryFileName.Append(std::to_string(numberOfFiles).c_str());

	BPath temporaryFilePath = temporaryQueriesDirectoryPath;
	if ((error = temporaryFilePath.InitCheck()) != B_OK)
		return error;
	
	temporaryFilePath.Append(temporaryFileName.String());
	*filePath = temporaryFilePath;
	return B_OK;
}


status_t
TFindPanel::SetTemporaryFileHandle()
{
	BPath temporaryFilePath;
	status_t error;
	
	if ((error = GetTemporaryFilePath(&temporaryFilePath)) != B_OK)
		return error;
	
	fFileEntry = new BEntry(temporaryFilePath.Path());
	if ((error = fFileEntry->InitCheck()) != B_OK) {
		return error;
	}
	fFile = new BFile(temporaryFilePath.Path(), B_CREATE_FILE | B_READ_WRITE);
	if ((error = fFile->InitCheck()) != B_OK)
		return error;
	
	if ((error = BNodeInfo(fFile).SetType(B_QUERY_MIMETYPE)) != B_OK)
		return error;
	
	return B_OK;
}


void
TFindPanel::BuildMimeTypeMenu()
{
	fMimeTypeMenu = new BPopUpMenu("MimeTypeMenu");
	fMimeTypeMenu->SetRadioMode(true);
	AddMimeTypesToMenu(fMimeTypeMenu);
	fMimeTypeField = new BMenuField("MimeTypeMenu", "", fMimeTypeMenu);
	fMimeTypeField->SetDivider(0.0f);
	fMimeTypeField->MenuItem()->SetLabel(B_TRANSLATE("All files and folders"));
}


static void
PopUpMenuSetTitle(BMenu* menu, const char* title)
{
	// This should really be in BMenuField
	BMenu* bar = menu->Supermenu();
	
	ASSERT(bar);
	ASSERT(bar->ItemAt(0));
	if (bar == NULL || !bar->ItemAt(0))
		return;
	
	bar->ItemAt(0)->SetLabel(title);
}


void
TFindPanel::AddVolumes(BMenu* menu)
{
	// ToDo: add calls to this to rebuild the menu when a volume gets mounted

	BMessage* message = new BMessage(kTVolumeItem);
	message->AddInt32("device", -1);
	menu->AddItem(new BMenuItem(B_TRANSLATE("All disks"), message));
	menu->AddSeparatorItem();
	PopUpMenuSetTitle(menu, B_TRANSLATE("All disks"));

	BVolumeRoster roster;
	BVolume volume;
	roster.Rewind();
	while (roster.GetNextVolume(&volume) == B_OK) {
		if (volume.IsPersistent() && volume.KnowsQuery()) {
			BDirectory root;
			if (volume.GetRootDirectory(&root) != B_OK)
				continue;

			BEntry entry;
			root.GetEntry(&entry);

			Model model(&entry, true);
			if (model.InitCheck() != B_OK)
				continue;

			message = new BMessage(kTVolumeItem);
			message->AddInt32("device", volume.Device());
			
			menu->AddItem(new ModelMenuItem(&model, model.Name(), message));
		}
	}

	if (menu->ItemAt(0))
		menu->ItemAt(0)->SetMarked(true);
}


void
TFindPanel::BuildVolumeMenu()
{
	fVolMenu = new BPopUpMenu("VolumeMenu", false, false);
	fVolumeField = new BMenuField("", B_TRANSLATE("In"), fVolMenu);
	fVolumeField->SetDivider(fVolumeField->StringWidth(fVolumeField->Label()) + 8);
	AddVolumes(fVolMenu);
}


void
TFindPanel::BuildFindPanelLayout()
{
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(0, B_USE_WINDOW_SPACING, 0, 0)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.AddGlue()
			.Add(fMimeTypeField)
			.Add(fVolumeField)
			.Add(fButton)
			.AddGlue()
		.End()
		.Add(new BSeparatorView())
		.Add(fColumnsContainer)
	.End();
}


void
TFindPanel::ResizeMenuFields()
{
	TFindPanel::ResizeMenuField(fMimeTypeField);
	TFindPanel::ResizeMenuField(fVolumeField);
}


TFindPanel::~TFindPanel()
{
	if (fFileEntry != NULL)
		fFileEntry->Remove();
	
	fFileEntry->Unset();
	delete fFileEntry;
	
	fFile->Unset();
	delete fFile;
}


void
TFindPanel::AttachedToWindow()
{
	BMessenger target(this);
	fVolMenu->SetTargetForItems(this);
	
	int32 count = fMimeTypeMenu->CountItems();
	for (int32 i = 0; i < count; ++i) {
		BMenuItem* item = fMimeTypeMenu->ItemAt(i);
		BMenu* subMenu = item->Submenu();
		if (subMenu == NULL)
			item->SetTarget(target);
		else
			subMenu->SetTargetForItems(target);
	}
	
	fButton->SetTarget(target);
}


void
TFindPanel::ResizeMenuField(BMenuField* menuField)
{
	BSize size;
	menuField->GetPreferredSize(&size.width, &size.height);

	BMenu* menu = menuField->Menu();

	float padding = 0.0f;
	float width = 0.0f;

	BMenuItem* markedItem = menu->FindMarked();
	if (markedItem != NULL) {
		if (markedItem->Submenu() != NULL) {
			BMenuItem* markedSubItem = markedItem->Submenu()->FindMarked();
			if (markedSubItem != NULL && markedSubItem->Label() != NULL) {
				float labelWidth
					= menuField->StringWidth(markedSubItem->Label());
				padding = size.width - labelWidth;
			}
		} else if (markedItem->Label() != NULL) {
			float labelWidth = menuField->StringWidth(markedItem->Label());
			padding = size.width - labelWidth;
		}
	}

	for (int32 index = menu->CountItems(); index-- > 0; ) {
		BMenuItem* item = menu->ItemAt(index);
		if (item->Label() != NULL)
			width = std::max(width, menuField->StringWidth(item->Label()));

		BMenu* submenu = item->Submenu();
		if (submenu != NULL) {
			for (int32 subIndex = submenu->CountItems(); subIndex-- > 0; ) {
				BMenuItem* subItem = submenu->ItemAt(subIndex);
				if (subItem->Label() == NULL)
					continue;

				width = std::max(width,
					menuField->StringWidth(subItem->Label()));
			}
		}
	}

	width = std::max(width, menuField->StringWidth(B_TRANSLATE("Multiple selections")));

	float maxWidth = be_control_look->DefaultItemSpacing() * 20;
	size.width = std::min(width + padding, maxWidth);
	menuField->SetExplicitSize(size);
}


status_t
TFindPanel::SetCurrentMimeType(BMenuItem* item)
{
	// unmark old MIME type (in most used list, and the tree)

	BMenuItem* marked = CurrentMimeType();
	if (marked != NULL) {
		marked->SetMarked(false);

		if ((marked = MimeTypeMenu()->FindMarked()) != NULL)
			marked->SetMarked(false);
	}

	// mark new MIME type (in most used list, and the tree)

	if (item != NULL) {
		item->SetMarked(true);
		fMimeTypeField->MenuItem()->SetLabel(item->Label());

		BMenuItem* search;
		for (int32 i = 2; (search = MimeTypeMenu()->ItemAt(i)) != NULL; i++) {
			if (item == search || search->Label() == NULL)
				continue;

			if (strcmp(item->Label(), search->Label()) == 0) {
				search->SetMarked(true);
				break;
			}

			BMenu* submenu = search->Submenu();
			if (submenu == NULL)
				continue;

			for (int32 j = submenu->CountItems(); j-- > 0;) {
				BMenuItem* sub = submenu->ItemAt(j);
				if (strcmp(item->Label(), sub->Label()) == 0) {
					sub->SetMarked(true);
					break;
				}
			}
		}
	}

	return B_OK;
}


status_t
TFindPanel::SetCurrentMimeType(const char* label)
{
	// unmark old MIME type (in most used list, and the tree)

	BMenuItem* marked = CurrentMimeType();
	if (marked != NULL) {
		marked->SetMarked(false);

		if ((marked = MimeTypeMenu()->FindMarked()) != NULL)
			marked->SetMarked(false);
	}

	// mark new MIME type (in most used list, and the tree)

	fMimeTypeField->MenuItem()->SetLabel(label);
	bool found = false;

	for (int32 index = MimeTypeMenu()->CountItems(); index-- > 0;) {
		BMenuItem* item = MimeTypeMenu()->ItemAt(index);
		BMenu* submenu = item->Submenu();
		if (submenu != NULL && !found) {
			for (int32 subIndex = submenu->CountItems(); subIndex-- > 0;) {
				BMenuItem* subItem = submenu->ItemAt(subIndex);
				if (subItem->Label() != NULL
					&& strcmp(label, subItem->Label()) == 0) {
					subItem->SetMarked(true);
					found = true;
				}
			}
		}

		if (item->Label() != NULL && strcmp(label, item->Label()) == 0) {
			item->SetMarked(true);
			return B_OK;
		}
	}

	return found ? B_OK : B_ENTRY_NOT_FOUND;
}


BMenuItem*
TFindPanel::CurrentMimeType(const char** type) const
{
	// search for marked item in the list
	BMenuItem* item = MimeTypeMenu()->FindMarked();

	if (item != NULL && MimeTypeMenu()->IndexOf(item) != 0
		&& item->Submenu() == NULL) {
		// if it's one of the most used items, ignore it
		item = NULL;
	}

	if (item == NULL) {
		for (int32 index = MimeTypeMenu()->CountItems(); index-- > 0;) {
			BMenu* submenu = MimeTypeMenu()->ItemAt(index)->Submenu();
			if (submenu != NULL && (item = submenu->FindMarked()) != NULL)
				break;
		}
	}

	if (type != NULL && item != NULL) {
		BMessage* message = item->Message();
		if (message == NULL)
			return NULL;

		if (message->FindString("mimetype", type) != B_OK)
			return NULL;
	}
	return item;
}


static
void AddSubtype(BString& text, const BMimeType& type)
{
	text.Append(" (");
	text.Append(strchr(type.Type(), '/') + 1);
		// omit the slash
	text.Append(")");
}


bool
TFindPanel::AddOneMimeTypeToMenu(const ShortMimeInfo* info, void* castToMenu)
{
	BPopUpMenu* menu = static_cast<BPopUpMenu*>(castToMenu);

	BMimeType type(info->InternalName());
	BMimeType super;
	type.GetSupertype(&super);
	if (super.InitCheck() < B_OK)
		return false;

	BMenuItem* superItem = menu->FindItem(super.Type());
	if (superItem != NULL) {
		BMessage* message = new BMessage(kMIMETypeItem);
		message->AddString("mimetype", info->InternalName());

		// check to ensure previous item's name differs
		BMenu* menu = superItem->Submenu();
		BMenuItem* previous = menu->ItemAt(menu->CountItems() - 1);
		BString text = info->ShortDescription();
		if (previous != NULL
			&& strcasecmp(previous->Label(), info->ShortDescription()) == 0) {
			AddSubtype(text, type);

			// update the previous item as well
			BMimeType type(previous->Message()->GetString("mimetype", NULL));
			BString label = ShortMimeInfo(type).ShortDescription();
			AddSubtype(label, type);
			previous->SetLabel(label.String());
		}

		menu->AddItem(new IconMenuItem(text.String(), message,
			info->InternalName()));
	}

	return false;
}


void
TFindPanel::AddMimeTypesToMenu(BPopUpMenu* menu)
{
	BMessage* itemMessage = new BMessage(kMIMETypeItem);
	itemMessage->AddString("mimetype", kAllMimeTypes);

	IconMenuItem* firstItem = new IconMenuItem(
		B_TRANSLATE("All files and folders"), itemMessage,
		static_cast<BBitmap*>(NULL));
	MimeTypeMenu()->AddItem(firstItem);
	MimeTypeMenu()->AddSeparatorItem();

	// add recent MIME types

	TTracker* tracker = dynamic_cast<TTracker*>(be_app);
	ASSERT(tracker != NULL);

	BList list;
	if (tracker != NULL && gTMostUsedMimeTypes.ObtainList(&list)) {
		int32 count = 0;
		for (int32 index = 0; index < list.CountItems(); index++) {
			const char* name = (const char*)list.ItemAt(index);

			MimeTypeList* mimeTypes = tracker->MimeTypes();
			if (mimeTypes != NULL) {
				const ShortMimeInfo* info = mimeTypes->FindMimeType(name);
				if (info == NULL)
					continue;

				BMessage* message = new BMessage(kMIMETypeItem);
				message->AddString("mimetype", info->InternalName());

				MimeTypeMenu()->AddItem(new BMenuItem(name, message));
				count++;
			}
		}
		if (count != 0)
			MimeTypeMenu()->AddSeparatorItem();

		gTMostUsedMimeTypes.ReleaseList();
	}

	// add MIME type tree list

	BMessage types;
	if (BMimeType::GetInstalledSupertypes(&types) == B_OK) {
		const char* superType;
		int32 index = 0;

		while (types.FindString("super_types", index++, &superType) == B_OK) {
			BMenu* superMenu = new BMenu(superType);

			BMessage* message = new BMessage(kMIMETypeItem);
			message->AddString("mimetype", superType);

			MimeTypeMenu()->AddItem(new IconMenuItem(superMenu, message,
				superType));

			// the MimeTypeMenu's font is not correct at this time
			superMenu->SetFont(be_plain_font);
		}
	}

	if (tracker != NULL) {
		tracker->MimeTypes()->EachCommonType(
			&TFindPanel::AddOneMimeTypeToMenu, MimeTypeMenu());
	}

	// remove empty super type menus (and set target)

	for (int32 index = MimeTypeMenu()->CountItems(); index-- > 2;) {
		BMenuItem* item = MimeTypeMenu()->ItemAt(index);
		BMenu* submenu = item->Submenu();
		if (submenu == NULL)
			continue;

		if (submenu->CountItems() == 0) {
			MimeTypeMenu()->RemoveItem(item);
			delete item;
		} else
			submenu->SetTargetForItems(this);
	}
}


void
TFindPanel::PushMimeType(BQuery* query) const
{
	const char* type;
	if (CurrentMimeType(&type) == NULL)
		return;


	if (strcmp(kAllMimeTypes, type)) {
		// add an asterisk if we are searching for a supertype
		char buffer[B_FILE_NAME_LENGTH];
		if (strchr(type, '/') == NULL) {
			strlcpy(buffer, type, sizeof(buffer));
			strlcat(buffer, "/*", sizeof(buffer));
			type = buffer;
		}

		query->PushAttr(kAttrMIMEType);
		query->PushString(type);
		query->PushOp(B_EQ);
	}
}


status_t
TFindPanel::GetMaximumHeightOfColumns(float* height) const
{
	if (height == NULL)
		return B_BAD_VALUE;
	
	float heightOfColumns = 0.0f;
	int32 numberOfColumns = fColumnsContainer->CountChildren();
	for (int32 i = 0; i < numberOfColumns; ++i) {
		TAttributeSearchColumn* searchColumn = dynamic_cast<TAttributeSearchColumn*>(
			fColumnsContainer->ChildAt(i));
		
		if (searchColumn == NULL)
			continue;
		
		BSize size;
		status_t error;
		if ((error = searchColumn->GetRequiredSize(&size)) != B_OK)
			return error;
		
		if (size.height > heightOfColumns)
			heightOfColumns = size.height;
	}
	
	*height = heightOfColumns;
	return B_OK;
}


status_t
TFindPanel::GetRequiredWidthOfColumnsContainer(float* width) const
{
	if (width == NULL)
		return B_BAD_VALUE;
	
	int32 count = fColumnsContainer->CountChildren();
	TAttributeColumn* lastColumn = dynamic_cast<TAttributeColumn*>(
		fColumnsContainer->ChildAt(count-1));
	
	BRect boundingRectangle = lastColumn->Frame();
	*width = boundingRectangle.right;
	return B_OK;
}


status_t
TFindPanel::HandleResizingColumnsContainer()
{
	float heightOfColumnsContainer;
	status_t error = GetMaximumHeightOfColumns(&heightOfColumnsContainer);
	if (error != B_OK)
		return error;
	
	float widthOfColumnsContainer;
	if ((error = GetRequiredWidthOfColumnsContainer(&widthOfColumnsContainer)) != B_OK)
		return error;
	widthOfColumnsContainer += be_control_look->DefaultItemSpacing();
	
	BSize finalContainerSize(widthOfColumnsContainer, heightOfColumnsContainer);
	finalContainerSize.height += be_control_look->DefaultItemSpacing();
	fColumnsContainer->SetExplicitMinSize(finalContainerSize);
	fColumnsContainer->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, finalContainerSize.Height()));
	fColumnsContainer->SetExplicitPreferredSize(finalContainerSize);
	fColumnsContainer->ResizeTo(finalContainerSize);
	
	int32 numberOfColumns = fColumnsContainer->CountChildren();
	for (int32 i = 0; i < numberOfColumns; ++i) {
		TAttributeColumn* column = dynamic_cast<TAttributeColumn*>(
			fColumnsContainer->ChildAt(i));
		if (column == NULL)
			continue;
		
		BMessage* message = new BMessage(kResizeHeight);
		message->AddFloat("height", heightOfColumnsContainer);
		BMessenger(column).SendMessage(message);
	}
	return B_OK;
}


status_t
SendMessageToColumn(TAttributeColumn* column, uint32 messageConstant)
{
	BMessage* message = new BMessage(messageConstant);
	return BMessenger(column).SendMessage(message);
}


status_t
TFindPanel::HandleMovingAColumn()
{
	int32 numberOfColumns = fColumnsContainer->CountChildren();
	for (int32 i = 0; i < numberOfColumns; ++i) {
		TAttributeColumn* column = dynamic_cast<TAttributeColumn*>(
			fColumnsContainer->ChildAt(i));
		if (column == NULL)
			continue;
		
		SendMessageToColumn(column, kMoveColumn);
	}
	
	return B_OK;
}


status_t
TFindPanel::HandleResizingColumns()
{
	int32 numberOfColumns = fColumnsContainer->CountChildren();
	for (int32 i = 0; i < numberOfColumns; ++i) {
		TAttributeColumn* column = dynamic_cast<TAttributeColumn*>(
			fColumnsContainer->ChildAt(i));
		
		if (column == NULL)
			continue;
		
		SendMessageToColumn(column, kResizeColumn);
	}
	
	return B_OK;
}


bool
ShouldSearchColumnBeAdded(BString label)
{
	const BString disabledColumnLabels[] = {
		BString("Real name"),
		BString("Created"),
		BString("Kind"),
		BString("Location"),
		BString("Permissions")
	};
	
	const int32 numberOfDisabledLabels = 
		sizeof(disabledColumnLabels)/sizeof(disabledColumnLabels[0]);
	
	for (int32 i = 0; i < numberOfDisabledLabels; ++i) {
		if (label == disabledColumnLabels[i])
			return false;
	}
	
	return true;
}


AttributeType
GetAttributeTypeFromInt(int32 type)
{
	if (type == B_OFF_T_TYPE || type == B_INT32_TYPE)
		return AttributeType::NUMERIC;
	else if (type == B_TIME_TYPE)
		return AttributeType::TEMPORAL;
	else
		return AttributeType::STRING;
}


status_t
TFindPanel::AddAttributeColumn(BColumn* column)
{
	if (column == NULL)
		return B_BAD_VALUE;
	
	BColumnTitle* titleView = fQueryPoseView->TitleView()->FindColumnTitle(column);
	if (titleView == NULL)
		return B_BAD_VALUE;
	
	if (ShouldSearchColumnBeAdded(column->Title())) {
		AttributeType type = GetAttributeTypeFromInt(column->AttrType());
		TAttributeSearchColumn* searchColumn = 
			TAttributeSearchColumn::CreateSearchColumnForAttributeType(type, column, titleView,
				this);
		fColumnsContainer->AddChild(searchColumn);
		return BMessenger(searchColumn).SendMessage(new BMessage(kAddSearchField));
	} else {
		TDisabledSearchColumn* disabledColumn = new TDisabledSearchColumn(column, titleView, this);
		fColumnsContainer->AddChild(disabledColumn);
		return B_OK;
	}
}


status_t
TFindPanel::RemoveAttributeColumn(BColumn* column)
{
	int32 columnsCount = fColumnsContainer->CountChildren();
	for (int32 i = 0; i < columnsCount; ++i) {
		TAttributeColumn* attributeColumn = dynamic_cast<TAttributeColumn*>(
			fColumnsContainer->ChildAt(i));
		BColumn* comparisonColumn = NULL;
		status_t error;
		if ((error = attributeColumn->GetColumn(&comparisonColumn)) != B_OK)
			return error;
		if (comparisonColumn == column) {
			attributeColumn->RemoveSelf();
			delete attributeColumn;
			break;
		}
	}
	HandleMovingAColumn();
	return B_OK;
}


status_t
TFindPanel::GetPredicateString(BString* predicateString) const
{
	if (predicateString == NULL)
		return B_BAD_VALUE;
	
	int32 numberOfColumns = fColumnsContainer->CountChildren();
	bool combinatorShouldBeAdded = false;
	
	BString predicateStringSetter = "";
	for (int32 i = 0; i < numberOfColumns; ++i) {
		TAttributeSearchColumn* searchColumn = dynamic_cast<TAttributeSearchColumn*>(
			fColumnsContainer->ChildAt(i));
		if (searchColumn == NULL)
			continue;
		
		
		BString columnPredicateString;
		searchColumn->GetPredicateString(&columnPredicateString);
		
		if (columnPredicateString == "")
			continue;

		// TODO: Add the other combinators
		if (combinatorShouldBeAdded)
			predicateStringSetter.Append("&&");
		else
			combinatorShouldBeAdded = true;

		predicateStringSetter.Append("(").Append(columnPredicateString).Append(")");
	}
	
	int32 length = predicateStringSetter.Length();
	if (predicateStringSetter.EndsWith("&&"))
		predicateStringSetter.Truncate(length - 2);
	
	*predicateString = predicateStringSetter;
	return B_OK;
}


status_t
TFindPanel::GetMimeTypeString(BString* mimeTypeString) const
{
	if (mimeTypeString == NULL)
		return B_BAD_VALUE;
	
	BString mimeTypeStringSetter;
	BQuery mimeQuery;
	PushMimeType(&mimeQuery);
	status_t error;
	if ((error = mimeQuery.GetPredicate(&mimeTypeStringSetter)) != B_OK)
		return error;
	*mimeTypeString = mimeTypeStringSetter;
	return B_OK;
}


status_t
ProcessPredicateString(BString* predicateString, BString* mimeTypeString)
{
	if (predicateString == NULL || mimeTypeString == NULL)
		return B_BAD_VALUE;
	
	if (*predicateString == "")
		*predicateString = "(name == \"**\")";
	
	if (*mimeTypeString != "" && *mimeTypeString != "(BEOS:TYPE==\"mime/ALLTYPES\")")
		predicateString->Append("&&").Append(*mimeTypeString);
	
	BString temp = "(";
	temp.Append(*predicateString).Append(")");
	*predicateString = temp;

	return B_OK;
}


static int32
GetNumberOfVolumes()
{
	static int32 numberOfVolumes = -1;
	if (numberOfVolumes >= 0)
		return numberOfVolumes;

	BVolumeRoster volumeRoster;
	BVolume volume;
	
	int32 count = 0;
	while (volumeRoster.GetNextVolume(&volume) == B_OK) {
		if (volume.IsPersistent() && volume.KnowsQuery() && volume.KnowsAttr())
			++count;
	}
	
	numberOfVolumes = count;
	return numberOfVolumes;
	
}


status_t
TFindPanel::WriteVolumesToFile()
{
	bool addAllVolumes = fVolMenu->ItemAt(0)->IsMarked();
	int32 numberOfVolumes = GetNumberOfVolumes();
	
	BMessage messageContainingVolumeInfo;
	for (int32 i = 2; i < numberOfVolumes + 2; ++i) {
		BMenuItem* volumeMenuItem = fVolMenu->ItemAt(i);
		BMessage* messageOfVolumeMenuItem = volumeMenuItem->Message();
		dev_t device;
		if (messageOfVolumeMenuItem->FindInt32("device", &device) != B_OK)
			continue;
		
		if (volumeMenuItem->IsMarked() || addAllVolumes) {
			BVolume volume(device);
			EmbedUniqueVolumeInfo(&messageContainingVolumeInfo, &volume);
		}
		
		ssize_t flattenedSize = messageContainingVolumeInfo.FlattenedSize();
		if (flattenedSize > 0) {
			BString bufferString;
			char* buffer = bufferString.LockBuffer(flattenedSize);
			messageContainingVolumeInfo.Flatten(buffer, flattenedSize);
			if (fFile->WriteAttr("_trk/qryvol1", B_MESSAGE_TYPE, 0, buffer,
					static_cast<size_t>(flattenedSize))
				!= flattenedSize) {
				return B_IO_ERROR;
			}
		}
	}
	
	return B_OK;
}


status_t
TFindPanel::SaveQueryAsAttributesToFile()
{
	if (fFile == NULL || fFileEntry == NULL)
		return B_BAD_VALUE;
	
	status_t error;
	if ((error = WriteVolumesToFile()) != B_OK)
		return error;
	
	BString predicateString;
	if ((error = GetPredicateString(&predicateString)) != B_OK)
		return error;

	BString mimeTypeString;
	if ((error = GetMimeTypeString(&mimeTypeString)) != B_OK)
		mimeTypeString = "";

	if (predicateString.EndsWith("&&"))
		predicateString.Truncate(predicateString.Length() - 2);

	ProcessPredicateString(&predicateString, &mimeTypeString);
	if ((error = WritePredicateStringToFile(&predicateString)) != B_OK)
		return error;

	return B_OK;
}


status_t
TFindPanel::WritePredicateStringToFile(BString* predicateString)
{
	if (predicateString == NULL)
		return B_BAD_VALUE;

	if (fFile == NULL || fFileEntry == NULL)
		return B_BAD_VALUE;
	
	return fFile->WriteAttrString("_trk/qrystr", predicateString);
}


status_t
TFindPanel::SendUpdateToPoseView()
{
	BMessenger messenger(fQueryPoseView);
	BMessage* message = new BMessage(kRefreshQueryResults);
	entry_ref ref;
	fFileEntry->GetRef(&ref);
	
	status_t error;
	if ((error = message->AddRef("refs", &ref)) != B_OK)
		return error;
	
	return messenger.SendMessage(message);
}


status_t
TFindPanel::SendPauseToPoseView()
{
	BMessenger messenger(fQueryPoseView);
	BMessage* message = new BMessage(kPauseSearchResults);
	return messenger.SendMessage(message);
}


void
TFindPanel::ShowVolumeMenuLabel()
{
	if (fVolMenu->ItemAt(0)->IsMarked()) {
		// "all disks" selected
		PopUpMenuSetTitle(fVolMenu, fVolMenu->ItemAt(0)->Label());
		return;
	}

	// find out if more than one items are marked
	int32 count = fVolMenu->CountItems();
	int32 countSelected = 0;
	BMenuItem* tmpItem = NULL;
	for (int32 index = 2; index < count; index++) {
		BMenuItem* item = fVolMenu->ItemAt(index);
		if (item->IsMarked()) {
			countSelected++;
			tmpItem = item;
		}
	}


	if (countSelected == 0) {
		// no disk selected, for now revert to search all disks
		// ToDo:
		// show no disks here and add a check that will not let the
		// query go if the user doesn't pick at least one
		fVolMenu->ItemAt(0)->SetMarked(true);
		PopUpMenuSetTitle(fVolMenu, fVolMenu->ItemAt(0)->Label());
	} else if (countSelected > 1)
		// if more than two disks selected, don't use the disk name
		// as a label
		PopUpMenuSetTitle(fVolMenu,	B_TRANSLATE("multiple disks"));
	else {
		ASSERT(tmpItem);
		PopUpMenuSetTitle(fVolMenu, tmpItem->Label());
	}
}


void
TFindPanel::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kAddColumn:
		{
			BColumn* column = NULL;
			if (message->FindPointer("pointer", reinterpret_cast<void**>(&column)) != B_OK
				|| column == NULL)
				break;

			AddAttributeColumn(column);
			HandleResizingColumnsContainer();
			break;
		}
	
		case kRemoveColumn:
		{
			BColumn* column = NULL;
			if (message->FindPointer("pointer", reinterpret_cast<void**>(&column)) != B_OK
				|| column == NULL)
				break;
			
			RemoveAttributeColumn(column);
			break;
		}
	
		case kResizeHeight:
		{
			HandleResizingColumnsContainer();
			break;
		}
		
		case kMoveColumn:
		{
			HandleMovingAColumn();
			break;
		}
		
		case kRefreshColumns:
		{
			HandleResizingColumns();
			break;
		}
		
		case kPauseOrSearch:
		{
			BString buttonLabel(fButton->Label());
			if (buttonLabel == B_TRANSLATE("Pause")) {
				fButton->SetLabel(B_TRANSLATE("Search"));
				SendPauseToPoseView();
			} else {
				fButton->SetLabel(B_TRANSLATE("Pause"));
				SaveQueryAsAttributesToFile();
				SendUpdateToPoseView();
			}
			break;
		}
		
		case kResetPauseButton:
		{
			fButton->SetLabel(B_TRANSLATE("Search"));
			break;
		}

		case kTVolumeItem:
		{
			// volume changed
			BMenuItem* invokedItem;
			dev_t dev;
			if (message->FindPointer("source", (void**)&invokedItem) != B_OK)
				return;

			if (message->FindInt32("device", &dev) != B_OK)
				break;

			BMenu* menu = invokedItem->Menu();
			ASSERT(menu);

			if (dev == -1) {
				// all disks selected, uncheck everything else
				int32 count = menu->CountItems();
				for (int32 index = 2; index < count; index++)
					menu->ItemAt(index)->SetMarked(false);

				// make all disks the title and check it
				PopUpMenuSetTitle(menu, menu->ItemAt(0)->Label());
				menu->ItemAt(0)->SetMarked(true);
			} else {
				// a specific volume selected, unmark "all disks"
				menu->ItemAt(0)->SetMarked(false);

				// toggle mark on invoked item
				int32 count = menu->CountItems();
				for (int32 index = 2; index < count; index++) {
					BMenuItem* item = menu->ItemAt(index);

					if (invokedItem == item) {
						// we just selected this
						bool wasMarked = item->IsMarked();
						item->SetMarked(!wasMarked);
					}
				}
			}
			// make sure the right label is showing
			ShowVolumeMenuLabel();

			break;
		}

		case kMIMETypeItem:
		{
			BMenuItem* item;
			if (message->FindPointer("source", (void**)&item) == B_OK) {
				// don't add the "All files and folders" to the list
				if (fMimeTypeMenu->IndexOf(item) != 0)
					gTMostUsedMimeTypes.AddName(item->Label());

				SetCurrentMimeType(item);
			}
			break;
		}

		case kScrollView:
		{
			float x, y;
			if (message->FindFloat("x", &x) != B_OK && message->FindFloat("y", &y) != B_OK)
				break;
			BPoint where(x, y);
			fColumnsContainer->ScrollTo(where);
			break;
		}

		default:
		{
			_inherited::MessageReceived(message);
			break;
		}
	}
}


TMostUsedNames::TMostUsedNames(const char* fileName, const char* directory,
	int32 maxCount)
	:
	fFileName(fileName),
	fDirectory(directory),
	fLoaded(false),
	fCount(maxCount)
{
}


TMostUsedNames::~TMostUsedNames()
{
	// only write back settings when we've been used
	if (!fLoaded)
		return;

	// write most used list to file

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) != B_OK
		|| path.Append(fDirectory) != B_OK || path.Append(fFileName) != B_OK) {
		return;
	}

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (file.InitCheck() == B_OK) {
		for (int32 i = 0; i < fList.CountItems(); i++) {
			list_entry* entry = static_cast<list_entry*>(fList.ItemAt(i));

			char line[B_FILE_NAME_LENGTH + 5];

			// limit upper bound to react more dynamically to changes
			if (--entry->count > 20)
				entry->count = 20;

			// if the item hasn't been chosen in a while, remove it
			// (but leave at least one item in the list)
			if (entry->count < -10 && i > 0)
				continue;

			sprintf(line, "%" B_PRId32 " %s\n", entry->count, entry->name);
			if (file.Write(line, strlen(line)) < B_OK)
				break;
		}
	}
	file.Unset();

	// free data

	for (int32 i = fList.CountItems(); i-- > 0;) {
		list_entry* entry = static_cast<list_entry*>(fList.ItemAt(i));
		free(entry->name);
		delete entry;
	}
}


bool
TMostUsedNames::ObtainList(BList* list)
{
	if (list == NULL)
		return false;

	if (!fLoaded)
		UpdateList();

	fLock.Lock();

	list->MakeEmpty();
	for (int32 i = 0; i < fCount; i++) {
		list_entry* entry = static_cast<list_entry*>(fList.ItemAt(i));
		if (entry == NULL)
			return true;

		list->AddItem(entry->name);
	}
	return true;
}


void
TMostUsedNames::ReleaseList()
{
	fLock.Unlock();
}


void
TMostUsedNames::AddName(const char* name)
{
	fLock.Lock();

	if (!fLoaded)
		LoadList();

	// remove last entry if there are more than
	// 2*fCount entries in the list

	list_entry* entry = NULL;

	if (fList.CountItems() > fCount * 2) {
		entry = static_cast<list_entry*>(
			fList.RemoveItem(fList.CountItems() - 1));

		// is this the name we want to add here?
		if (strcmp(name, entry->name)) {
			free(entry->name);
			delete entry;
			entry = NULL;
		} else
			fList.AddItem(entry);
	}

	if (entry == NULL) {
		for (int32 i = 0;
				(entry = static_cast<list_entry*>(fList.ItemAt(i))) != NULL; i++) {
			if (strcmp(entry->name, name) == 0)
				break;
		}
	}

	if (entry == NULL) {
		entry = new list_entry;
		entry->name = strdup(name);
		entry->count = 1;

		fList.AddItem(entry);
	} else if (entry->count < 0)
		entry->count = 1;
	else
		entry->count++;

	fLock.Unlock();
	UpdateList();
}


int
TMostUsedNames::CompareNames(const void* a,const void* b)
{
	list_entry* entryA = *(list_entry**)a;
	list_entry* entryB = *(list_entry**)b;

	if (entryA->count == entryB->count)
		return strcasecmp(entryA->name,entryB->name);

	return entryB->count - entryA->count;
}


void
TMostUsedNames::LoadList()
{
	if (fLoaded)
		return;
	fLoaded = true;

	// load the most used names list

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) != B_OK
		|| path.Append(fDirectory) != B_OK || path.Append(fFileName) != B_OK) {
		return;
	}

	FILE* file = fopen(path.Path(), "r");
	if (file == NULL)
		return;

	char line[B_FILE_NAME_LENGTH + 5];
	while (fgets(line, sizeof(line), file) != NULL) {
		int32 length = (int32)strlen(line) - 1;
		if (length >= 0 && line[length] == '\n')
			line[length] = '\0';

		int32 count = atoi(line);

		char* name = strchr(line, ' ');
		if (name == NULL || *(++name) == '\0')
			continue;

		list_entry* entry = new list_entry;
		entry->name = strdup(name);
		entry->count = count;

		fList.AddItem(entry);
	}
	fclose(file);
}


void
TMostUsedNames::UpdateList()
{
	AutoLock<Benaphore> locker(fLock);

	if (!fLoaded)
		LoadList();

	// sort list items

	fList.SortItems(TMostUsedNames::CompareNames);
}

}

