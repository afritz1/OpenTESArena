#include <cassert>

#include "SDL.h"

#include "ChooseClassCreationPanel.h"

#include "Button.h"
#include "ChooseClassPanel.h"
#include "MainMenuPanel.h"
#include "TextBox.h"
#include "../Game/GameState.h"
#include "../Math/Constants.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/FontName.h"
#include "../Media/MusicName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"

ChooseClassCreationPanel::ChooseClassCreationPanel(GameState *gameState)
	: Panel(gameState)
{
	this->parchment = [gameState]()
	{
		auto *surface = gameState->getTextureManager().getSurface(
			TextureFile::fromName(TextureName::ParchmentPopup)).getSurface();
		return std::unique_ptr<Surface>(new Surface(surface));
	}();

	this->titleTextBox = [gameState]()
	{
		auto center = Int2((ORIGINAL_WIDTH / 2), 80);
		auto color = Color(48, 12, 12);
		std::string text = std::string("How do you wish\nto select your class?");
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->generateTextBox = [gameState]()
	{
		auto center = Int2((ORIGINAL_WIDTH / 2), 120);
		auto color = Color(48, 12, 12);
		std::string text = std::string("Generate\n(not implemented)");
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->selectTextBox = [gameState]()
	{
		auto center = Int2(ORIGINAL_WIDTH / 2, 160);
		auto color = Color(48, 12, 12);
		std::string text = std::string("Select");
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->backToMainMenuButton = [gameState]()
	{
		auto function = [gameState]()
		{
			gameState->setPanel(std::unique_ptr<Panel>(new MainMenuPanel(gameState)));
			gameState->setMusic(MusicName::PercIntro);
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	this->generateButton = [gameState]()
	{
		auto center = Int2(ORIGINAL_WIDTH / 2, 120);
		auto function = [gameState]()
		{
			// Eventually go to a "ChooseQuestionsPanel". What about the "pop-up" message?
			/*auto classPanel = std::unique_ptr<Panel>(new ChooseClassPanel(gameState));
			gameState->setPanel(std::move(classPanel));*/
		};
		return std::unique_ptr<Button>(new Button(center, 175, 35, function));
	}();

	this->selectButton = [gameState]()
	{
		auto center = Int2(ORIGINAL_WIDTH / 2, 160);
		auto function = [gameState]()
		{
			auto classPanel = std::unique_ptr<Panel>(new ChooseClassPanel(gameState));
			gameState->setPanel(std::move(classPanel));
		};
		return std::unique_ptr<Button>(new Button(center, 175, 35, function));
	}();
}

ChooseClassCreationPanel::~ChooseClassCreationPanel()
{

}

void ChooseClassCreationPanel::handleEvents(bool &running)
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
			this->backToMainMenuButton->click();
		}

		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);

		bool generateClicked = leftClick &&
			this->generateButton->containsPoint(mouseOriginalPoint);
		bool selectClicked = leftClick &&
			this->selectButton->containsPoint(mouseOriginalPoint);

		if (generateClicked)
		{
			this->generateButton->click();
		}
		else if (selectClicked)
		{
			this->selectButton->click();
		}
	}
}

void ChooseClassCreationPanel::handleMouse(double dt)
{
	static_cast<void>(dt);
}

void ChooseClassCreationPanel::handleKeyboard(double dt)
{
	static_cast<void>(dt);
}

void ChooseClassCreationPanel::tick(double dt, bool &running)
{
	static_cast<void>(dt);

	this->handleEvents(running);
}

void ChooseClassCreationPanel::render(SDL_Renderer *renderer, const SDL_Rect *letterbox)
{
	// Clear full screen.
	this->clearScreen(renderer);

	// Draw background.
	const auto *background = this->getGameState()->getTextureManager()
		.getTexture(TextureFile::fromName(TextureName::CharacterCreation));
	this->drawLetterbox(background, renderer, letterbox);

	// Draw parchments: title, generate, select.
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

	parchmentWidth = static_cast<int>(this->parchment->getWidth() * parchmentXScale);
	parchmentHeight = static_cast<int>(this->parchment->getHeight() * parchmentYScale);
	parchmentX = (ORIGINAL_WIDTH / 2) - (parchmentWidth / 2);
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

	// Draw text: title, generate, select.
	this->drawScaledToNative(*this->titleTextBox.get(), renderer);
	this->drawScaledToNative(*this->generateTextBox.get(), renderer);
	this->drawScaledToNative(*this->selectTextBox.get(), renderer);
	
	// Draw cursor.
	const auto &cursor = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::SwordCursor));
	this->drawCursor(cursor, renderer);
}
