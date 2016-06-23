#include <cassert>

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

		// Interact with the pop-up text box if visible.
		if (this->initialTextBox->isVisible())
		{
			bool hideInitialPopUp = leftClick || rightClick || enterPressed ||
				spacePressed || escapePressed;

			if (hideInitialPopUp)
			{
				// Hide the initial text box.
				this->initialTextBox->setVisibility(false);
			}

			continue;
		}

		// Interact with the map screen instead.
		if (escapePressed)
		{
			this->backToGenderButton->click();
		}
		else if (leftClick)
		{
			// Listen for map clicks.
			for (const auto &provinceName : Province::getAllProvinceNames())
			{
				Province province(provinceName);
				const Rect &clickArea = province.getWorldMapClickArea();

				// Ignore the Imperial race because it is not implemented yet.
				if (clickArea.contains(mouseOriginalPoint) &&
					(provinceName != ProvinceName::ImperialProvince))
				{
					// Save the clicked province's race.
					this->raceName = std::unique_ptr<CharacterRaceName>(new CharacterRaceName(
						province.getRaceName()));

					// Go to the attributes panel.
					this->acceptButton->click();
					break;
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

	const int tooltipX = tooltip->getX();
	const int tooltipY = tooltip->getY();
	const int width = tooltip->getWidth() / 2;
	const int height = tooltip->getHeight() / 2;
	const int x = ((tooltipX + width) < ORIGINAL_WIDTH) ? tooltipX : (tooltipX - width);
	const int y = ((tooltipY + height) < ORIGINAL_HEIGHT) ? tooltipY : (tooltipY - height);

	this->drawScaledToNative(tooltipBackground, x, y - 1, width, height + 2, dst);
	this->drawScaledToNative(*tooltip.get(), x, y, width, height, dst);
}

void ChooseRacePanel::render(SDL_Surface *dst, const SDL_Rect *letterbox)
{
	// Clear full screen.
	this->clearScreen(dst);

	// Draw background map.
	const auto &worldMap = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::CharacterRaceSelect));
	this->drawLetterbox(worldMap, dst, letterbox);

	// Cover up the bottom-right "Exit" text.
	Surface cornerCoverUp(40, 12);
	const auto &worldMap2 = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::WorldMap));
	Rect mapClipRect(
		worldMap2.getWidth() - cornerCoverUp.getWidth(),
		worldMap2.getHeight() - cornerCoverUp.getHeight(),
		cornerCoverUp.getWidth(),
		cornerCoverUp.getHeight());
	worldMap2.blit(cornerCoverUp, Int2(), mapClipRect);
	this->drawScaledToNative(cornerCoverUp,
		ORIGINAL_WIDTH - cornerCoverUp.getWidth(),
		ORIGINAL_HEIGHT - cornerCoverUp.getHeight(),
		cornerCoverUp.getWidth(),
		cornerCoverUp.getHeight(),
		dst);

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
		// Draw tooltip if the mouse is in a province.
		auto mouseOriginalPosition = this->nativePointToOriginal(this->getMousePosition());
		for (const auto &provinceName : Province::getAllProvinceNames())
		{
			Province province(provinceName);
			const Rect &clickArea = province.getWorldMapClickArea();

			// Ignore the Imperial race for now as it is not implemented.
			if (clickArea.contains(mouseOriginalPosition) &&
				(provinceName != ProvinceName::ImperialProvince))
			{
				this->drawProvinceTooltip(provinceName, dst);
			}
		}
	}
}
