#ifndef CHARACTER_CREATION_UI_CONTROLLER_H
#define CHARACTER_CREATION_UI_CONTROLLER_H

#include <string>

#include "../Math/Vector2.h"

class Game;
class ListBox;

enum class MouseButtonType;

struct InputActionCallbackValues;

namespace ChooseClassCreationUiController
{
	void onBackToMainMenuInputAction(const InputActionCallbackValues &values);

	void onGenerateButtonSelected(Game &game);
	void onSelectButtonSelected(Game &game);
}

namespace ChooseClassUiController
{
	void onBackToChooseClassCreationInputAction(const InputActionCallbackValues &values);
	void onUpButtonSelected(ListBox &listBox);
	void onDownButtonSelected(ListBox &listBox);
	void onItemButtonSelected(Game &game, int charClassDefID);
}

namespace ChooseGenderUiController
{
	void onBackToChooseNameInputAction(const InputActionCallbackValues &values);
	void onMaleButtonSelected(Game &game);
	void onFemaleButtonSelected(Game &game);
}

namespace ChooseNameUiController
{
	void onBackToChooseClassInputAction(const InputActionCallbackValues &values);
	void onTextInput(const std::string_view &text, std::string &name, bool *outDirty);
	void onBackspaceInputAction(const InputActionCallbackValues &values, std::string &name, bool *outDirty);
	void onAcceptInputAction(const InputActionCallbackValues &values, const std::string &name);
}

namespace ChooseRaceUiController
{
	void onBackToChooseGenderInputAction(const InputActionCallbackValues &values);
	void onInitialPopUpButtonSelected(Game &game);
	void onMouseButtonChanged(Game &game, MouseButtonType buttonType, const Int2 &position, bool pressed);
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
