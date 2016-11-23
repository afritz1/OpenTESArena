#include <cassert>

#include "SDL.h"

#include "AutomapPanel.h"

#include "Button.h"
#include "GameWorldPanel.h"
#include "TextBox.h"
#include "../Game/Game.h"
#include "../Math/Int2.h"
#include "../Math/Rect.h"
#include "../Media/FontManager.h"
#include "../Media/FontName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Texture.h"

namespace
{
	// Click areas for compass directions.
	const Rect UpRegion(264, 23, 14, 14);
	const Rect DownRegion(264, 60, 14, 14);
	const Rect LeftRegion(245, 41, 14, 14);
	const Rect RightRegion(284, 41, 14, 14);
}

AutomapPanel::AutomapPanel(Game *game)
	: Panel(game)
{
	this->backToGameButton = []()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH - 57, Renderer::ORIGINAL_HEIGHT - 29);
		int width = 38;
		int height = 13;
		auto function = [](Game *game)
		{
			std::unique_ptr<Panel> gamePanel(new GameWorldPanel(game));
			game->setPanel(std::move(gamePanel));
		};
		return std::unique_ptr<Button>(new Button(center, width, height, function));
	}();
}

AutomapPanel::~AutomapPanel()
{

}

void AutomapPanel::handleEvent(const SDL_Event &e)
{
	bool escapePressed = (e.type == SDL_KEYDOWN) &&
		(e.key.keysym.sym == SDLK_ESCAPE);
	bool nPressed = (e.type == SDL_KEYDOWN) &&
		(e.key.keysym.sym == SDLK_n);

	if (escapePressed || nPressed)
	{
		this->backToGameButton->click(this->getGame());
	}

	bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
		(e.button.button == SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = this->getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame()->getRenderer()
			.nativePointToOriginal(mousePosition);

		// Check if "Exit" was clicked.
		if (this->backToGameButton->contains(mouseOriginalPoint))
		{
			this->backToGameButton->click(this->getGame());
		}
	}
}

void AutomapPanel::handleMouse(double dt)
{
	// Check if the left mouse button is held on one of the compass directions.
	static_cast<void>(dt);
}

void AutomapPanel::drawTooltip(const std::string &text, Renderer &renderer)
{
	const Font &font = this->getGame()->getFontManager().getFont(FontName::D);

	Texture tooltip(Panel::createTooltip(text, font, renderer));

	const Int2 mousePosition = this->getMousePosition();
	const Int2 originalPosition = renderer.nativePointToOriginal(mousePosition);
	const int mouseX = originalPosition.getX();
	const int mouseY = originalPosition.getY();
	const int x = ((mouseX + 8 + tooltip.getWidth()) < Renderer::ORIGINAL_WIDTH) ?
		(mouseX + 8) : (mouseX - tooltip.getWidth());
	const int y = ((mouseY + tooltip.getHeight()) < Renderer::ORIGINAL_HEIGHT) ?
		(mouseY - 1) : (mouseY - tooltip.getHeight());

	renderer.drawToOriginal(tooltip.get(), x, y);
}

void AutomapPanel::tick(double dt)
{
	this->handleMouse(dt);
}

void AutomapPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clearNative();
	renderer.clearOriginal();

	// Set palette.
	auto &textureManager = this->getGame()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw automap background.
	const auto &automapBackground = textureManager.getTexture(
		TextureFile::fromName(TextureName::Automap),
		PaletteFile::fromName(PaletteName::BuiltIn));
	renderer.drawToOriginal(automapBackground.get());

	// Check if the mouse is over the compass directions for tooltips.
	const Int2 originalPosition = renderer.nativePointToOriginal(this->getMousePosition());

	if (UpRegion.contains(originalPosition))
	{
		this->drawTooltip("Up", renderer);
	}
	else if (DownRegion.contains(originalPosition))
	{
		this->drawTooltip("Down", renderer);
	}
	else if (LeftRegion.contains(originalPosition))
	{
		this->drawTooltip("Left", renderer);
	}
	else if (RightRegion.contains(originalPosition))
	{
		this->drawTooltip("Right", renderer);
	}

	// Scale the original frame buffer onto the native one.
	renderer.drawOriginalToNative();

	// Draw quill cursor. This one uses a different point for blitting because 
	// the tip of the cursor is at the bottom left, not the top left.
	const auto &cursor = textureManager.getTexture(
		TextureFile::fromName(TextureName::QuillCursor),
		TextureFile::fromName(TextureName::Automap));
	const int cursorYOffset = static_cast<int>(
		static_cast<double>(cursor.getHeight()) * this->getCursorScale());
	const auto mousePosition = this->getMousePosition();
	renderer.drawToNative(cursor.get(),
		mousePosition.getX(),
		mousePosition.getY() - cursorYOffset,
		static_cast<int>(cursor.getWidth() * this->getCursorScale()),
		static_cast<int>(cursor.getHeight() * this->getCursorScale()));
}
