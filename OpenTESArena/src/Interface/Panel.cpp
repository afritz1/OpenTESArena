#include <vector>

#include "SDL.h"

#include "Panel.h"

#include "CinematicPanel.h"
#include "ImageSequencePanel.h"
#include "ImagePanel.h"
#include "MainMenuPanel.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Game/Game.h"
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
#include "../Rendering/Surface.h"

#include "components/vfs/manager.hpp"

Panel::Panel(Game *game)
{
	this->game = game;
}

Panel::~Panel()
{
	// Game is owned by the Game object.
}

SDL_Texture *Panel::createTooltip(const std::string &text,
	const Font &font, Renderer &renderer)
{
	const Color textColor(255, 255, 255, 255);
	const Color backColor(32, 32, 32, 192);

	// Create text.
	const TextBox textBox(0, 0, textColor, text, font, TextAlignment::Left, renderer);
	SDL_Surface *textSurface = textBox.getSurface();

	// Create background. Make it a little bigger than the text box.
	const int padding = 4;
	SDL_Surface *background = Surface::createSurfaceWithFormat(
		textSurface->w + padding, textSurface->h + padding, 
		Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);
	SDL_FillRect(background, nullptr, SDL_MapRGBA(background->format, backColor.getR(),
		backColor.getG(), backColor.getB(), backColor.getA()));

	// Offset the text from the top left corner by a bit so it isn't against the side 
	// of the tooltip (for aesthetic purposes).
	SDL_Rect rect;
	rect.x = padding / 2;
	rect.y = padding / 2;
	rect.w = textSurface->w;
	rect.h = textSurface->h;

	// Draw the text onto the background.
	SDL_BlitSurface(textSurface, nullptr, background, &rect);

	// Create a hardware texture for the tooltip.
	SDL_Texture *tooltip = renderer.createTextureFromSurface(background);
	SDL_FreeSurface(background);

	return tooltip;
}

std::unique_ptr<Panel> Panel::defaultPanel(Game *game)
{
	// If the intro skip option is set, then jump to the main menu.
	if (game->getOptions().introIsSkipped())
	{
		return std::unique_ptr<Panel>(new MainMenuPanel(game));
	}

	// All of these lambdas are linked together like a stack by each panel's last
	// argument.
	auto changeToMainMenu = [](Game *game)
	{
		std::unique_ptr<Panel> mainMenuPanel(new MainMenuPanel(game));
		game->setPanel(std::move(mainMenuPanel));
	};

	auto changeToIntroStory = [changeToMainMenu](Game *game)
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
			game,
			paletteNames,
			textureNames,
			imageDurations,
			changeToMainMenu));

		game->setPanel(std::move(introStoryPanel));
	};

	auto changeToScrolling = [changeToIntroStory](Game *game)
	{
		std::unique_ptr<Panel> scrollingPanel(new CinematicPanel(
			game,
			TextureFile::fromName(TextureSequenceName::OpeningScroll),
			PaletteFile::fromName(PaletteName::Default),
			0.042,
			changeToIntroStory));
		game->setPanel(std::move(scrollingPanel));
	};

	auto changeToQuote = [changeToScrolling](Game *game)
	{
		const double secondsToDisplay = 5.0;
		std::unique_ptr<Panel> quotePanel(new ImagePanel(
			game,
			PaletteFile::fromName(PaletteName::BuiltIn),
			TextureFile::fromName(TextureName::IntroQuote),
			secondsToDisplay,
			changeToScrolling));
		game->setPanel(std::move(quotePanel));
	};

	auto makeIntroTitlePanel = [changeToQuote, game]()
	{
		const double secondsToDisplay = 5.0;
		std::unique_ptr<Panel> titlePanel(new ImagePanel(
			game,
			PaletteFile::fromName(PaletteName::BuiltIn),
			TextureFile::fromName(TextureName::IntroTitle),
			secondsToDisplay,
			changeToQuote));
		return std::move(titlePanel);
	};

	auto changeToTitle = [makeIntroTitlePanel, changeToQuote](Game *game)
	{
		std::unique_ptr<Panel> titlePanel = makeIntroTitlePanel();
		game->setPanel(std::move(titlePanel));
	};

	/*auto makeIntroBookPanel = [changeToTitle, game]()
	{
		std::unique_ptr<Panel> introBook(new CinematicPanel(
			game,
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

Game *Panel::getGame() const
{
	return this->game;
}

double Panel::getCursorScale() const
{
	double cursorScale = this->getGame()->getOptions().getCursorScale();
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

void Panel::tick(double dt)
{
	// Do nothing by default.
	static_cast<void>(dt);
}
