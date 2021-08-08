#include <unordered_map>

#include "SDL.h"

#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "ChooseClassPanel.h"
#include "ChooseGenderPanel.h"
#include "ChooseNamePanel.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/ExeData.h"
#include "../Entities/CharacterClassDefinition.h"
#include "../Entities/CharacterClassLibrary.h"
#include "../Game/CharacterCreationState.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Input/InputActionName.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../UI/CursorAlignment.h"
#include "../UI/CursorData.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextEntry.h"

#include "components/utilities/String.h"

ChooseNamePanel::ChooseNamePanel(Game &game)
	: Panel(game) { }

bool ChooseNamePanel::init()
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();

	this->parchment = TextureUtils::generate(
		ChooseNameUiView::TexturePatternType,
		ChooseNameUiView::TextureWidth,
		ChooseNameUiView::TextureHeight,
		game.getTextureManager(),
		renderer);

	const auto &fontLibrary = game.getFontLibrary();
	const std::string titleText = ChooseNameUiModel::getTitleText(game);
	const TextBox::InitInfo titleTextBoxInitInfo = ChooseNameUiView::getTitleTextBoxInitInfo(titleText, fontLibrary);
	if (!this->titleTextBox.init(titleTextBoxInitInfo, titleText, renderer))
	{
		DebugLogError("Couldn't init title text box.");
		return false;
	}

	const TextBox::InitInfo entryTextBoxInitInfo = ChooseNameUiView::getEntryTextBoxInitInfo(fontLibrary);
	if (!this->entryTextBox.init(entryTextBoxInitInfo, renderer))
	{
		DebugLogError("Couldn't init entry text box.");
		return false;
	}

	this->addInputActionListener(InputActionName::Back, ChooseNameUiController::onBackToChooseClassInputAction);
	this->addInputActionListener(InputActionName::Accept,
		[this](const InputActionCallbackValues &values)
	{
		ChooseNameUiController::onAcceptInputAction(values, this->name);
	});

	this->addInputActionListener(InputActionName::Backspace,
		[this](const InputActionCallbackValues &values)
	{
		bool dirty;
		ChooseNameUiController::onBackspaceInputAction(values, this->name, &dirty);

		if (dirty)
		{
			this->entryTextBox.setText(this->name);
		}
	});

	this->addTextInputListener([this](const std::string_view &text)
	{
		bool dirty;
		ChooseNameUiController::onTextInput(text, this->name, &dirty);

		if (dirty)
		{
			this->entryTextBox.setText(this->name);
		}
	});

	auto &inputManager = game.getInputManager();
	inputManager.setTextInputMode(true);

	return true;
}

std::optional<CursorData> ChooseNamePanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void ChooseNamePanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Draw background.
	auto &textureManager = this->getGame().getTextureManager();
	const TextureAssetReference backgroundTextureAssetRef = CharacterCreationUiView::getNightSkyTextureAssetRef();
	const std::optional<PaletteID> backgroundPaletteID = textureManager.tryGetPaletteID(backgroundTextureAssetRef);
	if (!backgroundPaletteID.has_value())
	{
		DebugLogError("Couldn't get background palette ID for \"" + backgroundTextureAssetRef.filename + "\".");
		return;
	}

	const std::optional<TextureBuilderID> backgroundTextureBuilderID = textureManager.tryGetTextureBuilderID(backgroundTextureAssetRef);
	if (!backgroundTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get background texture builder ID for \"" + backgroundTextureAssetRef.filename + "\".");
		return;
	}

	renderer.drawOriginal(*backgroundTextureBuilderID, *backgroundPaletteID, textureManager);

	// Draw parchment: title.
	const int titleParchmentX = ChooseNameUiView::getTitleTextureX(this->parchment.getWidth());
	const int titleParchmentY = ChooseNameUiView::getTitleTextureY(this->parchment.getHeight());
	renderer.drawOriginal(this->parchment, titleParchmentX, titleParchmentY);

	// Draw text: title, name.
	const Rect &titleTextBoxRect = this->titleTextBox.getRect();
	const Rect &entryTextBoxRect = this->entryTextBox.getRect();
	renderer.drawOriginal(this->titleTextBox.getTexture(), titleTextBoxRect.getLeft(), titleTextBoxRect.getTop());
	renderer.drawOriginal(this->entryTextBox.getTexture(), entryTextBoxRect.getLeft(), entryTextBoxRect.getTop());
}
