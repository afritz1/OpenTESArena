#include "SDL.h"

#include "ChooseClassCreationPanel.h"
#include "ChooseClassPanel.h"
#include "CursorAlignment.h"
#include "MainMenuPanel.h"
#include "RichTextString.h"
#include "Surface.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/ExeData.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/FontLibrary.h"
#include "../Media/FontName.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"

#include "components/utilities/String.h"

ChooseClassCreationPanel::ChooseClassCreationPanel(Game &game)
	: Panel(game)
{
	this->parchment = TextureUtils::generate(TextureUtils::PatternType::Parchment, 180, 40,
		game.getTextureManager(), game.getRenderer());

	this->titleTextBox = [&game]()
	{
		const Int2 center((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 80);

		const auto &exeData = game.getBinaryAssetLibrary().getExeData();
		std::string text = exeData.charCreation.chooseClassCreation;
		text = String::replace(text, '\r', '\n');

		const int lineSpacing = 1;

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			text,
			FontName::A,
			Color(48, 12, 12),
			TextAlignment::Center,
			lineSpacing,
			fontLibrary);

		return std::make_unique<TextBox>(center, richText, fontLibrary, game.getRenderer());
	}();

	this->generateTextBox = [&game]()
	{
		const Int2 center((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 120);

		const auto &exeData = game.getBinaryAssetLibrary().getExeData();
		const std::string &text = exeData.charCreation.chooseClassCreationGenerate;

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			text,
			FontName::A,
			Color(48, 12, 12),
			TextAlignment::Center,
			fontLibrary);

		return std::make_unique<TextBox>(center, richText, fontLibrary, game.getRenderer());
	}();

	this->selectTextBox = [&game]()
	{
		const Int2 center((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 160);

		const auto &exeData = game.getBinaryAssetLibrary().getExeData();
		const std::string &text = exeData.charCreation.chooseClassCreationSelect;

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			text,
			FontName::A,
			Color(48, 12, 12),
			TextAlignment::Center,
			fontLibrary);

		return std::make_unique<TextBox>(center, richText, fontLibrary, game.getRenderer());
	}();

	this->backToMainMenuButton = []()
	{
		auto function = [](Game &game)
		{
			game.setCharacterCreationState(nullptr);
			game.setPanel<MainMenuPanel>(game);

			const MusicLibrary &musicLibrary = game.getMusicLibrary();
			const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinition(
				MusicDefinition::Type::MainMenu, game.getRandom());

			if (musicDef == nullptr)
			{
				DebugLogWarning("Missing main menu music.");
			}

			AudioManager &audioManager = game.getAudioManager();
			audioManager.setMusic(musicDef);
		};

		return Button<Game&>(function);
	}();

	this->generateButton = []()
	{
		const Int2 center((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 120);
		auto function = [](Game &game)
		{
			// Eventually go to a "ChooseQuestionsPanel". What about the "pop-up" message?
		};
		return Button<Game&>(center, 175, 35, function);
	}();

	this->selectButton = []()
	{
		const Int2 center((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 160);
		auto function = [](Game &game)
		{
			game.setPanel<ChooseClassPanel>(game);
		};
		return Button<Game&>(center, 175, 35, function);
	}();
}

std::optional<Panel::CursorData> ChooseClassCreationPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void ChooseClassCreationPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);

	if (escapePressed)
	{
		this->backToMainMenuButton.click(this->getGame());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame().getRenderer()
			.nativeToOriginal(mousePosition);

		if (this->generateButton.contains(mouseOriginalPoint))
		{
			this->generateButton.click(this->getGame());
		}
		else if (this->selectButton.contains(mouseOriginalPoint))
		{
			this->selectButton.click(this->getGame());
		}
	}
}

void ChooseClassCreationPanel::drawTooltip(const std::string &text, Renderer &renderer)
{
	const Texture tooltip = Panel::createTooltip(
		text, FontName::D, this->getGame().getFontLibrary(), renderer);

	const auto &inputManager = this->getGame().getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPosition = renderer.nativeToOriginal(mousePosition);
	const int mouseX = originalPosition.x;
	const int mouseY = originalPosition.y;
	const int x = ((mouseX + 8 + tooltip.getWidth()) < ArenaRenderUtils::SCREEN_WIDTH) ?
		(mouseX + 8) : (mouseX - tooltip.getWidth());
	const int y = ((mouseY + tooltip.getHeight()) < ArenaRenderUtils::SCREEN_HEIGHT) ?
		(mouseY - 1) : (mouseY - tooltip.getHeight());

	renderer.drawOriginal(tooltip, x, y);
}

void ChooseClassCreationPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Draw background.
	auto &textureManager = this->getGame().getTextureManager();
	const std::string &backgroundFilename = ArenaTextureName::CharacterCreation;
	const std::optional<PaletteID> backgroundPaletteID = textureManager.tryGetPaletteID(backgroundFilename.c_str());
	if (!backgroundPaletteID.has_value())
	{
		DebugLogError("Couldn't get background palette ID for \"" + backgroundFilename + "\".");
		return;
	}

	const std::optional<TextureBuilderID> backgroundTextureBuilderID =
		textureManager.tryGetTextureBuilderID(backgroundFilename.c_str());
	if (!backgroundTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get background texture builder ID for \"" + backgroundFilename + "\".");
		return;
	}

	renderer.drawOriginal(*backgroundTextureBuilderID, *backgroundPaletteID, textureManager);

	// Draw parchments: title, generate, select.
	const int parchmentX = (ArenaRenderUtils::SCREEN_WIDTH / 2) - (this->parchment.getWidth() / 2) - 1;
	const int parchmentY = (ArenaRenderUtils::SCREEN_HEIGHT / 2) - (this->parchment.getHeight() / 2) + 1;

	renderer.drawOriginal(this->parchment, parchmentX, parchmentY - 20);
	renderer.drawOriginal(this->parchment, parchmentX, parchmentY + 20);
	renderer.drawOriginal(this->parchment, parchmentX, parchmentY + 60);

	// Draw text: title, generate, select.
	renderer.drawOriginal(this->titleTextBox->getTexture(),
		this->titleTextBox->getX(), this->titleTextBox->getY());
	renderer.drawOriginal(this->generateTextBox->getTexture(),
		this->generateTextBox->getX(), this->generateTextBox->getY());
	renderer.drawOriginal(this->selectTextBox->getTexture(),
		this->selectTextBox->getX(), this->selectTextBox->getY());

	// Check if the mouse is hovered over one of the boxes for tooltips.
	const auto &inputManager = this->getGame().getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPoint = renderer.nativeToOriginal(mousePosition);

	if (this->generateButton.contains(originalPoint))
	{
		this->drawTooltip("Answer questions\n(not implemented)", renderer);
	}
	else if (this->selectButton.contains(originalPoint))
	{
		this->drawTooltip("Choose from a list", renderer);
	}
}
