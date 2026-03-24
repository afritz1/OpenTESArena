#include <optional>

#include "SDL.h"

#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "ChooseAttributesPanel.h"
#include "ChooseClassCreationPanel.h"
#include "ChooseClassPanel.h"
#include "ChooseGenderPanel.h"
#include "ChooseNamePanel.h"
#include "ChooseRacePanel.h"
#include "CinematicLibrary.h"
#include "GameWorldPanel.h"
#include "MainMenuPanel.h"
#include "TextCinematicPanel.h"
#include "WorldMapUiModel.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Audio/MusicLibrary.h"
#include "../Audio/MusicUtils.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../Sky/SkyUtils.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../Time/ArenaClockUtils.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../UI/TextBox.h"
#include "../UI/TextEntry.h"
#include "../World/CardinalDirection.h"
#include "../WorldMap/ArenaLocationUtils.h"

#include "components/utilities/String.h"

void ChooseClassCreationUiController::onBackToMainMenuInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		auto &game = values.game;
		game.setCharacterCreationState(nullptr);
		game.setPanel<MainMenuPanel>();

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
	// @todo: eventually go to a "ChooseQuestionsPanel" with "pop-up" message
}

void ChooseClassCreationUiController::onSelectButtonSelected(Game &game)
{
	game.setPanel<ChooseClassPanel>();
}

void ChooseClassUiController::onBackToChooseClassCreationInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		auto &game = values.game;
		game.setPanel<ChooseClassCreationPanel>();
	}
}

void ChooseClassUiController::onUpButtonSelected(ListBox &listBox)
{
	listBox.scrollUp();
}

void ChooseClassUiController::onDownButtonSelected(ListBox &listBox)
{
	listBox.scrollDown();
}

void ChooseClassUiController::onItemButtonSelected(Game &game, int charClassDefID)
{
	auto &charCreationState = game.getCharacterCreationState();
	charCreationState.classDefID = charClassDefID;

	game.setPanel<ChooseNamePanel>();
}

void ChooseGenderUiController::onBackToChooseNameInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		auto &game = values.game;
		game.setPanel<ChooseNamePanel>();
	}
}

void ChooseGenderUiController::onMaleButtonSelected(Game &game)
{
	auto &charCreationState = game.getCharacterCreationState();
	charCreationState.male = true;

	game.setPanel<ChooseRacePanel>();
}

void ChooseGenderUiController::onFemaleButtonSelected(Game &game)
{
	auto &charCreationState = game.getCharacterCreationState();
	charCreationState.male = false;

	game.setPanel<ChooseRacePanel>();
}

void ChooseNameUiController::onBackToChooseClassInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		auto &game = values.game;
		auto &inputManager = game.inputManager;
		inputManager.setTextInputMode(false);

		auto &charCreationState = game.getCharacterCreationState();
		charCreationState.setName(nullptr);

		game.setPanel<ChooseClassPanel>();
	}
}

void ChooseNameUiController::onTextInput(const std::string_view text, std::string &name, bool *outDirty)
{
	DebugAssert(outDirty != nullptr);

	*outDirty = TextEntry::append(name, text, ChooseNameUiModel::isCharacterAccepted,
		CharacterCreationState::MAX_NAME_LENGTH);
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
			auto &game = values.game;
			auto &inputManager = game.inputManager;
			inputManager.setTextInputMode(false);

			auto &charCreationState = game.getCharacterCreationState();
			charCreationState.setName(name.c_str());

			game.setPanel<ChooseGenderPanel>();
		}
	}
}

void ChooseAttributesUiController::onPostCharacterCreationCinematicFinished(Game &game)
{
	// Initialize the game world panel.
	game.setPanel<GameWorldPanel>();

	const MusicDefinition *musicDef = MusicUtils::getRandomDungeonMusicDefinition(game.random);
	if (musicDef == nullptr)
	{
		DebugLogWarning("Missing dungeon music.");
	}

	AudioManager &audioManager = game.audioManager;
	audioManager.setMusic(musicDef);
}
