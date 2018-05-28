#include <algorithm>
#include <cassert>
#include <limits>

#include "SDL.h"

#include "CursorAlignment.h"
#include "OptionsPanel.h"
#include "PauseMenuPanel.h"
#include "RichTextString.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Entities/Player.h"
#include "../Game/Game.h"
#include "../Game/GameData.h"
#include "../Game/Options.h"
#include "../Game/PlayerInterface.h"
#include "../Media/AudioManager.h"
#include "../Media/Color.h"
#include "../Media/FontManager.h"
#include "../Media/FontName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Texture.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

namespace
{
	// Screen locations for various options things.
	const Int2 TabsOrigin(3, 38);
	const Int2 TabsDimensions(54, 16);
	const Int2 ListOrigin(
		TabsOrigin.x + TabsDimensions.x + 5,
		TabsOrigin.y);
	const Int2 ListDimensions(254, TabsDimensions.y * 5);
	const Int2 DescriptionOrigin(
		TabsOrigin.x + 2,
		TabsOrigin.y + (TabsDimensions.y * 5) + 3);

	const Rect GraphicsTabRect(
		TabsOrigin.x,
		TabsOrigin.y,
		TabsDimensions.x,
		TabsDimensions.y);
	const Rect AudioTabRect(
		TabsOrigin.x,
		TabsOrigin.y + TabsDimensions.y,
		TabsDimensions.x,
		TabsDimensions.y);
	const Rect InputTabRect(
		TabsOrigin.x,
		TabsOrigin.y + (TabsDimensions.y * 2),
		TabsDimensions.x,
		TabsDimensions.y);
	const Rect MiscTabRect(
		TabsOrigin.x,
		TabsOrigin.y + (TabsDimensions.y * 3),
		TabsDimensions.x,
		TabsDimensions.y);
	const Rect DevTabRect(
		TabsOrigin.x,
		TabsOrigin.y + (TabsDimensions.y * 4),
		TabsDimensions.x,
		TabsDimensions.y);
}

OptionsPanel::Option::Option(const std::string &name, std::string &&tooltip, Type type)
	: name(name), tooltip(std::move(tooltip))
{
	this->type = type;
}

OptionsPanel::Option::Option(const std::string &name, Type type)
	: Option(name, std::string(), type) { }

const std::string &OptionsPanel::Option::getName() const
{
	return this->name;
}

const std::string &OptionsPanel::Option::getTooltip() const
{
	return this->tooltip;
}

OptionsPanel::Option::Type OptionsPanel::Option::getType() const
{
	return this->type;
}

OptionsPanel::BoolOption::BoolOption(const std::string &name, std::string &&tooltip,
	bool value, Callback &&callback)
	: Option(name, std::move(tooltip), Option::Type::Bool), callback(std::move(callback))
{
	this->value = value;
}

OptionsPanel::BoolOption::BoolOption(const std::string &name, bool value, Callback &&callback)
	: BoolOption(name, std::string(), value, std::move(callback)) { }

std::string OptionsPanel::BoolOption::getDisplayedValue() const
{
	return this->value ? "true" : "false";
}

void OptionsPanel::BoolOption::toggle()
{
	this->value = !this->value;
	this->callback(this->value);
}

OptionsPanel::IntOption::IntOption(const std::string &name, std::string &&tooltip, int value,
	int delta, int min, int max, Callback &&callback)
	: Option(name, std::move(tooltip), Option::Type::Int), callback(std::move(callback))
{
	this->value = value;
	this->delta = delta;
	this->min = min;
	this->max = max;
}

OptionsPanel::IntOption::IntOption(const std::string &name, int value, int delta, int min, int max,
	Callback &&callback)
	: IntOption(name, std::string(), value, delta, min, max, std::move(callback)) { }

int OptionsPanel::IntOption::getNext() const
{
	return std::min(this->value + this->delta, this->max);
}

int OptionsPanel::IntOption::getPrev() const
{
	return std::max(this->value - this->delta, this->min);
}

std::string OptionsPanel::IntOption::getDisplayedValue() const
{
	return (this->displayOverrides.size() > 0) ?
		this->displayOverrides.at(this->value) : std::to_string(this->value);
}

void OptionsPanel::IntOption::set(int value)
{
	this->value = value;
	this->callback(this->value);
}

void OptionsPanel::IntOption::setDisplayOverrides(std::vector<std::string> &&displayOverrides)
{
	this->displayOverrides = std::move(displayOverrides);
}

