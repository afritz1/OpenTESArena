#include "SDL.h"

#include "Panel.h"

#include "CinematicPanel.h"
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
		// Lots of text to read, so give each image extra time.
		double secondsPerImage = 14.0;
		std::unique_ptr<Panel> introStoryPanel(new CinematicPanel(
			gameState,
			PaletteFile::fromName(PaletteName::Default),
			TextureSequenceName::IntroStory,
			secondsPerImage,
			changeToMainMenu));
		gameState->setPanel(std::move(introStoryPanel));
	};

	auto changeToScrolling = [changeToIntroStory](GameState *gameState)
	{
		std::unique_ptr<Panel> scrollingPanel(new CinematicPanel(
			gameState,
			PaletteFile::fromName(PaletteName::Default),
			TextureSequenceName::OpeningScroll,
			CinematicPanel::DEFAULT_MOVIE_SECONDS_PER_IMAGE,
			changeToIntroStory));
		gameState->setPanel(std::move(scrollingPanel));
	};

	auto changeToQuote = [changeToScrolling](GameState *gameState)
	{
		double secondsToDisplay = 5.0;
		std::unique_ptr<Panel> quotePanel(new ImagePanel(
			gameState,
			PaletteFile::fromName(PaletteName::BuiltIn),
			TextureFile::fromName(TextureName::IntroQuote),
			secondsToDisplay,
			changeToScrolling));
		gameState->setPanel(std::move(quotePanel));
	};

	auto changeToTitle = [changeToQuote](GameState *gameState)
	{
		double secondsToDisplay = 5.0;
		std::unique_ptr<Panel> titlePanel(new ImagePanel(
			gameState,
			PaletteFile::fromName(PaletteName::BuiltIn),
			TextureFile::fromName(TextureName::IntroTitle),
			secondsToDisplay,
			changeToQuote));
		gameState->setPanel(std::move(titlePanel));
	};
	
	// Decide how the game starts up. If only the floppy disk data is available,
	// then go to the Arena splash screen. Otherwise, load the intro book video.	
	auto makeIntroBookPanel = [changeToTitle, gameState]()
	{
		std::unique_ptr<Panel> introBook(new CinematicPanel(
			gameState,
			PaletteFile::fromName(PaletteName::Default),
			TextureSequenceName::IntroBook,
			0.142 /* roughly 7fps */,
			changeToTitle));
		return std::move(introBook);
	};

	auto makeIntroTitlePanel = [changeToQuote, gameState]()
	{
		std::unique_ptr<Panel> titlePanel(new ImagePanel(
			gameState,
			PaletteFile::fromName(PaletteName::BuiltIn),
			TextureFile::fromName(TextureName::IntroTitle),
			5.0,
			changeToQuote));
		return std::move(titlePanel);
	};

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
