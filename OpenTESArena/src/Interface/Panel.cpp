#include "SDL.h"

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
#include "../Math/Rect.h"
#include "../Media/Color.h"
#include "../Media/MusicName.h"
#include "../Media/TextureName.h"
#include "../Media/TextureSequenceName.h"

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
	auto changeToMainMenu = [gameState]()
	{
		gameState->setPanel(std::unique_ptr<Panel>(new MainMenuPanel(gameState)));
	};

	auto changeToIntroStory = [gameState, changeToMainMenu]()
	{
		// Lots of text to read, so give each image extra time.
		double secondsPerImage = 14.0;
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
		double secondsToDisplay = 5.0;
		auto quotePanel = std::unique_ptr<Panel>(new ImagePanel(
			gameState,
			TextureName::IntroQuote,
			secondsToDisplay,
			changeToScrolling));
		gameState->setPanel(std::move(quotePanel));
	};

	auto changeToTitle = [gameState, changeToQuote]()
	{
		double secondsToDisplay = 5.0;
		auto titlePanel = std::unique_ptr<Panel>(new ImagePanel(
			gameState,
			TextureName::IntroTitle,
			secondsToDisplay,
			changeToQuote));
		gameState->setPanel(std::move(titlePanel));
	};

	return std::unique_ptr<Panel>(new CinematicPanel(gameState,
		TextureSequenceName::IntroBook, 0.142 /* roughly 7fps */,
		changeToTitle));
}

GameState *Panel::getGameState() const
{
	return this->gameStatePtr;
}

double Panel::getDrawScale() const
{
	auto letterbox = this->getGameState()->getLetterboxDimensions();
	return static_cast<double>(letterbox.w) / static_cast<double>(ORIGINAL_WIDTH);
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
	Rect rectangle(letterbox.x, letterbox.y,
		letterbox.w, letterbox.h);
	return rectangle.contains(point);
}

Int2 Panel::nativePointToOriginal(const Int2 &point) const
{
	const double drawScale = this->getDrawScale();
	auto letterbox = this->getGameState()->getLetterboxDimensions();
	return Int2(
		(point.getX() - letterbox.x) / drawScale,
		(point.getY() - letterbox.y) / drawScale);
}

Int2 Panel::originalPointToNative(const Int2 &point) const
{
	const double drawScale = this->getDrawScale();
	auto letterbox = this->getGameState()->getLetterboxDimensions();
	return Int2(
		(point.getX() * drawScale) + letterbox.x,
		(point.getY() * drawScale) + letterbox.y);
}

unsigned int Panel::getFormattedRGB(const Color &color,
	SDL_PixelFormat *format) const
{
	return SDL_MapRGB(format, color.getR(), color.getG(), color.getB());
}

void Panel::clearScreen(SDL_Renderer *renderer)
{
	SDL_RenderClear(renderer);
}

void Panel::drawCursor(const Surface &cursor, SDL_Renderer *renderer)
{
	auto *cursorSurface = cursor.getSurface();

	// This color key should still carry over to the SDL_Texture. Otherwise,
	// implement transparency and blending.
	SDL_SetColorKey(cursorSurface, SDL_TRUE, Color::Black.toRGB());

	SDL_Texture *cursorTexture = SDL_CreateTextureFromSurface(
		renderer, cursorSurface);

	const double cursorScale = 2.0;
	auto mousePosition = this->getMousePosition();
	SDL_Rect cursorRect;
	cursorRect.x = mousePosition.getX();
	cursorRect.y = mousePosition.getY();
	cursorRect.w = static_cast<int>(cursorSurface->w * cursorScale);
	cursorRect.h = static_cast<int>(cursorSurface->h * cursorScale);

	SDL_RenderCopy(renderer, cursorTexture, nullptr, &cursorRect);
	SDL_DestroyTexture(cursorTexture);
}

void Panel::drawScaledToNative(const SDL_Texture *texture, int x, int y,
	int w, int h, SDL_Renderer *renderer)
{
	const double drawScale = this->getDrawScale();
	auto nativePoint = this->originalPointToNative(Int2(x, y));

	SDL_Rect rect;
	rect.x = nativePoint.getX();
	rect.y = nativePoint.getY();
	rect.w = static_cast<int>(w * drawScale);
	rect.h = static_cast<int>(h * drawScale);

	SDL_RenderCopy(renderer, const_cast<SDL_Texture*>(texture), nullptr, &rect);
}

void Panel::drawScaledToNative(const SDL_Texture *texture, int x, int y,
	SDL_Renderer *renderer)
{
	int w, h;
	SDL_QueryTexture(const_cast<SDL_Texture*>(texture), nullptr, nullptr, &w, &h);

	this->drawScaledToNative(texture, x, y, w, h, renderer);
}

void Panel::drawScaledToNative(const SDL_Texture *texture, SDL_Renderer *renderer)
{
	// Draw at the upper left corner of the letterbox.
	int x = 0;
	int y = 0;
	this->drawScaledToNative(texture, x, y, renderer);
}

void Panel::drawScaledToNative(const Surface &surface, int x, int y,
	int w, int h, SDL_Renderer *renderer)
{
	// This is very slow as it is creating a new texture for the surface every call.
	// Only use this for compatibility until all panels use SDL_Textures for rendering.
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface.getSurface());
	this->drawScaledToNative(texture, x, y, w, h, renderer);
	SDL_DestroyTexture(texture);
}

void Panel::drawScaledToNative(const Surface &surface, int x, int y,
	SDL_Renderer *renderer)
{
	this->drawScaledToNative(surface, x, y, surface.getWidth(),
		surface.getHeight(), renderer);
}

void Panel::drawScaledToNative(const Surface &surface, SDL_Renderer *renderer)
{
	this->drawScaledToNative(surface, surface.getX(), surface.getY(), renderer);
}

void Panel::drawLetterbox(const SDL_Texture *texture, SDL_Renderer *renderer,
	const SDL_Rect *letterbox)
{
	SDL_RenderCopy(renderer, const_cast<SDL_Texture*>(texture), nullptr, letterbox);
}