OptionsPanel::DoubleOption::DoubleOption(const std::string &name, std::string &&tooltip,
	double value, double delta, double min, double max, int precision, Callback &&callback)
	: Option(name, std::move(tooltip), Option::Type::Double), callback(std::move(callback))
{
	this->value = value;
	this->delta = delta;
	this->min = min;
	this->max = max;
	this->precision = precision;
}

OptionsPanel::DoubleOption::DoubleOption(const std::string &name, double value, double delta,
	double min, double max, int precision, Callback &&callback)
	: DoubleOption(name, std::string(), value, delta, min, max, precision, std::move(callback)) { }

double OptionsPanel::DoubleOption::getNext() const
{
	return std::min(this->value + this->delta, this->max);
}

double OptionsPanel::DoubleOption::getPrev() const
{
	return std::max(this->value - this->delta, this->min);
}

std::string OptionsPanel::DoubleOption::getDisplayedValue() const
{
	return String::fixedPrecision(this->value, this->precision);
}

void OptionsPanel::DoubleOption::set(double value)
{
	this->value = value;
	this->callback(this->value);
}

OptionsPanel::StringOption::StringOption(const std::string &name, std::string &&tooltip,
	std::string &&value, Callback &&callback)
	: Option(name, std::move(tooltip), Option::Type::String), value(std::move(value)),
	callback(std::move(callback)) { }

OptionsPanel::StringOption::StringOption(const std::string &name, std::string &&value,
	Callback &&callback)
	: StringOption(name, std::string(), std::move(value), std::move(callback)) { }

std::string OptionsPanel::StringOption::getDisplayedValue() const
{
	return this->value;
}

void OptionsPanel::StringOption::set(std::string &&value)
{
	this->value = std::move(value);
	this->callback(this->value);
}

// Tabs.
const std::string OptionsPanel::GRAPHICS_TAB_NAME = "Graphics";
const std::string OptionsPanel::AUDIO_TAB_NAME = "Audio";
const std::string OptionsPanel::INPUT_TAB_NAME = "Input";
const std::string OptionsPanel::MISC_TAB_NAME = "Misc";
const std::string OptionsPanel::DEV_TAB_NAME = "Dev";

// Graphics.
const std::string OptionsPanel::CURSOR_SCALE_NAME = "Cursor Scale";
const std::string OptionsPanel::FPS_LIMIT_NAME = "FPS Limit";
const std::string OptionsPanel::FULLSCREEN_NAME = "Fullscreen";
const std::string OptionsPanel::LETTERBOX_MODE_NAME = "Letterbox Mode";
const std::string OptionsPanel::MODERN_INTERFACE_NAME = "Modern Interface";
const std::string OptionsPanel::RENDER_THREADS_MODE_NAME = "Render Threads Mode";
const std::string OptionsPanel::RESOLUTION_SCALE_NAME = "Resolution Scale";
const std::string OptionsPanel::VERTICAL_FOV_NAME = "Vertical FOV";

// Audio.
const std::string OptionsPanel::SOUND_RESAMPLING_NAME = "Sound Resampling";

// Input.
const std::string OptionsPanel::HORIZONTAL_SENSITIVITY_NAME = "Horizontal Sensitivity";
const std::string OptionsPanel::VERTICAL_SENSITIVITY_NAME = "Vertical Sensitivity";
const std::string OptionsPanel::CAMERA_PITCH_LIMIT_NAME = "Camera Pitch Limit";

// Misc.
const std::string OptionsPanel::SHOW_COMPASS_NAME = "Show Compass";
const std::string OptionsPanel::SKIP_INTRO_NAME = "Skip Intro";

// Dev.
const std::string OptionsPanel::COLLISION_NAME = "Collision";
const std::string OptionsPanel::SHOW_DEBUG_NAME = "Show Debug";

