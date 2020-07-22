#include "CursorAlignment.h"
#include "MessageBoxSubPanel.h"
#include "TextBox.h"
#include "../Game/Game.h"
#include "../Math/Rect.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"

MessageBoxSubPanel::MessageBoxSubPanel(Game &game, MessageBoxSubPanel::Title &&title,
	std::vector<MessageBoxSubPanel::Element> &&elements,
	const std::function<void(Game&)> &cancelFunction)
	: Panel(game), title(std::move(title)), elements(std::move(elements)),
	cancelFunction(cancelFunction) { }

MessageBoxSubPanel::MessageBoxSubPanel(Game &game, MessageBoxSubPanel::Title &&title,
	std::vector<MessageBoxSubPanel::Element> &&elements)
	: Panel(game), title(std::move(title)), elements(std::move(elements)),
	cancelFunction([](Game&) {}) { }

Panel::CursorData MessageBoxSubPanel::getCurrentCursor() const
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

void MessageBoxSubPanel::handleEvent(const SDL_Event &e)
{
	auto &game = this->getGame();
	const auto &inputManager = this->getGame().getInputManager();
	const bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	const bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
	const bool rightClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_RIGHT);

	if (escapePressed || rightClick)
	{
		this->cancelFunction(game);
	}
	else if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 originalPoint = this->getGame().getRenderer()
			.nativeToOriginal(mousePosition);

		// See if any buttons were clicked.
		for (const auto &element : this->elements)
		{
			const Rect elementRect(
				element.textureX,
				element.textureY,
				element.texture.getWidth(),
				element.texture.getHeight());

			if (elementRect.contains(originalPoint))
			{
				element.function(game);
			}
		}
	}

	// @todo: custom hotkeys.
}

void MessageBoxSubPanel::render(Renderer &renderer)
{
	// Draw title.
	renderer.drawOriginal(this->title.texture,
		this->title.textureX, this->title.textureY);
	renderer.drawOriginal(this->title.textBox->getTexture(),
		this->title.textBox->getX(), this->title.textBox->getY());

	// Draw elements.
	for (const auto &element : this->elements)
	{
		renderer.drawOriginal(element.texture,
			element.textureX, element.textureY);
		renderer.drawOriginal(element.textBox->getTexture(),
			element.textBox->getX(), element.textBox->getY());
	}
}
