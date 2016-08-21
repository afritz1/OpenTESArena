#include <cassert>

#include "SDL.h"

#include "ChooseGenderPanel.h"

#include "Button.h"
#include "ChooseNamePanel.h"
#include "ChooseRacePanel.h"
#include "TextBox.h"
#include "../Entities/CharacterClass.h"
#include "../Entities/CharacterGenderName.h"
#include "../Game/GameState.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
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
		const auto *surface = gameState->getTextureManager().getSurface(
			TextureFile::fromName(TextureName::ParchmentPopup)).getSurface();
		Int2 origin((Renderer::ORIGINAL_WIDTH / 2) - (surface->w / 2), 35);
		return std::unique_ptr<Surface>(new Surface(origin.getX(), origin.getY(), surface));
	}();

	this->genderTextBox = [gameState]()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH / 2, 80);
		Color color(48, 12, 12);
		std::string text = "Choose thy gender...";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			gameState->getTextureManager(),
			gameState->getRenderer()));
	}();

	this->maleTextBox = [gameState]()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH / 2, 120);
		Color color(48, 12, 12);
		std::string text = "Male";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			gameState->getTextureManager(),
			gameState->getRenderer()));
	}();

	this->femaleTextBox = [gameState]()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH / 2, 160);
		Color color(48, 12, 12);
		std::string text = "Female";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			gameState->getTextureManager(),
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
			int32_t width = e.window.data1;
			int32_t height = e.window.data2;
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

	// Set palette.
	auto &textureManager = this->getGameState()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw background.
	auto *background = textureManager.getTexture(
		TextureFile::fromName(TextureName::CharacterCreation), 
		PaletteFile::fromName(PaletteName::BuiltIn));
	renderer.drawToOriginal(background);

	// Draw parchments: title, male, and female.
	this->parchment->setTransparentColor(Color::Magenta);

	int32_t parchmentX = (Renderer::ORIGINAL_WIDTH / 2) - (this->parchment->getWidth() / 2);
	int32_t parchmentY = (Renderer::ORIGINAL_HEIGHT / 2) - (this->parchment->getHeight() / 2) - 20;
	renderer.drawToOriginal(this->parchment->getSurface(), parchmentX, parchmentY);
	renderer.drawToOriginal(this->parchment->getSurface(), parchmentX, parchmentY + 40);
	renderer.drawToOriginal(this->parchment->getSurface(), parchmentX, parchmentY + 80);

	// Draw text: title, male, and female.
	renderer.drawToOriginal(this->genderTextBox->getSurface(),
		this->genderTextBox->getX(), this->genderTextBox->getY());
	renderer.drawToOriginal(this->maleTextBox->getSurface(),
		this->maleTextBox->getX(), this->maleTextBox->getY());
	renderer.drawToOriginal(this->femaleTextBox->getSurface(),
		this->femaleTextBox->getX(), this->femaleTextBox->getY());

	// Scale the original frame buffer onto the native one.
	renderer.drawOriginalToNative();

	// Draw cursor.
	const auto &cursor = textureManager.getSurface(
		TextureFile::fromName(TextureName::SwordCursor));
	SDL_SetColorKey(cursor.getSurface(), SDL_TRUE,
		renderer.getFormattedARGB(Color::Black));
	auto mousePosition = this->getMousePosition();
	renderer.drawToNative(cursor.getSurface(),
		mousePosition.getX(), mousePosition.getY(),
		static_cast<int32_t>(cursor.getWidth() * this->getCursorScale()),
		static_cast<int32_t>(cursor.getHeight() * this->getCursorScale()));
}
