#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "ChooseAttributesUiState.h"
#include "ChooseClassCreationUiState.h"
#include "ChooseClassUiState.h"
#include "ChooseGenderUiState.h"
#include "ChooseNameUiState.h"
#include "ChooseRaceUiState.h"
#include "CinematicLibrary.h"
#include "GameWorldUiState.h"
#include "MainMenuUiState.h"
#include "TextCinematicUiState.h"
#include "WorldMapUiModel.h"
#include "../Audio/MusicLibrary.h"
#include "../Audio/MusicUtils.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../UI/TextEntry.h"

#include "components/utilities/String.h"

void ChooseClassCreationUiController::onBackToMainMenuInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		Game &game = values.game;
		game.setCharacterCreationState(nullptr);
		game.setNextContext(MainMenuUI::ContextName);

		const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
		const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinition(MusicType::MainMenu, game.random);
		if (musicDef == nullptr)
		{
			DebugLogWarning("Missing main menu music.");
		}

		AudioManager &audioManager = game.audioManager;
		audioManager.setMusic(musicDef);

		InputManager &inputManager = game.inputManager;
		inputManager.setInputActionMapActive(InputActionMapName::CharacterCreation, false);
	}
}

void ChooseClassCreationUiController::onGenerateButtonSelected(Game &game)
{
	// @todo: eventually go to a ChooseQuestionsUI with "pop-up" message
}

void ChooseClassCreationUiController::onSelectButtonSelected(Game &game)
{
	game.setNextContext(ChooseClassUI::ContextName);
}

void ChooseClassUiController::onBackToChooseClassCreationInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		Game &game = values.game;
		game.setNextContext(ChooseClassCreationUI::ContextName);
	}
}

void ChooseClassUiController::onItemButtonSelected(Game &game, int charClassDefID)
{
	CharacterCreationState &charCreationState = game.getCharacterCreationState();
	charCreationState.classDefID = charClassDefID;

	game.setNextContext(ChooseNameUI::ContextName);
}

void ChooseGenderUiController::onBackToChooseNameInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		Game &game = values.game;
		game.setNextContext(ChooseNameUI::ContextName);
	}
}

void ChooseGenderUiController::onMaleButtonSelected(Game &game)
{
	CharacterCreationState &charCreationState = game.getCharacterCreationState();
	charCreationState.male = true;

	game.setNextContext(ChooseRaceUI::ContextName);
}

void ChooseGenderUiController::onFemaleButtonSelected(Game &game)
{
	CharacterCreationState &charCreationState = game.getCharacterCreationState();
	charCreationState.male = false;

	game.setNextContext(ChooseRaceUI::ContextName);
}

void ChooseNameUiController::onBackToChooseClassInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		Game &game = values.game;
		InputManager &inputManager = game.inputManager;
		inputManager.setTextInputMode(false);

		CharacterCreationState &charCreationState = game.getCharacterCreationState();
		charCreationState.setName(nullptr);

		game.setNextContext(ChooseClassUI::ContextName);
	}
}

void ChooseNameUiController::onTextInput(const std::string_view text, std::string &name, bool *outDirty)
{
	DebugAssert(outDirty != nullptr);
	*outDirty = TextEntry::append(name, text, ChooseNameUiModel::isCharacterAccepted, CharacterCreationState::MAX_NAME_LENGTH);
}

void ChooseNameUiController::onBackspaceInputAction(const InputActionCallbackValues &values, std::string &name, bool *outDirty)
{
	DebugAssert(outDirty != nullptr);

	if (values.performed)
	{
		*outDirty = TextEntry::backspace(name);
	}
}

void ChooseNameUiController::onAcceptInputAction(const InputActionCallbackValues &values, const std::string &name)
{
	if (values.performed)
	{
		if (name.size() > 0)
		{
			Game &game = values.game;
			InputManager &inputManager = game.inputManager;
			inputManager.setTextInputMode(false);

			CharacterCreationState &charCreationState = game.getCharacterCreationState();
			charCreationState.setName(name.c_str());

			game.setNextContext(ChooseGenderUI::ContextName);
		}
	}
}

void ChooseAttributesUiController::onPostCharacterCreationCinematicFinished(Game &game)
{
	game.setNextContext(GameWorldUI::ContextName);

	const MusicDefinition *musicDef = MusicUtils::getRandomDungeonMusicDefinition(game.random);
	if (musicDef == nullptr)
	{
		DebugLogWarning("Missing dungeon music.");
	}

	AudioManager &audioManager = game.audioManager;
	audioManager.setMusic(musicDef);
}
