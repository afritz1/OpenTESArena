#include <algorithm>
#include <cassert>
#include <iomanip>
#include <sstream>

#include "SDL.h"

#include "OptionsPanel.h"

#include "PauseMenuPanel.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/FontManager.h"
#include "../Media/FontName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Texture.h"

const std::string OptionsPanel::FPS_TEXT = "FPS Limit: ";
const std::string OptionsPanel::RESOLUTION_SCALE_TEXT = "Resolution Scale: ";

OptionsPanel::OptionsPanel(Game *game)
	: Panel(game)
{
	this->titleTextBox = [game]()
	{
		Int2 center(160, 30);
		auto color = Color::White;
		std::string text("Options");
		auto &font = game->getFontManager().getFont(FontName::A);
		auto alignment = TextAlignment::Center;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			font,
			alignment,
			game->getRenderer()));
	}();

	this->backToPauseTextBox = [game]()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH - 30, Renderer::ORIGINAL_HEIGHT - 15);
		auto color = Color::White;
		std::string text("Return");
		auto &font = game->getFontManager().getFont(FontName::Arena);
		auto alignment = TextAlignment::Center;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			font,
			alignment,
			game->getRenderer()));
	}();

	this->fpsTextBox = [game]()
	{
		int x = 20;
		int y = 45;
		auto color = Color::White;
		std::string text(OptionsPanel::FPS_TEXT +
			std::to_string(game->getOptions().getTargetFPS()));
		auto &font = game->getFontManager().getFont(FontName::Arena);
		auto alignment = TextAlignment::Left;
		return std::unique_ptr<TextBox>(new TextBox(
			x,
			y,
			color,
			text,
			font,
			alignment,
			game->getRenderer()));
	}();

	this->resolutionScaleTextBox = [game]()
	{
		int x = 20;
		int y = 65;
		auto color = Color::White;

		std::stringstream ss;
		ss << OptionsPanel::RESOLUTION_SCALE_TEXT;
		ss << std::fixed << std::setprecision(2) << game->getOptions().getResolutionScale();

		const std::string text = ss.str();
		auto &font = game->getFontManager().getFont(FontName::Arena);
		auto alignment = TextAlignment::Left;
		return std::unique_ptr<TextBox>(new TextBox(
			x,
			y,
			color,
			text,
			font,
			alignment,
			game->getRenderer()));
	}();

	this->backToPauseButton = []()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH - 30, Renderer::ORIGINAL_HEIGHT - 15);
		auto function = [](Game *game)
		{
			std::unique_ptr<Panel> pausePanel(new PauseMenuPanel(game));
			game->setPanel(std::move(pausePanel));
		};
		return std::unique_ptr<Button<Game*>>(new Button<Game*>(
			center, 40, 16, function));
	}();

	this->fpsUpButton = []()
	{
		int x = 85;
		int y = 41;
		int width = 8;
		int height = 8;
		auto function = [](OptionsPanel *panel, Options &options)
		{
			const int newFPS = options.getTargetFPS() + 5;
			options.setTargetFPS(newFPS);
			panel->updateFPSText(newFPS);
		};
		return std::unique_ptr<Button<OptionsPanel*, Options&>>(
			new Button<OptionsPanel*, Options&>(x, y, width, height, function));
	}();

	this->fpsDownButton = [this]()
	{
		int x = this->fpsUpButton->getX();
		int y = this->fpsUpButton->getY() + this->fpsUpButton->getHeight();
		int width = this->fpsUpButton->getWidth();
		int height = this->fpsUpButton->getHeight();
		auto function = [](OptionsPanel *panel, Options &options)
		{
			const int newFPS = std::max(options.getTargetFPS() - 5, options.MIN_FPS);
			options.setTargetFPS(newFPS);
			panel->updateFPSText(newFPS);
		};
		return std::unique_ptr<Button<OptionsPanel*, Options&>>(
			new Button<OptionsPanel*, Options&>(x, y, width, height, function));
	}();

	this->resolutionScaleUpButton = []()
	{
		int x = 120;
		int y = 61;
		int width = 8;
		int height = 8;
		auto function = [](OptionsPanel *panel, Options &options, Renderer &renderer)
		{
			const double newResolutionScale = std::min(
				options.getResolutionScale() + 0.05, options.MAX_RESOLUTION_SCALE);
			options.setResolutionScale(newResolutionScale);
			panel->updateResolutionScaleText(newResolutionScale);

			// Resize the game world rendering.
			const Int2 windowDimensions = renderer.getWindowDimensions();
			renderer.resize(windowDimensions.x, windowDimensions.y, newResolutionScale);
		};
		return std::unique_ptr<Button<OptionsPanel*, Options&, Renderer&>>(
			new Button<OptionsPanel*, Options&, Renderer&>(x, y, width, height, function));
	}();

	this->resolutionScaleDownButton = [this]()
	{
		int x = this->resolutionScaleUpButton->getX();
		int y = this->resolutionScaleUpButton->getY() + this->resolutionScaleUpButton->getHeight();
		int width = this->resolutionScaleUpButton->getWidth();
		int height = this->resolutionScaleUpButton->getHeight();
		auto function = [](OptionsPanel *panel, Options &options, Renderer &renderer)
		{
			const double newResolutionScale = std::max(
				options.getResolutionScale() - 0.05, options.MIN_RESOLUTION_SCALE);
			options.setResolutionScale(newResolutionScale);
			panel->updateResolutionScaleText(newResolutionScale);

			// Resize the game world rendering.
			const Int2 windowDimensions = renderer.getWindowDimensions();
			renderer.resize(windowDimensions.x, windowDimensions.y, newResolutionScale);
		};
		return std::unique_ptr<Button<OptionsPanel*, Options&, Renderer&>>(
			new Button<OptionsPanel*, Options&, Renderer&>(x, y, width, height, function));
	}();
}

