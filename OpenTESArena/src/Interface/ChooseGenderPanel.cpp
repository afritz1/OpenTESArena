#include <cassert>
#include <iostream>

#include "SDL.h"

#include "ChooseGenderPanel.h"

#include "Button.h"
#include "ChooseNamePanel.h"
#include "ChooseRacePanel.h"
#include "TextBox.h"
#include "../Entities/CharacterClass.h"
#include "../Entities/CharacterGenderName.h"
#include "../Game/GameState.h"
#include "../Math/Constants.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/FontName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"

ChooseGenderPanel::ChooseGenderPanel(GameState *gameState, const CharacterClass &charClass,
	const std::string &name)
	: Panel(gameState)
{
	this->parchment = [gameState]()
	{
		auto *surface = gameState->getTextureManager().getSurface(
			TextureFile::fromName(TextureName::ParchmentPopup)).getSurface();
		auto origin = Int2((ORIGINAL_WIDTH / 2) - (surface->w / 2), 35);
		return std::unique_ptr<Surface>(new Surface(
			origin.getX(), origin.getY(), surface));
	}();

	this->genderTextBox = [gameState]()
	{
		auto center = Int2(160, 56);
		auto color = Color(48, 12, 12);
		std::string text = "Choose thy gender...";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->maleTextBox = [gameState]()
	{
		auto center = Int2(160, 106);
		auto color = Color(48, 12, 12);
		std::string text = "Male";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->femaleTextBox = [gameState]()
	{
		auto center = Int2(160, 146);
		auto color = Color(48, 12, 12);
		std::string text = "Female";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->backToNameButton = [gameState, charClass]()
	{
		auto function = [gameState, charClass]()
		{
			gameState->setPanel(std::unique_ptr<Panel>(new ChooseNamePanel(
				gameState, charClass)));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	this->maleButton = [gameState, charClass, name]()
	{
		auto center = Int2(160, 105);
		auto function = [gameState, charClass, name]()
		{
			auto classPanel = std::unique_ptr<Panel>(new ChooseRacePanel(
				gameState, charClass, name, CharacterGenderName::Male));
			gameState->setPanel(std::move(classPanel));
		};
		return std::unique_ptr<Button>(new Button(center, 120, 30, function));
	}();

	this->femaleButton = [gameState, charClass, name]()
	{
		auto center = Int2(160, 145);
		auto function = [gameState, charClass, name]()
		{
			auto classPanel = std::unique_ptr<Panel>(new ChooseRacePanel(
				gameState, charClass, name, CharacterGenderName::Female));
			gameState->setPanel(std::move(classPanel));
		};
		return std::unique_ptr<Button>(new Button(center, 120, 30, function));
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
	auto mouseOriginalPoint = this->nativePointToOriginal(mousePosition);

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
			this->backToNameButton->click();
		}

		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);

		bool maleClicked = leftClick &&
			this->maleButton->containsPoint(mouseOriginalPoint);
		bool femaleClicked = leftClick &&
			this->femaleButton->containsPoint(mouseOriginalPoint);

		if (maleClicked)
		{
			this->maleButton->click();
		}
		else if (femaleClicked)
		{
			this->femaleButton->click();
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

void ChooseGenderPanel::render(SDL_Surface *dst, const SDL_Rect *letterbox)
{
	// Clear full screen.
	this->clearScreen(dst);

	// Draw background.
	const auto &background = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::CharacterCreation));
	this->drawLetterbox(background, dst, letterbox);

	// Draw parchments: title, male, and female.
	this->parchment->setTransparentColor(Color::Magenta);
	this->drawScaledToNative(*this->parchment.get(), dst);

	const double parchmentScale = 0.70;
	int parchmentXOffset = 27;
	int parchmentYStep = 40;
	int parchmentYOffset = 10 + parchmentYStep;
	this->drawScaledToNative(*this->parchment.get(),
		this->parchment->getX() + parchmentXOffset,
		this->parchment->getY() + parchmentYOffset,
		static_cast<int>(this->parchment->getWidth() * parchmentScale),
		this->parchment->getHeight(),
		dst);

	parchmentYOffset = 10 + (parchmentYStep * 2);
	this->drawScaledToNative(*this->parchment.get(),
		this->parchment->getX() + parchmentXOffset,
		this->parchment->getY() + parchmentYOffset,
		static_cast<int>(this->parchment->getWidth() * parchmentScale),
		this->parchment->getHeight(),
		dst);

	// Draw text: title, male, and female.
	this->drawScaledToNative(*this->genderTextBox.get(), dst);
	this->drawScaledToNative(*this->maleTextBox.get(), dst);
	this->drawScaledToNative(*this->femaleTextBox.get(), dst);

	// Draw cursor.
	const auto &cursor = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::SwordCursor));
	this->drawCursor(cursor, dst);
}
