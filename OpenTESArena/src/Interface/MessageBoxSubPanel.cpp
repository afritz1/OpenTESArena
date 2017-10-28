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

MessageBoxSubPanel::MessageBoxSubPanel(Game *game, std::unique_ptr<TextBox> textBox,
	Texture &&textBoxTexture, Texture &&buttonTexture,
	const std::vector<std::function<void(Game*)>> &functions)
	: Panel(game), textBox(std::move(textBox)), textBoxTexture(std::move(textBoxTexture)),
	buttonTexture(std::move(buttonTexture)), functions(functions) { }

MessageBoxSubPanel::~MessageBoxSubPanel()
{

}

std::pair<SDL_Texture*, CursorAlignment> MessageBoxSubPanel::getCurrentCursor() const
{
	auto &textureManager = this->getGame()->getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor),
		PaletteFile::fromName(PaletteName::Default));
	return std::make_pair(texture.get(), CursorAlignment::TopLeft);
}

void MessageBoxSubPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame()->getInputManager();
	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
	bool rightClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_RIGHT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame()->getRenderer()
			.nativePointToOriginal(mousePosition);

		// See if any of the buttons were clicked.
		const Rect textBoxRect = this->textBox->getRect();
		for (size_t i = 0; i < this->functions.size(); i++)
		{
			const int yOffset = static_cast<int>(textBoxRect.getHeight() * i);

			const Rect buttonRect(
				textBoxRect.getLeft(),
				textBoxRect.getBottom() + yOffset,
				textBoxRect.getWidth(),
				textBoxRect.getHeight());

			if (buttonRect.contains(mouseOriginalPoint))
			{
				auto &function = this->functions.at(i);
				function(this->getGame());
			}
		}
	}
}

void MessageBoxSubPanel::render(Renderer &renderer)
{
	// Draw textures.

	// Draw text.

}
