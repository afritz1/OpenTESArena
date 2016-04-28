#include <cassert>
#include <iostream>

#include "SDL2\SDL.h"

#include "ChooseClassPanel.h"

#include "Button.h"
#include "ChooseGenderPanel.h"
#include "ChooseNamePanel.h"
#include "TextBox.h"
#include "../Entities/CharacterClass.h"
#include "../Entities/CharacterClassCategoryName.h"
#include "../Entities/CharacterGenderName.h"
#include "../Game/GameState.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/FontName.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"

ChooseClassPanel::ChooseClassPanel(GameState *gameState, CharacterGenderName gender)
	: Panel(gameState)
{
	this->backToGenderButton = nullptr;
	this->classTextBox = nullptr;
	this->warriorTextBox = nullptr;
	this->mageTextBox = nullptr;
	this->thiefTextBox = nullptr;
	this->warriorButton = nullptr;
	this->mageButton = nullptr;
	this->thiefButton = nullptr;
	this->gender = nullptr;

	this->parchment = [gameState]()
	{
		const int originalWidth = 320;
		auto *surface = gameState->getTextureManager().getSurface(
			TextureName::ParchmentPopup).getSurface();
		auto origin = Int2((originalWidth / 2) - (surface->w / 2), 20);
		return std::unique_ptr<Surface>(new Surface(
			origin.getX(), origin.getY(), surface));
	}();

	this->classTextBox = [gameState]()
	{
		auto origin = Int2(85, 35);
		auto color = Color(48, 12, 12);
		std::string text = "Choose thy class";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			origin.getX(),
			origin.getY(),
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->warriorTextBox = [gameState]()
	{
		auto origin = Int2(130, 85);
		auto color = Color(48, 12, 12);
		std::string text = "Warrior";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			origin.getX(),
			origin.getY(),
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->mageTextBox = [gameState]()
	{
		auto origin = Int2(140, 125);
		auto color = Color(48, 12, 12);
		std::string text = "Mage";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			origin.getX(),
			origin.getY(),
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->thiefTextBox = [gameState]()
	{
		auto origin = Int2(136, 165);
		auto color = Color(48, 12, 12);
		std::string text = "Thief";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			origin.getX(),
			origin.getY(),
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->backToGenderButton = [gameState]()
	{
		auto function = [gameState]()
		{
			gameState->setPanel(std::unique_ptr<Panel>(
				new ChooseGenderPanel(gameState)));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	this->warriorButton = [gameState, gender]()
	{
		auto center = Int2(160, 90);
		auto function = [gameState, gender]()
		{
			// Placeholder warrior variable.
			auto warriorClass = CharacterClass(
				"Warrior",
				CharacterClassCategoryName::Warrior,
				false,
				25,
				15,
				std::vector<ArmorMaterialType>(),
				std::vector<ShieldType>(),
				std::vector<WeaponType>());
			auto namePanel = std::unique_ptr<Panel>(new ChooseNamePanel(
				gameState, gender, warriorClass));
			gameState->setPanel(std::move(namePanel));
		};

		return std::unique_ptr<Button>(new Button(center, 120, 30, function));
	}();

	this->mageButton = [gameState, gender]()
	{
		auto center = Int2(160, 130);
		auto function = [gameState, gender]()
		{
			// Placeholder mage variable.
			auto mageClass = CharacterClass(
				"Mage",
				CharacterClassCategoryName::Mage,
				true,
				25,
				6,
				std::vector<ArmorMaterialType>(),
				std::vector<ShieldType>(),
				std::vector<WeaponType>());
			auto namePanel = std::unique_ptr<Panel>(new ChooseNamePanel(
				gameState, gender, mageClass));
			gameState->setPanel(std::move(namePanel));
		};

		return std::unique_ptr<Button>(new Button(center, 120, 30, function));
	}();

	this->thiefButton = [gameState, gender]()
	{
		auto center = Int2(160, 170);
		auto function = [gameState, gender]()
		{
			// Placeholder thief variable.
			auto thiefClass = CharacterClass(
				"Thief",
				CharacterClassCategoryName::Thief,
				true,
				25,
				14,
				std::vector<ArmorMaterialType>(),
				std::vector<ShieldType>(),
				std::vector<WeaponType>());
			auto namePanel = std::unique_ptr<Panel>(new ChooseNamePanel(
				gameState, gender, thiefClass));
			gameState->setPanel(std::move(namePanel));
		};

		return std::unique_ptr<Button>(new Button(center, 120, 30, function));
	}();

	this->gender = std::unique_ptr<CharacterGenderName>(
		new CharacterGenderName(gender));

	assert(this->backToGenderButton.get() != nullptr);
	assert(this->classTextBox.get() != nullptr);
	assert(this->warriorTextBox.get() != nullptr);
	assert(this->mageTextBox.get() != nullptr);
	assert(this->thiefTextBox.get() != nullptr);
	assert(this->warriorButton.get() != nullptr);
	assert(this->mageButton.get() != nullptr);
	assert(this->thiefButton.get() != nullptr);
	assert(this->gender.get() != nullptr);
	assert(*this->gender.get() == gender);
}

ChooseClassPanel::~ChooseClassPanel()
{

}

void ChooseClassPanel::handleEvents(bool &running)
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
			this->backToGenderButton->click();
		}

		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);

		// Eventually replace these "if's" with a std::map iteration once all the
		// class buttons are listed (if that's the chosen design).
		bool warriorClicked = leftClick &&
			this->warriorButton->containsPoint(mouseOriginalPoint);
		bool mageClicked = leftClick &&
			this->mageButton->containsPoint(mouseOriginalPoint);
		bool thiefClicked = leftClick &&
			this->thiefButton->containsPoint(mouseOriginalPoint);

		if (warriorClicked)
		{
			this->warriorButton->click();
		}
		else if (mageClicked)
		{
			this->mageButton->click();
		}
		else if (thiefClicked)
		{
			this->thiefButton->click();
		}
	}
}

void ChooseClassPanel::handleMouse(double dt)
{
	static_cast<void>(dt);
}

void ChooseClassPanel::handleKeyboard(double dt)
{
	static_cast<void>(dt);
}

void ChooseClassPanel::tick(double dt, bool &running)
{
	static_cast<void>(dt);

	this->handleEvents(running);
}

void ChooseClassPanel::render(SDL_Surface *dst, const SDL_Rect *letterbox)
{
	// Clear full screen.
	this->clearScreen(dst);

	// Draw background.
	const auto &background = this->getGameState()->getTextureManager()
		.getSurface(TextureName::CharacterCreation);
	this->drawLetterbox(background, dst, letterbox);

	// Draw parchments: title, warrior, mage, and thief.
	this->parchment->setTransparentColor(Color::Magenta);
	this->drawScaledToNative(*this->parchment.get(), dst);

	const auto parchmentScale = 0.70;
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

	parchmentYOffset = 10 + (parchmentYStep * 3);
	this->drawScaledToNative(*this->parchment.get(),
		this->parchment->getX() + parchmentXOffset,
		this->parchment->getY() + parchmentYOffset,
		static_cast<int>(this->parchment->getWidth() * parchmentScale),
		this->parchment->getHeight(),
		dst);

	// Draw text: title, warrior, mage, and thief.
	this->drawScaledToNative(*this->classTextBox.get(), dst);
	this->drawScaledToNative(*this->warriorTextBox.get(), dst);
	this->drawScaledToNative(*this->mageTextBox.get(), dst);
	this->drawScaledToNative(*this->thiefTextBox.get(), dst);

	// Draw cursor.
	const auto &cursor = this->getGameState()->getTextureManager()
		.getSurface(TextureName::SwordCursor);
	this->drawCursor(cursor, dst);
}
