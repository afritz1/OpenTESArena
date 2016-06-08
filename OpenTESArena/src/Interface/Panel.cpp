#include <SDL2/SDL.h>

#include "Panel.h"

#include "CinematicPanel.h"
#include "GameWorldPanel.h"
#include "ImagePanel.h"
#include "MainMenuPanel.h"
#include "Surface.h"
#include "../Game/GameState.h"
#include "../Game/Options.h"
#include "../Math/Constants.h"
#include "../Math/Int2.h"
#include "../Math/Rectangle.h"
#include "../Media/MusicName.h"
#include "../Media/TextureName.h"
#include "../Media/TextureSequenceName.h"

Panel::Panel(GameState *gameState)
{
	this->gameStatePtr = gameState;
}

Panel::~Panel()
{
	// gameState is owned in the Game object.
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

	auto changeToMainMenu = [gameState]()
	{
		gameState->setPanel(std::unique_ptr<Panel>(new MainMenuPanel(gameState)));
	};

	auto changeToIntroStory = [gameState, changeToMainMenu]()
	{
		// Lots of text to read, so give each image extra time.
		auto secondsPerImage = 14.0;
		auto introStoryPanel = std::unique_ptr<Panel>(new CinematicPanel(
			gameState,
			TextureSequenceName::IntroStory,
			secondsPerImage,
			changeToMainMenu));
		gameState->setPanel(std::move(introStoryPanel));
	};

	auto changeToScrolling = [gameState, changeToIntroStory]()
	{
		auto scrollingPanel = std::unique_ptr<Panel>(new CinematicPanel(
			gameState,
			TextureSequenceName::OpeningScroll,
			CinematicPanel::DEFAULT_MOVIE_SECONDS_PER_IMAGE,
			changeToIntroStory));
		gameState->setPanel(std::move(scrollingPanel));
	};

	auto changeToQuote = [gameState, changeToScrolling]()
	{
		auto secondsToDisplay = 5.0;
		auto quotePanel = std::unique_ptr<Panel>(new ImagePanel(
			gameState,
			TextureName::IntroQuote,
			secondsToDisplay,
			changeToScrolling));
		gameState->setPanel(std::move(quotePanel));
	};

	auto changeToTitle = [gameState, changeToQuote]()
	{
		auto secondsToDisplay = 5.0;
		auto titlePanel = std::unique_ptr<Panel>(new ImagePanel(
			gameState,
			TextureName::IntroTitle,
			secondsToDisplay,
			changeToQuote));
		gameState->setPanel(std::move(titlePanel));
	};

	return std::unique_ptr<Panel>(new CinematicPanel(gameState,
		TextureSequenceName::IntroBook,
		CinematicPanel::DEFAULT_MOVIE_SECONDS_PER_IMAGE,
		changeToTitle));
}

GameState *Panel::getGameState() const
{
	return this->gameStatePtr;
}

double Panel::getDrawScale() const
{
	auto letterbox = this->getGameState()->getLetterboxDimensions();
	return static_cast<double>(letterbox->w) / static_cast<double>(ORIGINAL_WIDTH);
}

Int2 Panel::getMousePosition() const
{
	int x, y;
	SDL_GetMouseState(&x, &y);
	return Int2(x, y);
}

bool Panel::letterboxContains(const Int2 &point) const
{
	auto letterbox = this->getGameState()->getLetterboxDimensions();
	auto rectangle = Rectangle(letterbox->x, letterbox->y,
		letterbox->w, letterbox->h);
	return rectangle.contains(point);
}

Int2 Panel::nativePointToOriginal(const Int2 &point) const
{
	auto drawScale = this->getDrawScale();
	auto letterbox = this->getGameState()->getLetterboxDimensions();
	return Int2(
		(point.getX() - letterbox->x) / drawScale,
		(point.getY() - letterbox->y) / drawScale);
}

Int2 Panel::originalPointToNative(const Int2 &point) const
{
	auto drawScale = this->getDrawScale();
	auto letterbox = this->getGameState()->getLetterboxDimensions();
	return Int2(
		(point.getX() * drawScale) + letterbox->x,
		(point.getY() * drawScale) + letterbox->y);
}

unsigned int Panel::getMagenta(SDL_PixelFormat *format)
{
	return SDL_MapRGB(format, 255, 0, 255);
}

void Panel::clearScreen(SDL_Surface *dst)
{
	SDL_FillRect(dst, nullptr, SDL_MapRGB(dst->format, 0, 0, 0));
}

void Panel::drawCursor(const Surface &cursor, SDL_Surface *dst)
{
	auto *cursorSurface = cursor.getSurface();
	SDL_SetColorKey(cursorSurface, SDL_TRUE, this->getMagenta(dst->format));

	const auto cursorScale = 2.0;
	auto mousePosition = this->getMousePosition();
	auto cursorRect = SDL_Rect();
	cursorRect.x = mousePosition.getX();
	cursorRect.y = mousePosition.getY();
	cursorRect.w = static_cast<int>(cursorSurface->w * cursorScale);
	cursorRect.h = static_cast<int>(cursorSurface->h * cursorScale);
	SDL_BlitScaled(cursorSurface, nullptr, dst, &cursorRect);
}

void Panel::drawScaledToNative(const Surface &surface, int x, int y, int w, int h,
	SDL_Surface *dst)
{
	auto nativePoint = this->originalPointToNative(Int2(x, y));

	auto rect = SDL_Rect();
	rect.x = nativePoint.getX();
	rect.y = nativePoint.getY();
	rect.w = static_cast<int>(w * this->getDrawScale());
	rect.h = static_cast<int>(h * this->getDrawScale());

	auto *baseSurface = surface.getSurface();
	SDL_BlitScaled(baseSurface, nullptr, dst, &rect);
}

void Panel::drawScaledToNative(const Surface &surface, SDL_Surface *dst)
{
	this->drawScaledToNative(surface, surface.getX(), surface.getY(),
		surface.getWidth(), surface.getHeight(), dst);
}

void Panel::drawLetterbox(const Surface &background, SDL_Surface *dst,
	const SDL_Rect *letterbox)
{
	auto *baseSurface = background.getSurface();
	SDL_BlitScaled(baseSurface, nullptr, dst, const_cast<SDL_Rect*>(letterbox));
}