OptionsPanel::OptionsPanel(Game &game)
	: Panel(game)
{
	this->titleTextBox = [&game]()
	{
		const Int2 center(160, 24);

		const RichTextString richText(
			"Options",
			FontName::A,
			Color::White,
			TextAlignment::Center,
			game.getFontManager());

		return std::make_unique<TextBox>(center, richText, game.getRenderer());
	}();

	this->backToPauseMenuTextBox = [&game]()
	{
		const Int2 center(
			Renderer::ORIGINAL_WIDTH - 30,
			Renderer::ORIGINAL_HEIGHT - 15);

		const RichTextString richText(
			"Return",
			FontName::Arena,
			Color::White,
			TextAlignment::Center,
			game.getFontManager());

		return std::make_unique<TextBox>(center, richText, game.getRenderer());
	}();

	// Lambda for creating tab text boxes.
	auto makeTabTextBox = [&game](const Int2 &center, const std::string &text)
	{
		const RichTextString richText(
			text,
			FontName::Arena,
			Color::White,
			TextAlignment::Center,
			game.getFontManager());

		return std::make_unique<TextBox>(
			center, richText, game.getRenderer());
	};

	const Int2 initialTabCenter(
		GraphicsTabRect.getLeft() + (GraphicsTabRect.getWidth() / 2),
		GraphicsTabRect.getTop() + (GraphicsTabRect.getHeight() / 2));
	this->graphicsTextBox = makeTabTextBox(
		initialTabCenter, OptionsPanel::GRAPHICS_TAB_NAME);
	this->audioTextBox = makeTabTextBox(
		initialTabCenter + Int2(0, TabsDimensions.y),
		OptionsPanel::AUDIO_TAB_NAME);
	this->inputTextBox = makeTabTextBox(
		initialTabCenter + Int2(0, TabsDimensions.y * 2),
		OptionsPanel::INPUT_TAB_NAME);
	this->miscTextBox = makeTabTextBox(
		initialTabCenter + Int2(0, TabsDimensions.y * 3),
		OptionsPanel::MISC_TAB_NAME);
	this->devTextBox = makeTabTextBox(
		initialTabCenter + Int2(0, TabsDimensions.y * 4),
		OptionsPanel::DEV_TAB_NAME);

	this->backToPauseMenuButton = [this]()
	{
		const Int2 center(
			Renderer::ORIGINAL_WIDTH - 30,
			Renderer::ORIGINAL_HEIGHT - 15);

		auto function = [](Game &game)
		{
			game.setPanel<PauseMenuPanel>(game);
		};

		return Button<Game&>(center, 40, 16, function);
	}();

	this->tabButton = []()
	{
		auto function = [](OptionsPanel &panel, OptionsPanel::Tab tab)
		{
			// Update display if the tab values are different.
			const bool tabsAreEqual = panel.tab == tab;
			panel.tab = tab;

			if (!tabsAreEqual)
			{
				panel.updateVisibleOptionTextBoxes();
			}
		};

		return Button<OptionsPanel&, OptionsPanel::Tab>(function);
	}();

	const auto &options = game.getOptions();

	// Create graphics options.
	this->graphicsOptions.push_back(std::make_unique<BoolOption>(
		OptionsPanel::FULLSCREEN_NAME,
		options.getGraphics_Fullscreen(),
		[this](bool value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		auto &renderer = game.getRenderer();
		options.setGraphics_Fullscreen(value);
		renderer.setFullscreen(value);
	}));

	this->graphicsOptions.push_back(std::make_unique<IntOption>(
		OptionsPanel::FPS_LIMIT_NAME,
		options.getGraphics_TargetFPS(),
		5,
		Options::MIN_FPS,
		std::numeric_limits<int>::max(),
		[this](int value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		options.setGraphics_TargetFPS(value);
	}));

	this->graphicsOptions.push_back(std::make_unique<DoubleOption>(
		OptionsPanel::RESOLUTION_SCALE_NAME,
		"Percent of the window resolution to use for software rendering.\nThis has a significant impact on performance.",
		options.getGraphics_ResolutionScale(),
		0.050,
		Options::MIN_RESOLUTION_SCALE,
		Options::MAX_RESOLUTION_SCALE,
		2,
		[this](double value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		options.setGraphics_ResolutionScale(value);

		// Resize the game world rendering.
		auto &renderer = game.getRenderer();
		const Int2 windowDimensions = renderer.getWindowDimensions();
		const bool fullGameWindow = options.getGraphics_ModernInterface();
		renderer.resize(windowDimensions.x, windowDimensions.y,
			value, fullGameWindow);
	}));

	this->graphicsOptions.push_back(std::make_unique<DoubleOption>(
		OptionsPanel::VERTICAL_FOV_NAME,
		"Recommended 60.0 for classic mode.",
		options.getGraphics_VerticalFOV(),
		5.0,
		Options::MIN_VERTICAL_FOV,
		Options::MAX_VERTICAL_FOV,
		1,
		[this](double value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		options.setGraphics_VerticalFOV(value);
	}));

	auto letterboxModeOption = std::make_unique<IntOption>(
		OptionsPanel::LETTERBOX_MODE_NAME,
		"Determines the aspect ratio of the game UI. The weapon animation\nin modern mode is unaffected by this.",
		options.getGraphics_LetterboxMode(),
		1,
		Options::MIN_LETTERBOX_MODE,
		Options::MAX_LETTERBOX_MODE,
		[this](int value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		auto &renderer = game.getRenderer();
		options.setGraphics_LetterboxMode(value);
		renderer.setLetterboxMode(value);
	});

	letterboxModeOption->setDisplayOverrides({ "16:10", "4:3", "Stretch" });
	this->graphicsOptions.push_back(std::move(letterboxModeOption));

	this->graphicsOptions.push_back(std::make_unique<DoubleOption>(
		OptionsPanel::CURSOR_SCALE_NAME,
		options.getGraphics_CursorScale(),
		0.10,
		Options::MIN_CURSOR_SCALE,
		Options::MAX_CURSOR_SCALE,
		1,
		[this](double value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		options.setGraphics_CursorScale(value);
	}));

	this->graphicsOptions.push_back(std::make_unique<BoolOption>(
		OptionsPanel::MODERN_INTERFACE_NAME,
		"Modern mode uses a minimal interface with free-look.",
		options.getGraphics_ModernInterface(),
		[this](bool value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		options.setGraphics_ModernInterface(value);

		// If classic mode, make sure the player is looking straight forward.
		// This is a restriction on the camera to retain the original feel.
		const bool isModernMode = value;
		if (!isModernMode)
		{
			auto &player = game.getGameData().getPlayer();
			const Double2 groundDirection = player.getGroundDirection();
			const Double3 lookAtPoint = player.getPosition() +
				Double3(groundDirection.x, 0.0, groundDirection.y);
			player.lookAt(lookAtPoint);
		}

		// Resize the game world rendering.
		auto &renderer = game.getRenderer();
		const Int2 windowDimensions = renderer.getWindowDimensions();
		const bool fullGameWindow = isModernMode;
		renderer.resize(windowDimensions.x, windowDimensions.y,
			options.getGraphics_ResolutionScale(), fullGameWindow);
	}));

	auto renderThreadsModeOption = std::make_unique<IntOption>(
		OptionsPanel::RENDER_THREADS_MODE_NAME,
		"Determines the number of CPU threads to use for rendering.\nThis has a significant impact on performance.",
		options.getGraphics_RenderThreadsMode(),
		1,
		Options::MIN_RENDER_THREADS_MODE,
		Options::MAX_RENDER_THREADS_MODE,
		[this](int value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		auto &renderer = game.getRenderer();
		options.setGraphics_RenderThreadsMode(value);
		renderer.setRenderThreadsMode(value);
	});

	renderThreadsModeOption->setDisplayOverrides({ "Low", "Medium", "High", "Max" });
	this->graphicsOptions.push_back(std::move(renderThreadsModeOption));

	// Create audio options.
	auto soundResamplingOption = std::make_unique<IntOption>(
		OptionsPanel::SOUND_RESAMPLING_NAME,
		"Affects quality of sounds. Results may vary depending on\nOpenAL version.",
		options.getAudio_SoundResampling(),
		1,
		0,
		Options::RESAMPLING_OPTION_COUNT - 1,
		[this](int value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		options.setAudio_SoundResampling(value);

		// If the sound resampling extension is supported, update the audio manager sources.
		auto &audioManager = game.getAudioManager();
		if (audioManager.hasResamplerExtension())
		{
			audioManager.setResamplingOption(value);
		}
	});

	soundResamplingOption->setDisplayOverrides({ "Default", "Fastest", "Medium", "Best" });
	this->audioOptions.push_back(std::move(soundResamplingOption));

	// Create input options.
	this->inputOptions.push_back(std::make_unique<DoubleOption>(
		OptionsPanel::HORIZONTAL_SENSITIVITY_NAME,
		options.getInput_HorizontalSensitivity(),
		0.10,
		Options::MIN_HORIZONTAL_SENSITIVITY,
		Options::MAX_HORIZONTAL_SENSITIVITY,
		1,
		[this](double value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		options.setInput_HorizontalSensitivity(value);
	}));

	this->inputOptions.push_back(std::make_unique<DoubleOption>(
		OptionsPanel::VERTICAL_SENSITIVITY_NAME,
		"Only affects camera look in modern mode.",
		options.getInput_VerticalSensitivity(),
		0.10,
		Options::MIN_VERTICAL_SENSITIVITY,
		Options::MAX_VERTICAL_SENSITIVITY,
		1,
		[this](double value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		options.setInput_VerticalSensitivity(value);
	}));

	this->inputOptions.push_back(std::make_unique<DoubleOption>(
		OptionsPanel::CAMERA_PITCH_LIMIT_NAME,
		"Determines how far above or below the horizon the camera can\nlook in modern mode.",
		options.getInput_CameraPitchLimit(),
		5.0,
		Options::MIN_CAMERA_PITCH_LIMIT,
		Options::MAX_CAMERA_PITCH_LIMIT,
		1,
		[this](double value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		options.setInput_CameraPitchLimit(value);

		// Reset player view to forward.
		auto &player = game.getGameData().getPlayer();
		const Double2 groundDirection = player.getGroundDirection();
		const Double3 lookAtPoint = player.getPosition() +
			Double3(groundDirection.x, 0.0, groundDirection.y);
		player.lookAt(lookAtPoint);
	}));

	// Create miscellaneous options.
	this->miscOptions.push_back(std::make_unique<BoolOption>(
		OptionsPanel::SHOW_COMPASS_NAME,
		options.getMisc_ShowCompass(),
		[this](bool value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		options.setMisc_ShowCompass(value);
	}));

	this->miscOptions.push_back(std::make_unique<BoolOption>(
		OptionsPanel::SKIP_INTRO_NAME,
		"Skips startup logo and related screens.",
		options.getMisc_SkipIntro(),
		[this](bool value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		options.setMisc_SkipIntro(value);
	}));

	// Create developer options.
	this->devOptions.push_back(std::make_unique<BoolOption>(
		OptionsPanel::COLLISION_NAME,
		"Enables player collision (not fully implemented yet).",
		options.getMisc_Collision(),
		[this](bool value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		options.setMisc_Collision(value);
	}));

	this->devOptions.push_back(std::make_unique<BoolOption>(
		OptionsPanel::SHOW_DEBUG_NAME,
		"Displays debug information in the game world.",
		options.getMisc_ShowDebug(),
		[this](bool value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		options.setMisc_ShowDebug(value);
	}));

	// Set initial tab.
	this->tab = OptionsPanel::Tab::Graphics;

	// Initialize all option text boxes for the initial tab.
	this->updateVisibleOptionTextBoxes();
}

