#include <vector>

#include "SDL.h"

#include "CinematicPanel.h"
#include "CursorAlignment.h"
#include "ImageSequencePanel.h"
#include "ImagePanel.h"
#include "MainMenuPanel.h"
#include "Panel.h"
#include "RichTextString.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/FontManager.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureName.h"
#include "../Media/TextureSequenceName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Surface.h"

#include "components/vfs/manager.hpp"

Panel::CursorData::CursorData(const Texture *texture, CursorAlignment alignment)
{
	this->texture = texture;
	this->alignment = alignment;
}

const Texture *Panel::CursorData::getTexture() const
{
	return this->texture;
}

CursorAlignment Panel::CursorData::getAlignment() const
{
	return this->alignment;
}

Panel::Panel(Game &game)
	: game(game) { }

Texture Panel::createTooltip(const std::string &text,
	FontName fontName, FontManager &fontManager, Renderer &renderer)
{
	const Color textColor(255, 255, 255, 255);
	const Color backColor(32, 32, 32, 192);

	const int x = 0;
	const int y = 0;

	const RichTextString richText(
		text,
		fontName,
		textColor,
		TextAlignment::Left,
		fontManager);

	// Create text.
	const TextBox textBox(x, y, richText, renderer);
	const Surface &textSurface = textBox.getSurface();

	// Create background. Make it a little bigger than the text box.
	const int padding = 4;
	Surface background = Surface::createWithFormat(
		textSurface.getWidth() + padding, textSurface.getHeight() + padding,
		Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);
	background.fill(backColor.r, backColor.g, backColor.b, backColor.a);

	// Offset the text from the top left corner by a bit so it isn't against the side 
	// of the tooltip (for aesthetic purposes).
	SDL_Rect rect;
	rect.x = padding / 2;
	rect.y = padding / 2;
	rect.w = textSurface.getWidth();
	rect.h = textSurface.getHeight();

	// Draw the text onto the background.
	SDL_BlitSurface(textSurface.get(), nullptr, background.get(), &rect);

	// Create a hardware texture for the tooltip.
	Texture tooltip = renderer.createTextureFromSurface(background);

	return tooltip;
}

std::unique_ptr<Panel> Panel::defaultPanel(Game &game)
{
	// If not showing the intro, then jump to the main menu.
	if (!game.getOptions().getMisc_ShowIntro())
	{
		return std::make_unique<MainMenuPanel>(game);
	}

	// All of these lambdas are linked together like a stack by each panel's last
	// argument.
	auto changeToMainMenu = [](Game &game)
	{
		game.setPanel<MainMenuPanel>(game);
	};

	auto changeToIntroStory = [changeToMainMenu](Game &game)
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

		game.setPanel<ImageSequencePanel>(game, paletteNames, textureNames,
			imageDurations, changeToMainMenu);
	};

	auto changeToScrolling = [changeToIntroStory](Game &game)
	{
		game.setPanel<CinematicPanel>(
			game,
			PaletteFile::fromName(PaletteName::Default),
			TextureFile::fromName(TextureSequenceName::OpeningScroll),
			0.042,
			changeToIntroStory);
	};

	auto changeToQuote = [changeToScrolling](Game &game)
	{
		const double secondsToDisplay = 5.0;
		game.setPanel<ImagePanel>(
			game,
			PaletteFile::fromName(PaletteName::BuiltIn),
			TextureFile::fromName(TextureName::IntroQuote),
			secondsToDisplay,
			changeToScrolling);
	};

	auto makeIntroTitlePanel = [changeToQuote, &game]()
	{
		const double secondsToDisplay = 5.0;
		return std::make_unique<ImagePanel>(
			game,
			PaletteFile::fromName(PaletteName::BuiltIn),
			TextureFile::fromName(TextureName::IntroTitle),
			secondsToDisplay,
			changeToQuote);
	};

	// Decide how the game starts up. If only the floppy disk data is available,
	// then go to the splash screen. Otherwise, load the intro book video.
	const auto &exeData = game.getMiscAssets().getExeData();
	const bool isFloppyVersion = exeData.isFloppyVersion();
	if (!isFloppyVersion)
	{
		auto changeToTitle = [makeIntroTitlePanel](Game &game)
		{
			game.setPanel(makeIntroTitlePanel());
		};

		auto makeIntroBookPanel = [changeToTitle, &game]()
		{
			return std::make_unique<CinematicPanel>(
				game,
				PaletteFile::fromName(PaletteName::Default),
				TextureFile::fromName(TextureSequenceName::IntroBook),
				1.0 / 7.0,
				changeToTitle);
		};

		return makeIntroBookPanel();
	}
	else
	{
		return makeIntroTitlePanel();
	}
}

Panel::CursorData Panel::getCurrentCursor() const
{
	// Null by default.
	return CursorData(nullptr, CursorAlignment::TopLeft);
}

void Panel::handleEvent(const SDL_Event &e)
{
	// Do nothing by default.
	static_cast<void>(e);
}

void Panel::onPauseChanged(bool paused)
{
	// Do nothing by default.
	static_cast<void>(paused);
}

void Panel::resize(int windowWidth, int windowHeight)
{
	// Do nothing by default.
	static_cast<void>(windowWidth);
	static_cast<void>(windowHeight);
}

Game &Panel::getGame() const
{
	return this->game;
}

void Panel::tick(double dt)
{
	// Do nothing by default.
	static_cast<void>(dt);
}

void Panel::renderSecondary(Renderer &renderer)
{
	// Do nothing by default.
	static_cast<void>(renderer);
}
