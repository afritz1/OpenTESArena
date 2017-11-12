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
	this->textBox = std::unique_ptr<TextBox>(new TextBox(
		textCenter, richText, game.getRenderer()));
}

TextSubPanel::TextSubPanel(Game &game, const Int2 &textCenter,
	const RichTextString &richText, const std::function<void(Game&)> &endingAction)
	: TextSubPanel(game, textCenter, richText, endingAction, std::move(Texture()), Int2()) { }

TextSubPanel::~TextSubPanel()
{

}

std::pair<SDL_Texture*, CursorAlignment> TextSubPanel::getCurrentCursor() const
{
	auto &textureManager = this->getGame().getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor),
		PaletteFile::fromName(PaletteName::Default));
	return std::make_pair(texture.get(), CursorAlignment::TopLeft);
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
		this->getGame().popSubPanel();
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
		const Rect nativeTextureRect = renderer.originalRectToNative(textureRect);

		renderer.draw(this->texture.get(),
			nativeTextureRect.getLeft(),
			nativeTextureRect.getTop(),
			nativeTextureRect.getWidth(),
			nativeTextureRect.getHeight());
	}

	const Rect nativeTextBoxRect = renderer.originalRectToNative(this->textBox->getRect());

	// Draw text.
	renderer.draw(this->textBox->getTexture(),
		nativeTextBoxRect.getLeft(),
		nativeTextBoxRect.getTop(),
		nativeTextBoxRect.getWidth(),
		nativeTextBoxRect.getHeight());
}
