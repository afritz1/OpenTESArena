#include <cassert>

#include "SDL.h"

#include "ChooseGenderPanel.h"

#include "Button.h"
#include "ChooseNamePanel.h"
#include "ChooseRacePanel.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Entities/CharacterClass.h"
#include "../Entities/CharacterGenderName.h"
#include "../Game/GameState.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/FontManager.h"
#include "../Media/FontName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"

ChooseGenderPanel::ChooseGenderPanel(GameState *gameState, const CharacterClass &charClass,
	const std::string &name)
	: Panel(gameState)
{
	this->parchment = [gameState]()
	{
		auto &renderer = gameState->getRenderer();

		// Create placeholder parchment.
		SDL_Surface *surface = Surface::createSurfaceWithFormat(180, 40,
			Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);
		SDL_FillRect(surface, nullptr, SDL_MapRGBA(surface->format, 166, 125, 81, 255));

		SDL_Texture *texture = renderer.createTextureFromSurface(surface);
		SDL_FreeSurface(surface);

		return texture;
	}();

	this->genderTextBox = [gameState]()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH / 2, 80);
		Color color(48, 12, 12);
		std::string text = "Choose thy gender...";
		auto &font = gameState->getFontManager().getFont(FontName::A);
		auto alignment = TextAlignment::Center;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			font,
			alignment,
			gameState->getRenderer()));
	}();

	this->maleTextBox = [gameState]()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH / 2, 120);
		Color color(48, 12, 12);
		std::string text = "Male";
		auto &font = gameState->getFontManager().getFont(FontName::A);
		auto alignment = TextAlignment::Center;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			font,
			alignment,
			gameState->getRenderer()));
	}();

	this->femaleTextBox = [gameState]()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH / 2, 160);
		Color color(48, 12, 12);
		std::string text = "Female";
		auto &font = gameState->getFontManager().getFont(FontName::A);
		auto alignment = TextAlignment::Center;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			font,
			alignment,
			gameState->getRenderer()));
	}();

	this->backToNameButton = [charClass]()
	{
		auto function = [charClass](GameState *gameState)
		{
			gameState->setPanel(std::unique_ptr<Panel>(new ChooseNamePanel(
				gameState, charClass)));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	this->maleButton = [charClass, name]()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH / 2, 120);
		auto function = [charClass, name](GameState *gameState)
		{
			std::unique_ptr<Panel> classPanel(new ChooseRacePanel(
				gameState, charClass, name, CharacterGenderName::Male));
			gameState->setPanel(std::move(classPanel));
		};
		return std::unique_ptr<Button>(new Button(center, 175, 35, function));
	}();

	this->femaleButton = [charClass, name]()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH / 2, 160);
		auto function = [charClass, name](GameState *gameState)
		{
			std::unique_ptr<Panel> classPanel(new ChooseRacePanel(
				gameState, charClass, name, CharacterGenderName::Female));
			gameState->setPanel(std::move(classPanel));
		};
		return std::unique_ptr<Button>(new Button(center, 175, 35, function));
	}();

	this->charClass = std::unique_ptr<CharacterClass>(new CharacterClass(charClass));
	this->name = name;
}

ChooseGenderPanel::~ChooseGenderPanel()
{
	SDL_DestroyTexture(this->parchment);
}

void ChooseGenderPanel::handleEvents(bool &running)
{
	auto mousePosition = this->getMousePosition();
	auto mouseOriginalPoint = this->getGameState()->getRenderer()
		.nativePointToOriginal(mousePosition);

	SDL_Event e;
	while (SDL_PollEvent(&e) != 0)
	{
		bool applicationExit = (e.type == SDL_QUIT);
		bool resized = (e.type == SDL_WINDOWEVENT) &&
			(e.window.event == SDL_WINDOWEVENT_RESIZED);
		bool escapePressed = (e.type == SDL_KEYDOWN) &&
			(e.key.keysym.sym == SDLK_ESCAPE);

		if (applicationExit)
		{
			running = false;
		}
		if (resized)
		{
			int width = e.window.data1;
			int height = e.window.data2;
			this->getGameState()->resizeWindow(width, height);
		}
		if (escapePressed)
		{
			this->backToNameButton->click(this->getGameState());
		}

		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);

		bool maleClicked = leftClick &&
			this->maleButton->containsPoint(mouseOriginalPoint);
		bool femaleClicked = leftClick &&
			this->femaleButton->containsPoint(mouseOriginalPoint);

		if (maleClicked)
		{
			this->maleButton->click(this->getGameState());
		}
		else if (femaleClicked)
		{
			this->femaleButton->click(this->getGameState());
		}
	}
}

void ChooseGenderPanel::handleMouse(double dt)
{
	static_cast<void>(dt);
}

void ChooseGenderPanel::handleKeyboard(double dt)
{
	static_cast<void>(dt);
}

void ChooseGenderPanel::tick(double dt, bool &running)
{
	static_cast<void>(dt);

	this->handleEvents(running);
}

void ChooseGenderPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clearNative();
	renderer.clearOriginal();

	// Set palette.
	auto &textureManager = this->getGameState()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw background.
	auto *background = textureManager.getTexture(
		TextureFile::fromName(TextureName::CharacterCreation), 
		PaletteFile::fromName(PaletteName::BuiltIn));
	renderer.drawToOriginal(background);

	// Draw parchments: title, male, and female.
	int parchmentWidth, parchmentHeight;
	SDL_QueryTexture(this->parchment, nullptr, nullptr, &parchmentWidth, &parchmentHeight);
	int parchmentX = (Renderer::ORIGINAL_WIDTH / 2) - (parchmentWidth / 2);
	int parchmentY = (Renderer::ORIGINAL_HEIGHT / 2) - (parchmentHeight / 2) - 20;
	renderer.drawToOriginal(this->parchment, parchmentX, parchmentY);
	renderer.drawToOriginal(this->parchment, parchmentX, parchmentY + 40);
	renderer.drawToOriginal(this->parchment, parchmentX, parchmentY + 80);

	// Draw text: title, male, and female.
	renderer.drawToOriginal(this->genderTextBox->getTexture(),
		this->genderTextBox->getX(), this->genderTextBox->getY());
	renderer.drawToOriginal(this->maleTextBox->getTexture(),
		this->maleTextBox->getX(), this->maleTextBox->getY());
	renderer.drawToOriginal(this->femaleTextBox->getTexture(),
		this->femaleTextBox->getX(), this->femaleTextBox->getY());

	// Scale the original frame buffer onto the native one.
	renderer.drawOriginalToNative();

	// Draw cursor.
	const auto &cursor = textureManager.getSurface(
		TextureFile::fromName(TextureName::SwordCursor));
	auto mousePosition = this->getMousePosition();
	renderer.drawToNative(cursor.getSurface(),
		mousePosition.getX(), mousePosition.getY(),
		static_cast<int>(cursor.getWidth() * this->getCursorScale()),
		static_cast<int>(cursor.getHeight() * this->getCursorScale()));
}
