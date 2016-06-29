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
		const auto *surface = gameState->getTextureManager().getSurface(
			TextureFile::fromName(TextureName::ParchmentPopup)).getSurface();
		Int2 origin((ORIGINAL_WIDTH / 2) - (surface->w / 2), 35);
		return std::unique_ptr<Surface>(new Surface(origin.getX(), origin.getY(), surface));
	}();

	this->genderTextBox = [gameState]()
	{
		Int2 center((ORIGINAL_WIDTH / 2), 80);
		Color color(48, 12, 12);
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
		Int2 center((ORIGINAL_WIDTH / 2), 120);
		Color color(48, 12, 12);
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
		Int2 center((ORIGINAL_WIDTH / 2), 160);
		Color color(48, 12, 12);
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
		Int2 center((ORIGINAL_WIDTH / 2), 120);
		auto function = [gameState, charClass, name]()
		{
			auto classPanel = std::unique_ptr<Panel>(new ChooseRacePanel(
				gameState, charClass, name, CharacterGenderName::Male));
			gameState->setPanel(std::move(classPanel));
		};
		return std::unique_ptr<Button>(new Button(center, 175, 35, function));
	}();

	this->femaleButton = [gameState, charClass, name]()
	{
		Int2 center((ORIGINAL_WIDTH / 2), 160);
		auto function = [gameState, charClass, name]()
		{
			auto classPanel = std::unique_ptr<Panel>(new ChooseRacePanel(
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

void ChooseGenderPanel::render(SDL_Renderer *renderer, const SDL_Rect *letterbox)
{
	// Clear full screen.
	this->clearScreen(renderer);

	// Draw background.
	const auto *background = this->getGameState()->getTextureManager()
		.getTexture(TextureFile::fromName(TextureName::CharacterCreation));
	this->drawLetterbox(background, renderer, letterbox);

	// Draw parchments: title, male, and female.
	this->parchment->setTransparentColor(Color::Magenta);

	double parchmentXScale = 1.0;
	double parchmentYScale = 1.0;
	int parchmentWidth = static_cast<int>(this->parchment->getWidth() * parchmentXScale);
	int parchmentHeight = static_cast<int>(this->parchment->getHeight() * parchmentYScale);
	int parchmentX = (ORIGINAL_WIDTH / 2) - (parchmentWidth / 2);
	int parchmentY = (ORIGINAL_HEIGHT / 2) - (parchmentHeight / 2) - 20;
	this->drawScaledToNative(*this->parchment.get(),
		parchmentX,
		parchmentY,
		parchmentWidth,
		parchmentHeight,
		renderer);

	parchmentY += 40;
	this->drawScaledToNative(*this->parchment.get(),
		parchmentX,
		parchmentY,
		parchmentWidth,
		parchmentHeight,
		renderer);

	parchmentY += 40;
	this->drawScaledToNative(*this->parchment.get(),
		parchmentX,
		parchmentY,
		parchmentWidth,
		parchmentHeight,
		renderer);

	// Draw text: title, male, and female.
	this->drawScaledToNative(*this->genderTextBox.get(), renderer);
	this->drawScaledToNative(*this->maleTextBox.get(), renderer);
	this->drawScaledToNative(*this->femaleTextBox.get(), renderer);

	// Draw cursor.
	const auto &cursor = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::SwordCursor));
	this->drawCursor(cursor, renderer);
}
