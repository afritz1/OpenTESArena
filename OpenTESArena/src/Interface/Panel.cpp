#include <vector>

#include "SDL.h"

#include "Panel.h"

#include "CinematicPanel.h"
#include "ImageSequencePanel.h"
#include "ImagePanel.h"
#include "MainMenuPanel.h"
#include "Surface.h"
#include "../Game/GameState.h"
#include "../Game/Options.h"
#include "../Math/Int2.h"
#include "../Math/Rect.h"
#include "../Media/Color.h"
#include "../Media/MusicName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureName.h"
#include "../Media/TextureSequenceName.h"
#include "../Rendering/Renderer.h"

#include "components/vfs/manager.hpp"

Panel::Panel(GameState *gameState)
{
	this->gameStatePtr = gameState;
}

Panel::~Panel()
{
	// gameState is owned by the Game object.
}

std::unique_ptr<Panel> Panel::defaultPanel(GameState *gameState)
{
	// If the intro skip option is set, then jump to the main menu.
	if (gameState->getOptions().introIsSkipped())
	{
		return std::unique_ptr<Panel>(new MainMenuPanel(gameState));
	}

	// All of these lambdas are linked together like a stack by each panel's last
	// argument.
	auto changeToMainMenu = [](GameState *gameState)
	{
		std::unique_ptr<Panel> mainMenuPanel(new MainMenuPanel(gameState));
		gameState->setPanel(std::move(mainMenuPanel));
	};

	auto changeToIntroStory = [changeToMainMenu](GameState *gameState)
	{
		std::vector<std::string> paletteNames
		{
			"SCROLL03.IMG", "SCROLL03.IMG", "SCROLL03.IMG"
		};

		std::vector<std::string> textureNames
		{
			"SCROLL01.IMG", "SCROLL02.IMG", "SCROLL03.IMG"
		};

		// In the original game, the last frame ("...hope flies on death's wings...")
		// seems to be a bit shorter.
		std::vector<double> imageDurations
		{
			13.0, 13.0, 10.0
		};

		std::unique_ptr<Panel> introStoryPanel(new ImageSequencePanel(
			gameState,
			paletteNames,
			textureNames,
			imageDurations,
			changeToMainMenu));

		gameState->setPanel(std::move(introStoryPanel));
	};

	auto changeToScrolling = [changeToIntroStory](GameState *gameState)
	{
		std::unique_ptr<Panel> scrollingPanel(new CinematicPanel(
			gameState,
			TextureFile::fromName(TextureSequenceName::OpeningScroll),
			PaletteFile::fromName(PaletteName::Default),
			0.042,
			changeToIntroStory));
		gameState->setPanel(std::move(scrollingPanel));
	};

	auto changeToQuote = [changeToScrolling](GameState *gameState)
	{
		const double secondsToDisplay = 5.0;
		std::unique_ptr<Panel> quotePanel(new ImagePanel(
			gameState,
			PaletteFile::fromName(PaletteName::BuiltIn),
			TextureFile::fromName(TextureName::IntroQuote),
			secondsToDisplay,
			changeToScrolling));
		gameState->setPanel(std::move(quotePanel));
	};

	auto makeIntroTitlePanel = [changeToQuote, gameState]()
	{
		const double secondsToDisplay = 5.0;
		std::unique_ptr<Panel> titlePanel(new ImagePanel(
			gameState,
			PaletteFile::fromName(PaletteName::BuiltIn),
			TextureFile::fromName(TextureName::IntroTitle),
			secondsToDisplay,
			changeToQuote));
		return std::move(titlePanel);
	};

	auto changeToTitle = [makeIntroTitlePanel, changeToQuote](GameState *gameState)
	{
		std::unique_ptr<Panel> titlePanel = makeIntroTitlePanel();
		gameState->setPanel(std::move(titlePanel));
	};

	/*auto makeIntroBookPanel = [changeToTitle, gameState]()
	{
		std::unique_ptr<Panel> introBook(new CinematicPanel(
			gameState,
			TextureFile::fromName(TextureSequenceName::IntroBook),
			PaletteFile::fromName(PaletteName::Default),
			0.142, // Roughly 7 fps.
			changeToTitle));
		return std::move(introBook);
	};*/

	// Decide how the game starts up. If only the floppy disk data is available,
	// then go to the Arena splash screen. Otherwise, load the intro book video.	

	// Just skip the intro book check for now.
	return makeIntroTitlePanel();

	// Check if "INTRO.FLC" is available (only available in CD version).
	//VFS::IStreamPtr stream = VFS::Manager::get().open("INTRO.FLC");

	// Once all texture sequences are available as FLCFile loads, uncomment this.
	//return (stream != nullptr) ? makeIntroBookPanel() : makeIntroTitlePanel();
}

GameState *Panel::getGameState() const
{
	return this->gameStatePtr;
}

double Panel::getCursorScale() const
{
	double cursorScale = this->getGameState()->getOptions().getCursorScale();
	return cursorScale;
}

Int2 Panel::getMousePosition() const
{
	int x, y;
	SDL_GetMouseState(&x, &y);
	return Int2(x, y);
}

void Panel::setRelativeMouseMode(bool active)
{
	SDL_bool enabled = active ? SDL_TRUE : SDL_FALSE;
	SDL_SetRelativeMouseMode(enabled);
}