OptionsPanel::~OptionsPanel()
{

}

void OptionsPanel::updateFPSText(int fps)
{
	assert(this->fpsTextBox.get() != nullptr);

	this->fpsTextBox = [this, fps]()
	{
		std::string text(OptionsPanel::FPS_TEXT + std::to_string(fps));
		auto &fontManager = this->getGame()->getFontManager();

		return std::unique_ptr<TextBox>(new TextBox(
			this->fpsTextBox->getX(),
			this->fpsTextBox->getY(),
			this->fpsTextBox->getTextColor(),
			text,
			fontManager.getFont(this->fpsTextBox->getFontName()),
			this->fpsTextBox->getAlignment(),
			this->getGame()->getRenderer()));
	}();
}

void OptionsPanel::updateResolutionScaleText(double resolutionScale)
{
	assert(this->resolutionScaleTextBox.get() != nullptr);

	this->resolutionScaleTextBox = [this, resolutionScale]()
	{
		std::stringstream ss;
		ss << OptionsPanel::RESOLUTION_SCALE_TEXT;
		ss << std::fixed << std::setprecision(2) << resolutionScale;

		const std::string text = ss.str();
		auto &fontManager = this->getGame()->getFontManager();

		return std::unique_ptr<TextBox>(new TextBox(
			this->resolutionScaleTextBox->getX(),
			this->resolutionScaleTextBox->getY(),
			this->resolutionScaleTextBox->getTextColor(),
			text,
			fontManager.getFont(this->resolutionScaleTextBox->getFontName()),
			this->resolutionScaleTextBox->getAlignment(),
			this->getGame()->getRenderer()));
	}();
}

void OptionsPanel::handleEvent(const SDL_Event &e)
{
	bool escapePressed = (e.type == SDL_KEYDOWN) &&
		(e.key.keysym.sym == SDLK_ESCAPE);

	if (escapePressed)
	{
		this->backToPauseButton->click(this->getGame());
	}

	bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
		(e.button.button == SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = this->getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame()->getRenderer()
			.nativePointToOriginal(mousePosition);

		// Check for various button clicks.
		if (this->fpsUpButton->contains(mouseOriginalPoint))
		{
			this->fpsUpButton->click(this, this->getGame()->getOptions());
		}
		else if (this->fpsDownButton->contains(mouseOriginalPoint))
		{
			this->fpsDownButton->click(this, this->getGame()->getOptions());
		}
		else if (this->resolutionScaleUpButton->contains(mouseOriginalPoint))
		{
			this->resolutionScaleUpButton->click(this, this->getGame()->getOptions(),
				this->getGame()->getRenderer());
		}
		else if (this->resolutionScaleDownButton->contains(mouseOriginalPoint))
		{
			this->resolutionScaleDownButton->click(this, this->getGame()->getOptions(),
				this->getGame()->getRenderer());
		}
		else if (this->backToPauseButton->contains(mouseOriginalPoint))
		{
			this->backToPauseButton->click(this->getGame());
		}
	}
}

void OptionsPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clearNative();
	renderer.clearOriginal();

	// Set palette.
	auto &textureManager = this->getGame()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw solid background.
	renderer.clearOriginal(Color(70, 70, 78));

	// Draw buttons.
	const auto &arrows = textureManager.getTexture(
		TextureFile::fromName(TextureName::UpDown),
		PaletteFile::fromName(PaletteName::CharSheet));
	renderer.drawToOriginal(arrows.get(), this->fpsUpButton->getX(),
		this->fpsUpButton->getY());
	renderer.drawToOriginal(arrows.get(), this->resolutionScaleUpButton->getX(),
		this->resolutionScaleUpButton->getY());

	Texture returnBackground(Texture::generate(Texture::PatternType::Custom1,
		this->backToPauseButton->getWidth(), this->backToPauseButton->getHeight(),
		textureManager, renderer));
	renderer.drawToOriginal(returnBackground.get(), this->backToPauseButton->getX(),
		this->backToPauseButton->getY());

	// Draw text: title, return, fps.
	renderer.drawToOriginal(this->titleTextBox->getTexture(),
		this->titleTextBox->getX(), this->titleTextBox->getY());
	renderer.drawToOriginal(this->backToPauseTextBox->getTexture(),
		this->backToPauseTextBox->getX(), this->backToPauseTextBox->getY());
	renderer.drawToOriginal(this->fpsTextBox->getTexture(),
		this->fpsTextBox->getX(), this->fpsTextBox->getY());
	renderer.drawToOriginal(this->resolutionScaleTextBox->getTexture(),
		this->resolutionScaleTextBox->getX(), this->resolutionScaleTextBox->getY());

	// Scale the original frame buffer onto the native one.
	renderer.drawOriginalToNative();

	// Draw cursor.
	const auto &cursor = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor));
	auto mousePosition = this->getMousePosition();
	renderer.drawToNative(cursor.get(),
		mousePosition.x, mousePosition.y,
		static_cast<int>(cursor.getWidth() * this->getCursorScale()),
		static_cast<int>(cursor.getHeight() * this->getCursorScale()));
}
