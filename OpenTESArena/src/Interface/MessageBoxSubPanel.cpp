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

MessageBoxSubPanel::~MessageBoxSubPanel()
{

}

std::pair<SDL_Texture*, CursorAlignment> MessageBoxSubPanel::getCurrentCursor() const
{
	auto &textureManager = this->getGame().getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor),
		PaletteFile::fromName(PaletteName::Default));
	return std::make_pair(texture.get(), CursorAlignment::TopLeft);
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

	// To do: custom hotkeys.
}

void MessageBoxSubPanel::render(Renderer &renderer)
{
	// Draw title.
	renderer.drawOriginal(this->title.texture.get(),
		this->title.textureX, this->title.textureY);
	renderer.drawOriginal(this->title.textBox->getTexture(),
		this->title.textBox->getX(), this->title.textBox->getY());

	// Draw elements.
	for (const auto &element : this->elements)
	{
		renderer.drawOriginal(element.texture.get(),
			element.textureX, element.textureY);
		renderer.drawOriginal(element.textBox->getTexture(),
			element.textBox->getX(), element.textBox->getY());
	}
}
