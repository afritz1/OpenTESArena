#ifndef CHARACTER_CREATION_UI_CONTROLLER_H
#define CHARACTER_CREATION_UI_CONTROLLER_H

#include <string>

#include "../Math/Vector2.h"

class Game;

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
	void onTextInput(const std::string_view text, std::string &name, bool *outDirty);
	void onBackspaceInputAction(const InputActionCallbackValues &values, std::string &name, bool *outDirty);
	void onAcceptInputAction(const InputActionCallbackValues &values, const std::string &name);
}

namespace ChooseAttributesUiController
{
	// -- Cinematic after character creation --
	void onPostCharacterCreationCinematicFinished(Game &game);
}

#endif
