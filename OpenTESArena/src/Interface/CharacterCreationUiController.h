#ifndef CHARACTER_CREATION_UI_CONTROLLER_H
#define CHARACTER_CREATION_UI_CONTROLLER_H

#include <string>

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
	void onChooseClassListBoxItemButtonSelected(Game &game, int charClassDefID);

	// -- Choose gender --
	void onBackToChooseNameButtonSelected(Game &game);
	void onChooseGenderMaleButtonSelected(Game &game);
	void onChooseGenderFemaleButtonSelected(Game &game);

	// -- Choose name --
	void onBackToChooseClassButtonSelected(Game &game);
	void onChooseNameAcceptButtonSelected(Game &game, const std::string &acceptedName);

	// -- Choose race --
	void onBackToChooseGenderButtonSelected(Game &game);
	void onChooseRaceInitialPopUpButtonSelected(Game &game);
	void onChooseRaceProvinceButtonSelected(Game &game, int raceID);
	void onChooseRaceProvinceConfirmButtonSelected(Game &game, int raceID);
	void onChooseRaceProvinceCancelButtonSelected(Game &game);
	void onChooseRaceProvinceConfirmedFirstButtonSelected(Game &game);
	void onChooseRaceProvinceConfirmedSecondButtonSelected(Game &game);
	void onChooseRaceProvinceConfirmedThirdButtonSelected(Game &game);
	void onChooseRaceProvinceConfirmedFourthButtonSelected(Game &game);

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