std::vector<std::unique_ptr<OptionsPanel::Option>> &OptionsPanel::getVisibleOptions()
{
	if (this->tab == OptionsPanel::Tab::Graphics)
	{
		return this->graphicsOptions;
	}
	else if (this->tab == OptionsPanel::Tab::Audio)
	{
		return this->audioOptions;
	}
	else if (this->tab == OptionsPanel::Tab::Input)
	{
		return this->inputOptions;
	}
	else if (this->tab == OptionsPanel::Tab::Misc)
	{
		return this->miscOptions;
	}
	else if (this->tab == OptionsPanel::Tab::Dev)
	{
		return this->devOptions;
	}
	else
	{
		throw DebugException("Invalid tab \"" +
			std::to_string(static_cast<int>(this->tab)) + "\".");
	}
}

void OptionsPanel::updateOptionTextBox(int index)
{
	auto &game = this->getGame();
	const auto &visibleOption = this->getVisibleOptions().at(index);

	const RichTextString richText(
		visibleOption->getName() + ": " + visibleOption->getDisplayedValue(),
		FontName::Arena,
		Color::White,
		TextAlignment::Left,
		game.getFontManager());

	this->currentTabTextBoxes.at(index) = std::make_unique<TextBox>(
		ListOrigin.x,
		ListOrigin.y + (richText.getDimensions().y * index),
		richText,
		game.getRenderer());
}

