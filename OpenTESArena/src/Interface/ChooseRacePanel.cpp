#include <cassert>
#include <iostream>

#include "SDL.h"

#include "ChooseRacePanel.h"

#include "Button.h"
#include "ChooseAttributesPanel.h"
#include "ChooseGenderPanel.h"
#include "TextBox.h"
#include "../Entities/CharacterClass.h"
#include "../Entities/CharacterGenderName.h"
#include "../Entities/CharacterRaceName.h"
#include "../Game/GameState.h"
#include "../Math/Constants.h"
#include "../Math/Int2.h"
#include "../Math/Rect.h"
#include "../Media/Color.h"
#include "../Media/FontName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../World/Province.h"
#include "../World/ProvinceName.h"

ChooseRacePanel::ChooseRacePanel(GameState *gameState, const CharacterClass &charClass, 
	const std::string &name, CharacterGenderName gender)
	: Panel(gameState)
{
	this->parchment = [gameState]()
	{
		auto *surface = gameState->getTextureManager().getSurface(
			TextureFile::fromName(TextureName::ParchmentPopup)).getSurface();
		return std::unique_ptr<Surface>(new Surface(surface));
	}();

	this->initialTextBox = [gameState, charClass, name]()
	{
		auto center = Int2(160, 100);
		auto color = Color(48, 12, 12);
		std::string text = "From where dost thou hail,\n" +
			name + "\nthe\n" + charClass.getDisplayName() + "?";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->backToGenderButton = [gameState, charClass, name]()
	{
		auto function = [gameState, charClass, name]()
		{
			auto namePanel = std::unique_ptr<Panel>(new ChooseGenderPanel(
				gameState, charClass, name));
			gameState->setPanel(std::move(namePanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	this->acceptButton = [this, gameState, gender, charClass, name]()
	{
		auto function = [this, gameState, gender, charClass, name]()
		{
			auto attributesPanel = std::unique_ptr<Panel>(new ChooseAttributesPanel(
				gameState, gender, charClass, name, *this->raceName.get()));
			gameState->setPanel(std::move(attributesPanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	// Clickable (x, y, width, height) areas for each province. The original game actually
	// uses a set of pixels, I think.
	this->provinceAreas = std::map<ProvinceName, Rect>
	{
		{ ProvinceName::BlackMarsh, Rect(216, 144, 55, 12) },
		{ ProvinceName::Elsweyr, Rect(148, 127, 37, 11) },
		{ ProvinceName::Hammerfell, Rect(72, 75, 50, 11) },
		{ ProvinceName::HighRock, Rect(52, 51, 44, 11) },
		{ ProvinceName::ImperialProvince, Rect(133, 105, 83, 11) },
		{ ProvinceName::Morrowind, Rect(222, 84, 52, 11) },
		{ ProvinceName::Skyrim, Rect(142, 44, 34, 11) },
		{ ProvinceName::SummersetIsle, Rect(37, 149, 49, 19) },
		{ ProvinceName::Valenwood, Rect(106, 147, 49, 10) }
	};

	this->charClass = std::unique_ptr<CharacterClass>(new CharacterClass(charClass));
	this->name = name;
	this->gender = std::unique_ptr<CharacterGenderName>(new CharacterGenderName(gender));
	this->raceName = nullptr;
}

ChooseRacePanel::~ChooseRacePanel()
{

}

void ChooseRacePanel::handleEvents(bool &running)
{
	auto mousePosition = this->getMousePosition();
	auto mouseOriginalPoint = this->nativePointToOriginal(mousePosition);

	SDL_Event e;
	while (SDL_PollEvent(&e) != 0)
	{
		bool applicationExit = (e.type == SDL_QUIT);
		bool resized = (e.type == SDL_WINDOWEVENT) &&
			(e.window.event == SDL_WINDOWEVENT_RESIZED);

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

		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);
		bool rightClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_RIGHT);

		bool escapePressed = (e.type == SDL_KEYDOWN) &&
			(e.key.keysym.sym == SDLK_ESCAPE);
		bool enterPressed = (e.type == SDL_KEYDOWN) &&
			((e.key.keysym.sym == SDLK_RETURN) ||
				(e.key.keysym.sym == SDLK_KP_ENTER));
		bool spacePressed = (e.type == SDL_KEYDOWN) &&
			(e.key.keysym.sym == SDLK_SPACE);

		// Context-sensitive input depending on the visibility of the first text box.
		if (this->initialTextBox->isVisible())
		{
			bool hideInitialPopUp = leftClick || rightClick || enterPressed ||
				spacePressed || escapePressed;

			if (hideInitialPopUp)
			{
				// Hide the initial text box.
				this->initialTextBox->setVisibility(false);
			}
		}
		else
		{
			if (escapePressed)
			{
				this->backToGenderButton->click();
			}
			else if (leftClick)
			{
				// Listen for map clicks.
				for (const auto &area : this->provinceAreas)
				{
					// Ignore the Imperial race because it is not implemented yet.
					if (area.second.contains(mouseOriginalPoint) &&
						(area.first != ProvinceName::ImperialProvince))
					{
						// Save the clicked province's race.
						auto provinceRace = Province(area.first).getRaceName();
						this->raceName = std::unique_ptr<CharacterRaceName>(new CharacterRaceName(
							provinceRace));

						// Go to the attributes panel.
						this->acceptButton->click();
						break;
					}
				}
			}
		}
	}
}

void ChooseRacePanel::handleMouse(double dt)
{
	static_cast<void>(dt);
}

void ChooseRacePanel::handleKeyboard(double dt)
{
	static_cast<void>(dt);
}

void ChooseRacePanel::tick(double dt, bool &running)
{
	static_cast<void>(dt);

	this->handleEvents(running);
}

void ChooseRacePanel::drawProvinceTooltip(ProvinceName provinceName, SDL_Surface *dst)
{
	auto mouseOriginalPosition = this->nativePointToOriginal(this->getMousePosition());
	const std::string raceName = Province(provinceName).getRaceDisplayName(true);
	auto tooltip = std::unique_ptr<TextBox>(new TextBox(
		mouseOriginalPosition.getX(),
		mouseOriginalPosition.getY(),
		Color::White,
		"Land of the " + raceName,
		FontName::A,
		this->getGameState()->getTextureManager()));
	Surface tooltipBackground(tooltip->getX(), tooltip->getY(),
		tooltip->getWidth(), tooltip->getHeight());
	tooltipBackground.fill(Color(32, 32, 32));

	const int width = tooltip->getWidth() / 2;
	const int height = tooltip->getHeight() / 2;
	const int x = ((tooltip->getX() + width) < ORIGINAL_WIDTH) ? tooltip->getX() :
		(tooltip->getX() - width);
	const int y = ((tooltip->getY() + height) < ORIGINAL_HEIGHT) ? tooltip->getY() :
		(tooltip->getY() - height);

	this->drawScaledToNative(tooltipBackground, x, y - 1, width, height + 2, dst);
	this->drawScaledToNative(*tooltip.get(), x, y, width, height, dst);
}

void ChooseRacePanel::render(SDL_Surface *dst, const SDL_Rect *letterbox)
{
	// Clear full screen.
	this->clearScreen(dst);

	// Draw background map.
	const auto &worldMap = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::WorldMap));
	this->drawLetterbox(worldMap, dst, letterbox);

	// Draw visible parchments and text.
	this->parchment->setTransparentColor(Color::Magenta);
	if (this->initialTextBox->isVisible())
	{
		const int parchmentWidth = static_cast<int>(this->parchment->getWidth() * 1.35);
		const int parchmentHeight = static_cast<int>(this->parchment->getHeight() * 1.65);
		this->drawScaledToNative(*this->parchment.get(),
			(ORIGINAL_WIDTH / 2) - (parchmentWidth / 2),
			(ORIGINAL_HEIGHT / 2) - (parchmentHeight / 2),
			parchmentWidth,
			parchmentHeight,
			dst);
		this->drawScaledToNative(*this->initialTextBox.get(), dst);
	}

	// Draw cursor.
	const auto &cursor = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::SwordCursor));
	this->drawCursor(cursor, dst);

	// Draw hovered province tooltip.
	if (!this->initialTextBox->isVisible())
	{
		auto mouseOriginalPosition =
			this->nativePointToOriginal(this->getMousePosition());

		for (const auto &pair : this->provinceAreas)
		{
			// Draw tooltip if the mouse is in the province.

			// Ignore the Imperial race for now as it is not implemented.
			if (pair.second.contains(mouseOriginalPosition) &&
				(pair.first != ProvinceName::ImperialProvince))
			{
				this->drawProvinceTooltip(pair.first, dst);
			}
		}
	}
}
