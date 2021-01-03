#include <unordered_map>

#include "SDL.h"

#include "ChooseClassPanel.h"
#include "ChooseGenderPanel.h"
#include "ChooseNamePanel.h"
#include "CursorAlignment.h"
#include "RichTextString.h"
#include "Surface.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "TextEntry.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/ExeData.h"
#include "../Entities/CharacterClassDefinition.h"
#include "../Entities/CharacterClassLibrary.h"
#include "../Game/CharacterCreationState.h"
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

ChooseNamePanel::ChooseNamePanel(Game &game)
	: Panel(game)
{
	this->parchment = TextureUtils::generate(TextureUtils::PatternType::Parchment, 300, 60,
		game.getTextureManager(), game.getRenderer());

	this->titleTextBox = [&game]()
	{
		const int x = 26;
		const int y = 82;

		const auto &charCreationState = game.getCharacterCreationState();
		const auto &charClassLibrary = game.getCharacterClassLibrary();
		const int charClassDefID = charCreationState.getClassDefID();
		const auto &charClassDef = charClassLibrary.getDefinition(charClassDefID);

		const auto &exeData = game.getBinaryAssetLibrary().getExeData();
		std::string text = exeData.charCreation.chooseName;
		text = String::replace(text, "%s", charClassDef.getName());

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			text,
			FontName::A,
			Color(48, 12, 12),
			TextAlignment::Left,
			fontLibrary);

		return std::make_unique<TextBox>(x, y, richText, fontLibrary, game.getRenderer());
	}();

	this->nameTextBox = [&game]()
	{
		const int x = 61;
		const int y = 101;

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			std::string(),
			FontName::A,
			Color(48, 12, 12),
			TextAlignment::Left,
			fontLibrary);

		return std::make_unique<TextBox>(x, y, richText, fontLibrary, game.getRenderer());
	}();

	this->backToClassButton = []()
	{
		auto function = [](Game &game)
		{
			SDL_StopTextInput();

			auto &charCreationState = game.getCharacterCreationState();
			charCreationState.setName(nullptr);

			game.setPanel<ChooseClassPanel>(game);
		};

		return Button<Game&>(function);
	}();

	this->acceptButton = []()
	{
		auto function = [](Game &game, const std::string &name)
		{
			SDL_StopTextInput();

			auto &charCreationState = game.getCharacterCreationState();
			charCreationState.setName(name.c_str());

			game.setPanel<ChooseGenderPanel>(game);
		};

		return Button<Game&, const std::string&>(function);
	}();

	// Activate SDL text input (handled in handleEvent()).
	SDL_StartTextInput();
}

std::optional<Panel::CursorData> ChooseNamePanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void ChooseNamePanel::handleEvent(const SDL_Event &e)
{
	auto &game = this->getGame();
	const auto &inputManager = game.getInputManager();
	const bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	const bool enterPressed = inputManager.keyPressed(e, SDLK_RETURN) ||
		inputManager.keyPressed(e, SDLK_KP_ENTER);
	const bool backspacePressed = inputManager.keyPressed(e, SDLK_BACKSPACE) ||
		inputManager.keyPressed(e, SDLK_KP_BACKSPACE);

	if (escapePressed)
	{
		this->backToClassButton.click(this->getGame());
	}
	else if (enterPressed && (this->name.size() > 0))
	{
		// Accept the given name.
		this->acceptButton.click(this->getGame(), this->name);
	}
	else
	{
		// Listen for SDL text input and changes in text. Only letters and spaces are allowed.
		auto charIsAllowed = [](char c)
		{
			return (c == ' ') || ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z'));
		};
		
		const bool textChanged = TextEntry::updateText(this->name, e,
			backspacePressed, charIsAllowed, CharacterCreationState::MAX_NAME_LENGTH);

		if (textChanged)
		{
			// Update the displayed name.
			this->nameTextBox = [this]()
			{
				const int x = 61;
				const int y = 101;

				auto &game = this->getGame();
				const RichTextString &oldRichText = this->nameTextBox->getRichText();

				const auto &fontLibrary = game.getFontLibrary();
				const RichTextString richText(
					this->name,
					oldRichText.getFontName(),
					oldRichText.getColor(),
					oldRichText.getAlignment(),
					fontLibrary);

				return std::make_unique<TextBox>(x, y, richText, fontLibrary, game.getRenderer());
			}();
		}
	}
}

void ChooseNamePanel::render(Renderer &renderer)
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

	// Draw parchment: title.
	renderer.drawOriginal(this->parchment,
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - (this->parchment.getWidth() / 2),
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - (this->parchment.getHeight() / 2));

	// Draw text: title, name.
	renderer.drawOriginal(this->titleTextBox->getTexture(),
		this->titleTextBox->getX(), this->titleTextBox->getY());
	renderer.drawOriginal(this->nameTextBox->getTexture(),
		this->nameTextBox->getX(), this->nameTextBox->getY());
}
