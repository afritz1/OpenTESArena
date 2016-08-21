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
#include "../Media/PaletteName.h"
#include "../Media/TextureName.h"
#include "../Media/TextureSequenceName.h"
#include "../Rendering/Renderer.h"

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
			PaletteName::Default,
			TextureSequenceName::IntroStory,
			secondsPerImage,
			changeToMainMenu));
		gameState->setPanel(std::move(introStoryPanel));
	};

	auto changeToScrolling = [changeToIntroStory](GameState *gameState)
	{
		std::unique_ptr<Panel> scrollingPanel(new CinematicPanel(
			gameState,
			PaletteName::Default,
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
			PaletteName::BuiltIn,
			TextureName::IntroQuote,
			secondsToDisplay,
			changeToScrolling));
		gameState->setPanel(std::move(quotePanel));
	};

	auto changeToTitle = [changeToQuote](GameState *gameState)
	{
		double secondsToDisplay = 5.0;
		std::unique_ptr<Panel> titlePanel(new ImagePanel(
			gameState,
			PaletteName::BuiltIn,
			TextureName::IntroTitle,
			secondsToDisplay,
			changeToQuote));
		gameState->setPanel(std::move(titlePanel));
	};

	return std::unique_ptr<Panel>(new CinematicPanel(
		gameState,
		PaletteName::Default,
		TextureSequenceName::IntroBook, 
		0.142 /* roughly 7fps */,
		changeToTitle));
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
	int32_t x, y;
	SDL_GetMouseState(&x, &y);
	return Int2(x, y);
}
