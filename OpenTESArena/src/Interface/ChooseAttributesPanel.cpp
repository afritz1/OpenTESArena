#include <cassert>
#include <iostream>

#include "SDL2\SDL.h"

#include "ChooseAttributesPanel.h"

#include "Button.h"
#include "ChooseRacePanel.h"
#include "GameWorldPanel.h"
#include "TextBox.h"
#include "../Game/GameState.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/MusicName.h"
#include "../Media/FontName.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"

ChooseAttributesPanel::ChooseAttributesPanel(GameState *gameState, 
	CharacterGenderName gender, CharacterClassName className, 
	const std::string &name, CharacterRaceName raceName)
	: Panel(gameState)
{
	this->titleTextBox = nullptr;
	this->backToRaceButton = nullptr;
	this->acceptButton = nullptr;
	this->gender = nullptr;
	this->className = nullptr;
	this->raceName = nullptr;

	this->titleTextBox = [gameState]()
	{
		auto origin = Int2(20, 80);
		auto color = Color::White;
		std::string text = "Thy attributes will soon be here. \nLeft click.";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			origin.getX(),
			origin.getY(),
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->backToRaceButton = [gameState, gender, className, name]()
	{
		auto function = [gameState, gender, className, name]()
		{
			auto racePanel = std::unique_ptr<Panel>(new ChooseRacePanel(
				gameState, gender, className, name));
			gameState->setPanel(std::move(racePanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	// Set the game world's music to be some default for now.
	// None of the gender, class name, etc. things are being sent to the game world
	// panel because I'm planning on putting them in the GameState instead.
	this->acceptButton = [gameState]()
	{
		auto function = [gameState]()
		{
			auto gameWorldPanel = std::unique_ptr<Panel>(new GameWorldPanel(gameState));
			gameState->setMusic(MusicName::Magic);
			gameState->setPanel(std::move(gameWorldPanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	this->gender = std::unique_ptr<CharacterGenderName>(
		new CharacterGenderName(gender));
	this->className = std::unique_ptr<CharacterClassName>(
		new CharacterClassName(className));
	this->raceName = std::unique_ptr<CharacterRaceName>(
		new CharacterRaceName(raceName));
	this->name = name;
	
	assert(this->titleTextBox.get() != nullptr);
	assert(this->backToRaceButton.get() != nullptr);
	assert(this->acceptButton.get() != nullptr);
	assert(this->gender.get() != nullptr);
	assert(this->className.get() != nullptr);
	assert(this->raceName.get() != nullptr);
	assert(this->name == name);
}

ChooseAttributesPanel::~ChooseAttributesPanel()
{

}

void ChooseAttributesPanel::handleEvents(bool &running)
{
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
			this->backToRaceButton->click();
		}

		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);

		if (leftClick)
		{
			this->acceptButton->click();
		}
	}
}

void ChooseAttributesPanel::handleMouse(double dt)
{
	static_cast<void>(dt);
}

void ChooseAttributesPanel::handleKeyboard(double dt)
{
	static_cast<void>(dt);
}

void ChooseAttributesPanel::tick(double dt, bool &running)
{
	static_cast<void>(dt);

	this->handleEvents(running);
}

void ChooseAttributesPanel::render(SDL_Surface *dst, const SDL_Rect *letterbox)
{
	// Clear full screen.
	this->clearScreen(dst);
		
	// Draw temporary background. I don't have the marble background or character
	// portraits programmed in yet, but they will be, eventually.
	SDL_FillRect(dst, letterbox, SDL_MapRGB(dst->format, 32, 32, 64));

	// Draw text: title.
	this->drawScaledToNative(*this->titleTextBox.get(), dst);

	// Draw cursor.
	const auto &cursor = this->getGameState()->getTextureManager()
		.getSurface(TextureName::SwordCursor);
	this->drawCursor(cursor, dst);
}
