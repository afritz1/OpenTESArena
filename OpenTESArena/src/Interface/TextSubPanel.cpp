#include "SDL.h"

#include "CursorAlignment.h"
#include "RichTextString.h"
#include "TextBox.h"
#include "TextSubPanel.h"
#include "../Game/Game.h"
#include "../Math/Rect.h"
#include "../Media/FontManager.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"

TextSubPanel::TextSubPanel(Game &game, const Int2 &textCenter,
	const RichTextString &richText, const std::function<void(Game&)> &endingAction,
	Texture &&texture, const Int2 &textureCenter)
	: Panel(game), endingAction(endingAction), texture(std::move(texture)),
	textureCenter(textureCenter)
{
	this->textBox = std::make_unique<TextBox>(textCenter, richText, game.getRenderer());
}

TextSubPanel::TextSubPanel(Game &game, const Int2 &textCenter,
	const RichTextString &richText, const std::function<void(Game&)> &endingAction)
	: TextSubPanel(game, textCenter, richText, endingAction, std::move(Texture()), Int2()) { }

Panel::CursorData TextSubPanel::getCurrentCursor() const
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &textureManager = game.getTextureManager();
	
	const std::string &paletteFilename = PaletteFile::fromName(PaletteName::Default);
	PaletteID paletteID;
	if (!textureManager.tryGetPaletteID(paletteFilename.c_str(), &paletteID))
	{
		DebugLogWarning("Couldn't get palette ID for \"" + paletteFilename + "\".");
		return CursorData::EMPTY;
	}

	const std::string &textureFilename = TextureFile::fromName(TextureName::SwordCursor);
	TextureID textureID;
	if (!textureManager.tryGetTextureID(textureFilename.c_str(), paletteID, renderer, &textureID))
	{
		DebugLogWarning("Couldn't get texture ID for \"" + textureFilename + "\".");
		return CursorData::EMPTY;
	}

	const Texture &texture = textureManager.getTextureHandle(textureID);
	return CursorData(&texture, CursorAlignment::TopLeft);
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
