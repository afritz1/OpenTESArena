#include <cassert>

#include "SDL.h"

#include "ChooseRacePanel.h"

#include "Button.h"
#include "ChooseAttributesPanel.h"
#include "ChooseGenderPanel.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Entities/CharacterClass.h"
#include "../Entities/CharacterGenderName.h"
#include "../Entities/CharacterRaceName.h"
#include "../Game/GameState.h"
#include "../Math/Int2.h"
#include "../Math/Rect.h"
#include "../Media/Color.h"
#include "../Media/FontManager.h"
#include "../Media/FontName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../World/Province.h"
#include "../World/ProvinceName.h"

ChooseRacePanel::ChooseRacePanel(GameState *gameState, const CharacterClass &charClass,
	const std::string &name, CharacterGenderName gender)
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

	this->initialTextBox = [gameState, charClass, name]()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH / 2, 100);
		Color color(48, 12, 12);
		std::string text = "From where dost thou hail,\n" +
			name + "\nthe\n" + charClass.getDisplayName() + "?";
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

	this->backToGenderButton = [charClass, name]()
	{
		auto function = [charClass, name](GameState *gameState)
		{
			std::unique_ptr<Panel> namePanel(new ChooseGenderPanel(
				gameState, charClass, name));
			gameState->setPanel(std::move(namePanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	this->acceptButton = [this, gender, charClass, name]()
	{
		auto function = [this, gender, charClass, name](GameState *gameState)
		{
			std::unique_ptr<Panel> attributesPanel(new ChooseAttributesPanel(
				gameState, charClass, name, gender, *this->raceName.get()));
			gameState->setPanel(std::move(attributesPanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	this->charClass = std::unique_ptr<CharacterClass>(new CharacterClass(charClass));
	this->name = name;
	this->gender = std::unique_ptr<CharacterGenderName>(new CharacterGenderName(gender));
	this->raceName = nullptr;
	this->initialTextBoxVisible = true;
}

ChooseRacePanel::~ChooseRacePanel()
{
	SDL_DestroyTexture(this->parchment);
}

void ChooseRacePanel::handleEvents(bool &running)
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
		// To do: find a better way to do this (via a queue of pop-ups?).
		if (this->initialTextBoxVisible)
		{
			bool hideInitialPopUp = leftClick || rightClick || enterPressed ||
				spacePressed || escapePressed;

			if (hideInitialPopUp)
			{
				// Hide the initial text box.
				this->initialTextBoxVisible = false;
			}

			continue;
		}

		// Interact with the map screen instead.
		if (escapePressed)
		{
			this->backToGenderButton->click(this->getGameState());
		}
		else if (leftClick)
		{
			// Listen for map clicks.
			for (const auto provinceName : Province::getAllProvinceNames())
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
					this->acceptButton->click(this->getGameState());
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

void ChooseRacePanel::drawProvinceTooltip(ProvinceName provinceName, Renderer &renderer)
{
	auto mouseOriginalPosition = this->getGameState()->getRenderer()
		.nativePointToOriginal(this->getMousePosition());
	const std::string raceName = Province(provinceName).getRaceDisplayName(true);
	std::unique_ptr<TextBox> tooltip(new TextBox(
		mouseOriginalPosition.getX(),
		mouseOriginalPosition.getY(),
		Color::White,
		"Land of the " + raceName,
		this->getGameState()->getFontManager().getFont(FontName::D),
		TextAlignment::Left,
		this->getGameState()->getRenderer()));

	int tooltipWidth, tooltipHeight;
	SDL_QueryTexture(tooltip->getTexture(), nullptr, nullptr,
		&tooltipWidth, &tooltipHeight);

	Surface tooltipBackground(tooltip->getX(), tooltip->getY(),
		tooltipWidth, tooltipHeight);
	tooltipBackground.fill(Color(32, 32, 32));

	const int tooltipX = tooltip->getX();
	const int tooltipY = tooltip->getY();
	const int x = ((tooltipX + 8 + tooltipWidth) < Renderer::ORIGINAL_WIDTH) ?
		(tooltipX + 8) : (tooltipX - tooltipWidth);
	const int y = ((tooltipY + tooltipHeight) < Renderer::ORIGINAL_HEIGHT) ?
		tooltipY : (tooltipY - tooltipHeight);

	renderer.drawToOriginal(tooltipBackground.getSurface(), x, y - 1, 
		tooltipWidth, tooltipHeight + 2);
	renderer.drawToOriginal(tooltip->getSurface(), x, y, tooltipWidth, tooltipHeight);
}

void ChooseRacePanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clearNative();
	renderer.clearOriginal();

	// Set palette.
	auto &textureManager = this->getGameState()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw background map.
	auto *raceSelectMap = textureManager.getTexture(
		TextureFile::fromName(TextureName::RaceSelect), 
		PaletteFile::fromName(PaletteName::BuiltIn));
	renderer.drawToOriginal(raceSelectMap);

	// Don't worry about the yellow dots for now. Whatever the original game is doing
	// to cover them up should be figured out sometime.

	// Draw visible parchments and text.
	if (this->initialTextBoxVisible)
	{
		int parchmentWidth, parchmentHeight;
		SDL_QueryTexture(this->parchment, nullptr, nullptr, &parchmentWidth, &parchmentHeight);
		const int parchmentNewWidth = static_cast<int>(parchmentWidth * 1.35);
		const int parchmentNewHeight = static_cast<int>(parchmentHeight * 1.65);

		renderer.drawToOriginal(this->parchment,
			(Renderer::ORIGINAL_WIDTH / 2) - (parchmentNewWidth / 2),
			(Renderer::ORIGINAL_HEIGHT / 2) - (parchmentNewHeight / 2),
			parchmentNewWidth,
			parchmentNewHeight);

		renderer.drawToOriginal(this->initialTextBox->getTexture(),
			this->initialTextBox->getX(), this->initialTextBox->getY());
	}

	// Draw hovered province tooltip.
	if (!this->initialTextBoxVisible)
	{
		auto mouseOriginalPosition = this->getGameState()->getRenderer()
			.nativePointToOriginal(this->getMousePosition());

		// Draw tooltip if the mouse is in a province.
		for (const auto provinceName : Province::getAllProvinceNames())
		{
			Province province(provinceName);
			const Rect &clickArea = province.getWorldMapClickArea();

			// Ignore the Imperial race for now as it is not implemented.
			if (clickArea.contains(mouseOriginalPosition) &&
				(provinceName != ProvinceName::ImperialProvince))
			{
				this->drawProvinceTooltip(provinceName, renderer);
			}
		}
	}

	// Scale the original frame buffer onto the native one.
	renderer.drawOriginalToNative();

	// Draw cursor.
	auto *cursor = textureManager.getSurface(
		TextureFile::fromName(TextureName::SwordCursor));
	auto mousePosition = this->getMousePosition();
	renderer.drawToNative(cursor,
		mousePosition.getX(), mousePosition.getY(),
		static_cast<int>(cursor->w * this->getCursorScale()),
		static_cast<int>(cursor->h * this->getCursorScale()));
}
