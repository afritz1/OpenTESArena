#include <cassert>
#include <iostream>
#include <map>

#include "SDL2\SDL.h"

#include "ChooseNamePanel.h"

#include "Button.h"
#include "ChooseClassPanel.h"
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

const int ChooseNamePanel::MAX_NAME_LENGTH = 25;

ChooseNamePanel::ChooseNamePanel(GameState *gameState, CharacterGenderName gender,
	const CharacterClass &charClass)
	: Panel(gameState)
{
	this->parchment = nullptr;
	this->titleTextBox = nullptr;
	this->nameTextBox = nullptr;
	this->backToClassButton = nullptr;
	this->acceptButton = nullptr;
	this->gender = nullptr;
	this->charClass = nullptr;
	this->name = std::string();

	this->parchment = [gameState]()
	{
		auto *surface = gameState->getTextureManager().getSurface(
			TextureFile::fromName(TextureName::ParchmentPopup)).getSurface();
		return std::unique_ptr<Surface>(new Surface(surface));
	}();

	this->titleTextBox = [gameState, charClass]()
	{
		auto center = Int2(ORIGINAL_WIDTH / 2, 90);
		auto color = Color(48, 12, 12);
		std::string text = "What will be thy name,\n" + charClass.getDisplayName() + "?";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->nameTextBox = [gameState]()
	{
		auto center = Int2(ORIGINAL_WIDTH / 2, 110);
		auto color = Color::White;
		std::string text = "";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
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

	this->acceptButton = [this, gameState, gender, charClass]()
	{
		auto function = [this, gameState, gender, charClass]()
		{
			auto racePanel = std::unique_ptr<Panel>(new ChooseRacePanel(
				gameState, gender, charClass, this->name));
			gameState->setPanel(std::move(racePanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	this->gender = std::unique_ptr<CharacterGenderName>(
		new CharacterGenderName(gender));
	this->charClass = std::unique_ptr<CharacterClass>(
		new CharacterClass(charClass));

	assert(this->parchment.get() != nullptr);
	assert(this->titleTextBox.get() != nullptr);
	assert(this->nameTextBox.get() != nullptr);
	assert(this->backToClassButton.get() != nullptr);
	assert(this->acceptButton.get() != nullptr);
	assert(this->gender.get() != nullptr);
	assert(*this->gender.get() == gender);
	assert(this->charClass.get() != nullptr);
	assert(this->name.size() == 0);
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
		bool enterPressed = (e.type == SDL_KEYDOWN) &&
			((e.key.keysym.sym == SDLK_RETURN) || (e.key.keysym.sym == SDLK_KP_ENTER));

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
			this->backToClassButton->click();
		}

		// Only accept the name if it has a positive size.
		if (enterPressed && (this->name.size() > 0))
		{
			this->acceptButton->click();
		}

		// Upper and lower case English characters.
		const auto letters = std::map<SDL_Keycode, std::pair<char, char>>
		{
			{ SDLK_a, { 'A', 'a' } },
			{ SDLK_b, { 'B', 'b' } },
			{ SDLK_c, { 'C', 'c' } },
			{ SDLK_d, { 'D', 'd' } },
			{ SDLK_e, { 'E', 'e' } },
			{ SDLK_f, { 'F', 'f' } },
			{ SDLK_g, { 'G', 'g' } },
			{ SDLK_h, { 'H', 'h' } },
			{ SDLK_i, { 'I', 'i' } },
			{ SDLK_j, { 'J', 'j' } },
			{ SDLK_k, { 'K', 'k' } },
			{ SDLK_l, { 'L', 'l' } },
			{ SDLK_m, { 'M', 'm' } },
			{ SDLK_n, { 'N', 'n' } },
			{ SDLK_o, { 'O', 'o' } },
			{ SDLK_p, { 'P', 'p' } },
			{ SDLK_q, { 'Q', 'q' } },
			{ SDLK_r, { 'R', 'r' } },
			{ SDLK_s, { 'S', 's' } },
			{ SDLK_t, { 'T', 't' } },
			{ SDLK_u, { 'U', 'u' } },
			{ SDLK_v, { 'V', 'v' } },
			{ SDLK_w, { 'W', 'w' } },
			{ SDLK_x, { 'X', 'x' } },
			{ SDLK_y, { 'Y', 'y' } },
			{ SDLK_z, { 'Z', 'z' } }
		};

		// Punctuation (some duplicates exist to keep the shift behavior for quotes).
		const auto punctuation = std::map<SDL_Keycode, std::pair<char, char>>
		{
			{ SDLK_COMMA, { ',', ',' } },
			{ SDLK_MINUS, { '-', '-' } },
			{ SDLK_PERIOD, { '.', '.' } },
			{ SDLK_QUOTE, { '"', '\'' } }
		};

		if (e.type == SDL_KEYDOWN)
		{
			const auto *keys = SDL_GetKeyboardState(nullptr);
			const auto keyCode = e.key.keysym.sym;
			bool shiftPressed = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];

			// See if the pressed key is a recognized letter.
			if (letters.find(keyCode) != letters.end())
			{
				// Add the letter to the name if there is room.
				if (this->name.size() < ChooseNamePanel::MAX_NAME_LENGTH)
				{
					const auto &pair = letters.at(keyCode);
					this->name.push_back(shiftPressed ? pair.first : pair.second);
				}
			}	
			else if (punctuation.find(keyCode) != punctuation.end())
			{
				// The pressed key is recognized punctuation. Add it.
				if (this->name.size() < ChooseNamePanel::MAX_NAME_LENGTH)
				{
					const auto &pair = punctuation.at(keyCode);
					this->name.push_back(shiftPressed ? pair.first : pair.second);
				}
			}
			else if (keyCode == SDLK_SPACE)
			{
				// The pressed key is space. Add a space.
				if (this->name.size() < ChooseNamePanel::MAX_NAME_LENGTH)
				{
					this->name.push_back(' ');
				}
			}
			else if (keyCode == SDLK_BACKSPACE)
			{
				// The pressed key is backspace. Erase one letter if able.
				if (this->name.size() > 0)
				{
					this->name.pop_back();
				}
			}

			// Update the displayed name.
			this->nameTextBox = [this]
			{
				return std::unique_ptr<TextBox>(new TextBox(
					Int2(ORIGINAL_WIDTH / 2, 110),
					Color::White,
					this->name,
					FontName::A,
					this->getGameState()->getTextureManager()));
			}();
		}

		// If (asciiCharacter or space is pressed) then push it onto the string.
		// If (Backspace is pressed) then delete one off if not at the start.

		// If (enter is pressed) then try and accept the string. Save it to a member.
	}
}

void ChooseNamePanel::handleMouse(double dt)
{
	static_cast<void>(dt);
}

void ChooseNamePanel::handleKeyboard(double dt)
{
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
		.getSurface(TextureFile::fromName(TextureName::CharacterCreation));
	this->drawLetterbox(background, dst, letterbox);

	// Draw parchment: title.
	this->parchment->setTransparentColor(Color::Magenta);

	const auto parchmentXScale = 1.5;
	const auto parchmentYScale = 1.65;
	const int parchmentWidth = static_cast<int>(this->parchment->getWidth() * parchmentXScale);
	const int parchmentHeight = static_cast<int>(this->parchment->getHeight() * parchmentYScale);
	this->drawScaledToNative(*this->parchment.get(),
		(ORIGINAL_WIDTH / 2) - (parchmentWidth / 2),
		(ORIGINAL_HEIGHT / 2) - (parchmentHeight / 2),
		parchmentWidth,
		parchmentHeight,
		dst);

	// Draw text: title, name.
	this->drawScaledToNative(*this->titleTextBox.get(), dst);
	this->drawScaledToNative(*this->nameTextBox.get(), dst);

	// Draw cursor.
	const auto &cursor = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::SwordCursor));
	this->drawCursor(cursor, dst);
}
