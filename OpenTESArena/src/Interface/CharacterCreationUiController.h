#ifndef CHARACTER_CREATION_UI_CONTROLLER_H
#define CHARACTER_CREATION_UI_CONTROLLER_H

#include <string>

class Game;
class ListBox;

struct InputActionCallbackValues;

namespace ChooseClassCreationUiController
{
	void onBackToMainMenuInputAction(const InputActionCallbackValues &values);

	void onGenerateButtonSelected(Game &game);
	void onSelectButtonSelected(Game &game);
}

namespace ChooseClassUiController
{
	void onBackToChooseClassCreationButtonSelected(Game &game);
	void onUpButtonSelected(ListBox &listBox);
	void onDownButtonSelected(ListBox &listBox);
	void onItemButtonSelected(Game &game, int charClassDefID);
}

namespace ChooseGenderUiController
{
	void onBackToChooseNameButtonSelected(Game &game);
	void onMaleButtonSelected(Game &game);
	void onFemaleButtonSelected(Game &game);
}

namespace ChooseNameUiController
{
	void onBackToChooseClassButtonSelected(Game &game);
	void onAcceptButtonSelected(Game &game, const std::string &acceptedName);
}

namespace ChooseRaceUiController
{
	void onBackToChooseGenderButtonSelected(Game &game);
	void onInitialPopUpButtonSelected(Game &game);
	void onProvinceButtonSelected(Game &game, int raceID);
	void onProvinceConfirmButtonSelected(Game &game, int raceID);
	void onProvinceCancelButtonSelected(Game &game);
	void onProvinceConfirmedFirstButtonSelected(Game &game);
	void onProvinceConfirmedSecondButtonSelected(Game &game);
	void onProvinceConfirmedThirdButtonSelected(Game &game);
	void onProvinceConfirmedFourthButtonSelected(Game &game);
}

namespace ChooseAttributesUiController
{
	void onBackToRaceSelectionInputAction(const InputActionCallbackValues &values);

	void onInitialPopUpSelected(Game &game);
	void onSaveButtonSelected(Game &game, bool *attributesAreSaved);
	void onRerollButtonSelected(Game &game);
	void onAppearanceTextBoxSelected(Game &game);
	void onPortraitButtonSelected(Game &game, bool incrementIndex);
	void onDoneButtonSelected(Game &game, bool *attributesAreSaved);
	void onUnsavedDoneButtonSelected(Game &game, bool *attributesAreSaved);
	void onSavedDoneButtonSelected(Game &game);

	// -- Cinematic after character creation --
	void onPostCharacterCreationCinematicFinished(Game &game);
}

#endif
