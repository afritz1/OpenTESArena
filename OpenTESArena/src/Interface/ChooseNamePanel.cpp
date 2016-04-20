#include <cassert>
#include <iostream>

#include "SDL2\SDL.h"

#include "ChooseNamePanel.h"

#include "Button.h"
#include "ChooseClassPanel.h"
#include "ChooseRacePanel.h"
#include "TextBox.h"
#include "../Entities/CharacterClassName.h"
#include "../Entities/CharacterGenderName.h"
#include "../Game/GameState.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/FontName.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"

ChooseNamePanel::ChooseNamePanel(GameState *gameState, CharacterGenderName gender, 
	CharacterClassName className)
	: Panel(gameState)
{
	this->titleTextBox = nullptr;
	this->backToClassButton = nullptr;
	this->acceptButton = nullptr;
	this->gender = nullptr;
	this->className = nullptr;

	this->titleTextBox = [gameState]()
	{
		auto origin = Int2(20, 80);
		auto color = Color::White;
		std::string text = "Thou shall be known as \"Player\".\n\nPress enter.";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			origin.getX(),
			origin.getY(),
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->backToClassButton = [gameState, gender]()
	{
		auto function = [gameState, gender]()
		{
			auto classPanel = std::unique_ptr<Panel>(
				new ChooseClassPanel(gameState, gender));
			gameState->setPanel(std::move(classPanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	// Somehow pass the entered string to the next panel constructor. 
	// Maybe "this->getNameString()"?
	this->acceptButton = [gameState, gender, className]()
	{
		auto function = [gameState, gender, className]()
		{
			auto racePanel = std::unique_ptr<Panel>(new ChooseRacePanel(
				gameState, gender, className, "Player"));
			gameState->setPanel(std::move(racePanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	this->gender = std::unique_ptr<CharacterGenderName>(
		new CharacterGenderName(gender));
	this->className = std::unique_ptr<CharacterClassName>(
		new CharacterClassName(className));

	assert(this->titleTextBox.get() != nullptr);
	assert(this->backToClassButton.get() != nullptr);
	assert(this->acceptButton.get() != nullptr);
	assert(this->gender.get() != nullptr);
	assert(*this->gender.get() == gender);
	assert(this->className.get() != nullptr);
	assert(*this->className.get() == className);
}

ChooseNamePanel::~ChooseNamePanel()
{

}

void ChooseNamePanel::handleEvents(bool &running)
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
			// Back to class panel.
			this->backToClassButton->click();
		}

		bool enterPressed = (e.type == SDL_KEYDOWN) &&
			((e.key.keysym.sym == SDLK_RETURN) ||
				(e.key.keysym.sym == SDLK_KP_ENTER));

		if (enterPressed)
		{
			// if entered name is valid, continue.
			this->acceptButton->click();

			// otherwise, do nothing? Maybe all names are valid.
		}
		
		// Arrow keys too advanced for now! They're just convenience features.

		// If (asciiCharacter or space is pressed) then push it onto the string.
		// If (Backspace is pressed) then delete one off not at the start.

		// If (enter is pressed) then try and accept the string.
	}
}

void ChooseNamePanel::handleMouse(double dt)
{
	static_cast<void>(dt);
}

void ChooseNamePanel::handleKeyboard(double dt)
{
	// In theory, holding down a key for a time WILL make it repeat eventually,
	// but that's too advanced for here.
	static_cast<void>(dt);
}

void ChooseNamePanel::tick(double dt, bool &running)
{
	static_cast<void>(dt);

	this->handleEvents(running);
}

void ChooseNamePanel::render(SDL_Surface *dst, const SDL_Rect *letterbox)
{
	// Clear full screen.
	this->clearScreen(dst);

	// Draw background.
	const auto &background = this->getGameState()->getTextureManager()
		.getSurface(TextureName::CharacterCreation);
	this->drawLetterbox(background, dst, letterbox);

	// Draw text: title.
	this->drawScaledToNative(*this->titleTextBox.get(), dst);

	// Draw cursor.
	const auto &cursor = this->getGameState()->getTextureManager()
		.getSurface(TextureName::SwordCursor);
	this->drawCursor(cursor, dst);
}
