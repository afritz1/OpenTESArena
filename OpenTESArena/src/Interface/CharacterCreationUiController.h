#ifndef CHARACTER_CREATION_UI_CONTROLLER_H
#define CHARACTER_CREATION_UI_CONTROLLER_H

class Game;
class ListBox;

namespace CharacterCreationUiController
{
	// -- Choose class creation --
	void onBackToMainMenuButtonSelected(Game &game);
	void onGenerateClassButtonSelected(Game &game);
	void onSelectClassButtonSelected(Game &game);

	// -- Choose class --
	void onBackToChooseClassCreationButtonSelected(Game &game);
	void onChooseClassListBoxUpButtonSelected(ListBox &listBox);
	void onChooseClassListBoxDownButtonSelected(ListBox &listBox);
	void onChooseClassListBoxAcceptButtonSelected(Game &game, int charClassDefID);

	// -- Choose attributes --
	void onBackToRaceSelectionButtonSelected(Game &game);
	void onChooseAttributesPopUpSelected(Game &game);
	void onSaveAttributesButtonSelected(Game &game, bool *attributesAreSaved);
	void onRerollAttributesButtonSelected(Game &game);
	void onAppearanceMessageBoxSelected(Game &game);
	void onAppearancePortraitButtonSelected(Game &game, bool incrementIndex);
	void onAttributesDoneButtonSelected(Game &game, bool *attributesAreSaved);
	void onUnsavedAttributesDoneButtonSelected(Game &game, bool *attributesAreSaved);
	void onSavedAttributesDoneButtonSelected(Game &game);

	// -- Cinematic after character creation --
	void onPostCharacterCreationCinematicFinished(Game &game);
}

#endif