void OptionsPanel::updateVisibleOptionTextBoxes()
{
	auto &game = this->getGame();
	const auto &visibleOptions = this->getVisibleOptions();

	this->currentTabTextBoxes.clear();
	this->currentTabTextBoxes.resize(visibleOptions.size());
	
	for (int i = 0; i < static_cast<int>(visibleOptions.size()); i++)
	{
		this->updateOptionTextBox(i);
	}
}

void OptionsPanel::drawDescription(const std::string &text, Renderer &renderer)
{
	auto &game = this->getGame();

	const RichTextString richText(
		text,
		FontName::Arena,
		Color::White,
		TextAlignment::Left,
		game.getFontManager());

	auto descriptionTextBox = std::make_unique<TextBox>(
		DescriptionOrigin.x,
		DescriptionOrigin.y,
		richText,
		game.getRenderer());

	renderer.drawOriginal(descriptionTextBox->getTexture(),
		descriptionTextBox->getX(), descriptionTextBox->getY());
}

std::pair<SDL_Texture*, CursorAlignment> OptionsPanel::getCurrentCursor() const
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &textureManager = game.getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor),
		PaletteFile::fromName(PaletteName::Default), renderer);
	return std::make_pair(texture.get(), CursorAlignment::TopLeft);
}

void OptionsPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	const bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	const bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
	const bool rightClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_RIGHT);

	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPoint = this->getGame().getRenderer()
		.nativeToOriginal(mousePosition);

	if (escapePressed)
	{
		this->backToPauseMenuButton.click(this->getGame());
	}
	else if (leftClick)
	{
		// Check for various button clicks.
		if (this->backToPauseMenuButton.contains(originalPoint))
		{
			this->backToPauseMenuButton.click(this->getGame());
		}
		else if (GraphicsTabRect.contains(originalPoint))
		{
			this->tabButton.click(*this, OptionsPanel::Tab::Graphics);
		}
		else if (AudioTabRect.contains(originalPoint))
		{
			this->tabButton.click(*this, OptionsPanel::Tab::Audio);
		}
		else if (InputTabRect.contains(originalPoint))
		{
			this->tabButton.click(*this, OptionsPanel::Tab::Input);
		}
		else if (MiscTabRect.contains(originalPoint))
		{
			this->tabButton.click(*this, OptionsPanel::Tab::Misc);
		}
		else if (DevTabRect.contains(originalPoint))
		{
			this->tabButton.click(*this, OptionsPanel::Tab::Dev);
		}
	}

	// Check for option clicks. Left click is "next", right click is "previous", with
	// respect to an option's value in its pre-defined range (if any).
	if (leftClick || rightClick)
	{
		auto &visibleOptions = this->getVisibleOptions();

		for (int i = 0; i < static_cast<int>(visibleOptions.size()); i++)
		{
			const auto &optionTextBox = this->currentTabTextBoxes.at(i);
			const int optionTextBoxHeight = optionTextBox->getRect().getHeight();

			const Rect optionRect(
				ListOrigin.x,
				ListOrigin.y + (optionTextBoxHeight * i),
				ListDimensions.x,
				optionTextBoxHeight);

			// See if the option's rectangle contains the mouse click.
			if (optionRect.contains(originalPoint))
			{
				auto &option = visibleOptions.at(i);

				// Lambdas for modifying the option value based on what it is and whether
				// to try and increment it or decrement it (if that has any meaning).
				auto tryIncrement = [&option]()
				{
					if (option->getType() == Option::Type::Bool)
					{
						BoolOption *boolOpt = static_cast<BoolOption*>(option.get());
						boolOpt->toggle();
					}
					else if (option->getType() == Option::Type::Int)
					{
						IntOption *intOpt = static_cast<IntOption*>(option.get());
						intOpt->set(intOpt->getNext());
					}
					else if (option->getType() == Option::Type::Double)
					{
						DoubleOption *doubleOpt = static_cast<DoubleOption*>(option.get());
						doubleOpt->set(doubleOpt->getNext());
					}
					else if (option->getType() == Option::Type::String)
					{
						// Do nothing.
						static_cast<void>(option);
					}
					else
					{
						throw DebugException("Invalid type \"" +
							std::to_string(static_cast<int>(option->getType())) + "\".");
					}
				};

				auto tryDecrement = [&option]()
				{
					if (option->getType() == Option::Type::Bool)
					{
						BoolOption *boolOpt = static_cast<BoolOption*>(option.get());
						boolOpt->toggle();
					}
					else if (option->getType() == Option::Type::Int)
					{
						IntOption *intOpt = static_cast<IntOption*>(option.get());
						intOpt->set(intOpt->getPrev());
					}
					else if (option->getType() == Option::Type::Double)
					{
						DoubleOption *doubleOpt = static_cast<DoubleOption*>(option.get());
						doubleOpt->set(doubleOpt->getPrev());
					}
					else if (option->getType() == Option::Type::String)
					{
						// Do nothing.
						static_cast<void>(option);
					}
					else
					{
						throw DebugException("Invalid type \"" +
							std::to_string(static_cast<int>(option->getType())) + "\".");
					}
				};

				// Modify the option based on which button was pressed.
				if (leftClick)
				{
					tryIncrement();
				}
				else
				{
					tryDecrement();
				}

				// Update option text.
				this->updateOptionTextBox(i);
				break;
			}
		}
	}
}

void OptionsPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Set palette.
	auto &textureManager = this->getGame().getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw solid background.
	const Color backgroundColor(60, 60, 68);
	renderer.clearOriginal(backgroundColor);

	// Draw return button and tabs.
	Texture tabBackground(Texture::generate(Texture::PatternType::Custom1,
		GraphicsTabRect.getWidth(), GraphicsTabRect.getHeight(), textureManager, renderer));
	for (int i = 0; i < 5; i++)
	{
		renderer.drawOriginal(tabBackground.get(),
			GraphicsTabRect.getLeft(),
			GraphicsTabRect.getTop() + (tabBackground.getHeight() * i));
	}

	Texture returnBackground(Texture::generate(Texture::PatternType::Custom1,
		this->backToPauseMenuButton.getWidth(), this->backToPauseMenuButton.getHeight(),
		textureManager, renderer));
	renderer.drawOriginal(returnBackground.get(), this->backToPauseMenuButton.getX(),
		this->backToPauseMenuButton.getY());

	// Draw text.
	renderer.drawOriginal(this->titleTextBox->getTexture(),
		this->titleTextBox->getX(), this->titleTextBox->getY());
	renderer.drawOriginal(this->backToPauseMenuTextBox->getTexture(),
		this->backToPauseMenuTextBox->getX(), this->backToPauseMenuTextBox->getY());
	renderer.drawOriginal(this->graphicsTextBox->getTexture(),
		this->graphicsTextBox->getX(), this->graphicsTextBox->getY());
	renderer.drawOriginal(this->audioTextBox->getTexture(),
		this->audioTextBox->getX(), this->audioTextBox->getY());
	renderer.drawOriginal(this->inputTextBox->getTexture(),
		this->inputTextBox->getX(), this->inputTextBox->getY());
	renderer.drawOriginal(this->miscTextBox->getTexture(),
		this->miscTextBox->getX(), this->miscTextBox->getY());
	renderer.drawOriginal(this->devTextBox->getTexture(),
		this->devTextBox->getX(), this->devTextBox->getY());

	const auto &inputManager = this->getGame().getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPosition = renderer.nativeToOriginal(mousePosition);

	// Draw each option's text.
	const auto &visibleOptions = this->getVisibleOptions();
	int highlightedOptionIndex = -1;
	for (int i = 0; i < static_cast<int>(visibleOptions.size()); i++)
	{
		const auto &optionTextBox = this->currentTabTextBoxes.at(i);
		const int optionTextBoxHeight = optionTextBox->getRect().getHeight();
		const Rect optionRect(
			ListOrigin.x,
			ListOrigin.y + (optionTextBoxHeight * i),
			ListDimensions.x,
			optionTextBoxHeight);

		const bool optionRectContainsMouse = optionRect.contains(originalPosition);

		// If the options rect contains the mouse cursor, highlight it before drawing text.
		if (optionRectContainsMouse)
		{
			const Color highlightColor = backgroundColor + Color(20, 20, 20);
			renderer.fillOriginalRect(highlightColor,
				optionRect.getLeft(), optionRect.getTop(),
				optionRect.getWidth(), optionRect.getHeight());

			// Store the highlighted option index for tooltip drawing.
			highlightedOptionIndex = i;
		}

		// Draw option text.
		renderer.drawOriginal(optionTextBox->getTexture(),
			optionTextBox->getX(), optionTextBox->getY());
	}

	// Draw description if hovering over an option with a non-empty tooltip.
	if (highlightedOptionIndex != -1)
	{
		const auto &visibleOption = visibleOptions.at(highlightedOptionIndex);
		const std::string &tooltip = visibleOption->getTooltip();

		// Only draw if the tooltip has text.
		if (!tooltip.empty())
		{
			this->drawDescription(tooltip, renderer);
		}
	}
}
