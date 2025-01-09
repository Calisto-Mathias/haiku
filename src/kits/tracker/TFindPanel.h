#ifndef T_FIND_PANEL_H
#define T_FIND_PANEL_H

// Imports
#include <View.h>

// Forward Declarations
class BButton;
class BFile;
class BMenu;
class BMenuField;
class BMenuItem;
class BMessage;
class BPopUpMenu;
class BQuery;


namespace BPrivate {

enum class AttributeType;
class BColumn;
class BQueryContainerWindow;
class BQueryPoseView;
class ShortMimeInfo;


class TFindPanel : public BView {
public:
								TFindPanel(BQueryContainerWindow*, BQueryPoseView*);
								~TFindPanel();

protected:
	virtual	void				MessageReceived(BMessage*);
	virtual	void				AttachedToWindow();

private:
			// Private Member Functions for populating Menus (Ported From FindPanel.cpp)
			BMenuItem*			CurrentMimeType(const char** type = NULL) const;
			status_t			SetCurrentMimeType(BMenuItem* item);
			status_t			SetCurrentMimeType(const char* label);
			void				AddMimeTypesToMenu(BPopUpMenu*);
	static	bool				AddOneMimeTypeToMenu(const ShortMimeInfo*, void* castToMenu);
			void				PushMimeType(BQuery*) const;
			BPopUpMenu*			MimeTypeMenu() const { return fMimeTypeMenu; }
			void				ShowVolumeMenuLabel();

	static	void				ResizeMenuField(BMenuField*);
	
			// Private Member Functions for handling fColumnsContainer;
			status_t			HandleResizingColumnsContainer();
			status_t			GetRequiredWidthOfColumnsContainer(float*) const;
			status_t			GetMaximumHeightOfColumns(float*) const;

			status_t			HandleMovingAColumn();
			status_t			HandleResizingColumns();

	static	status_t			CreateSearchColumnForAttributeType(AttributeType type);
			status_t			AddAttributeColumn(BColumn*);
			status_t			RemoveAttributeColumn(BColumn*);
			
			status_t			SaveQueryAsAttributesToFile();
			status_t			GetPredicateString(BString*) const;
			status_t			GetMimeTypeString(BString*) const;
			status_t			WritePredicateStringToFile(BString*);
			status_t			WriteVolumesToFile();
			
			//Find Panel Builders
			status_t			SetTemporaryFileHandle();
			void				BuildMimeTypeMenu();
			void				BuildVolumeMenu();
			void				AddVolumes(BMenu*);
			void				BuildFindPanelLayout();
			void				ResizeMenuFields();

			status_t			SendUpdateToPoseView();
			status_t			SendPauseToPoseView();
private:
			BFile*				fFile;
			BEntry*				fFileEntry;
			
			BPopUpMenu*			fMimeTypeMenu;
			BMenuField*			fMimeTypeField;
			BMenuField*			fVolumeField;
			BPopUpMenu*			fVolMenu;
			BButton*			fButton;
			
			BView*				fColumnsContainer;
			
			//Parent/Sister References
			BQueryContainerWindow*	fQueryContainerWindow;
			BQueryPoseView*			fQueryPoseView;
			
			typedef BView _inherited;
};

}

#endif