#include "SDL.h"

#include "CursorAlignment.h"
#include "RichTextString.h"
#include "TextBox.h"
#include "TextSubPanel.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Game/Game.h"
#include "../Math/Rect.h"
#include "../Media/FontLibrary.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"

TextSubPanel::TextSubPanel(Game &game, const Int2 &textCenter,
	const RichTextString &richText, const std::function<void(Game&)> &endingAction,
	Texture &&texture, const Int2 &textureCenter)
	: Panel(game), endingAction(endingAction), texture(std::move(texture)),
	textureCenter(textureCenter)
{
	this->textBox = std::make_unique<TextBox>(textCenter, richText,
		game.getFontLibrary(), game.getRenderer());
}

TextSubPanel::TextSubPanel(Game &game, const Int2 &textCenter,
	const RichTextString &richText, const std::function<void(Game&)> &endingAction)
	: TextSubPanel(game, textCenter, richText, endingAction, std::move(Texture()), Int2()) { }

std::optional<Panel::CursorData> TextSubPanel::getCurrentCursor() const
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &textureManager = game.getTextureManager();
	
	const std::string &paletteFilename = ArenaPaletteName::Default;
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteFilename.c_str());
	if (!paletteID.has_value())
	{
		DebugLogWarning("Couldn't get palette ID for \"" + paletteFilename + "\".");
		return std::nullopt;
	}

	const std::string &textureFilename = ArenaTextureName::SwordCursor;
	const std::optional<TextureBuilderID> textureBuilderID =
		textureManager.tryGetTextureBuilderID(textureFilename.c_str());
	if (!textureBuilderID.has_value())
	{
		DebugLogWarning("Couldn't get texture ID for \"" + textureFilename + "\".");
		return std::nullopt;
	}

	return CursorData(*textureBuilderID, *paletteID, CursorAlignment::TopLeft);
}

void TextSubPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	bool spacePressed = inputManager.keyPressed(e, SDLK_SPACE);
	bool enterPressed = inputManager.keyPressed(e, SDLK_RETURN) ||
		inputManager.keyPressed(e, SDLK_KP_ENTER);
	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
	bool rightClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_RIGHT);

	if (escapePressed || spacePressed || enterPressed || leftClick || rightClick)
	{
		this->endingAction(this->getGame());
	}
}

void TextSubPanel::render(Renderer &renderer)
{
	// Draw background if it exists.
	if (this->texture.get() != nullptr)
	{
		const Rect textureRect(
			this->textureCenter.x - (this->texture.getWidth() / 2),
			this->textureCenter.y - (this->texture.getHeight() / 2),
			this->texture.getWidth(),
			this->texture.getHeight());
		const Rect nativeTextureRect = renderer.originalToNative(textureRect);

		renderer.draw(this->texture,
			nativeTextureRect.getLeft(),
			nativeTextureRect.getTop(),
			nativeTextureRect.getWidth(),
			nativeTextureRect.getHeight());
	}

	const Rect nativeTextBoxRect = renderer.originalToNative(this->textBox->getRect());

	// Draw text.
	renderer.draw(this->textBox->getTexture(),
		nativeTextBoxRect.getLeft(),
		nativeTextBoxRect.getTop(),
		nativeTextBoxRect.getWidth(),
		nativeTextBoxRect.getHeight());
}
