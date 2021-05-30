#include <algorithm>
#include <limits>
#include <optional>

#include "SDL.h"

#include "OptionsPanel.h"
#include "OptionsUiController.h"
#include "OptionsUiView.h"
#include "PauseMenuPanel.h"
#include "../Audio/AudioManager.h"
#include "../Entities/Player.h"
#include "../Game/Game.h"
#include "../Game/GameState.h"
#include "../Game/Options.h"
#include "../Game/PlayerInterface.h"
#include "../Media/Color.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../UI/CursorAlignment.h"
#include "../UI/FontLibrary.h"
#include "../UI/FontName.h"
#include "../UI/RichTextString.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"
#include "../UI/Texture.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

OptionsPanel::OptionsPanel(Game &game)
	: Panel(game)
{
	this->titleTextBox = [&game]()
	{
		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			OptionsUiModel::OptionsTitleText,
			OptionsUiView::TitleFontName,
			OptionsUiView::getTitleTextColor(),
			OptionsUiView::TitleTextAlignment,
			fontLibrary);

		return std::make_unique<TextBox>(
			OptionsUiView::TitleTextBoxCenterPoint,
			richText,
			fontLibrary,
			game.getRenderer());
	}();

	this->backToPauseMenuTextBox = [&game]()
	{
		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			OptionsUiModel::BackToPauseMenuText,
			OptionsUiView::BackToPauseMenuFontName,
			OptionsUiView::getBackToPauseMenuTextColor(),
			OptionsUiView::BackToPauseMenuTextAlignment,
			fontLibrary);

		return std::make_unique<TextBox>(
			OptionsUiView::BackToPauseMenuTextBoxCenterPoint,
			richText,
			fontLibrary,
			game.getRenderer());
	}();

	// Lambda for creating tab text boxes.
	auto makeTabTextBox = [&game](int tabIndex, const std::string &text)
	{
		const Rect &graphicsTabRect = OptionsUiView::GraphicsTabRect;
		const Int2 &tabsDimensions = OptionsUiView::TabsDimensions;
		const Int2 initialTabTextCenter(
			graphicsTabRect.getLeft() + (graphicsTabRect.getWidth() / 2),
			graphicsTabRect.getTop() + (graphicsTabRect.getHeight() / 2));
		const Int2 tabOffset(0, tabsDimensions.y * tabIndex);
		const Int2 center = initialTabTextCenter + tabOffset;

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			text,
			OptionsUiView::TabFontName,
			OptionsUiView::getTabTextColor(),
			OptionsUiView::TabTextAlignment,
			fontLibrary);

		return std::make_unique<TextBox>(
			center,
			richText,
			fontLibrary,
			game.getRenderer());
	};

	// @todo: should make this iterable
	this->graphicsTextBox = makeTabTextBox(0, OptionsUiModel::GRAPHICS_TAB_NAME);
	this->audioTextBox = makeTabTextBox(1, OptionsUiModel::AUDIO_TAB_NAME);
	this->inputTextBox = makeTabTextBox(2, OptionsUiModel::INPUT_TAB_NAME);
	this->miscTextBox = makeTabTextBox(3, OptionsUiModel::MISC_TAB_NAME);
	this->devTextBox = makeTabTextBox(4, OptionsUiModel::DEV_TAB_NAME);

	this->backToPauseMenuButton = Button<Game&>(
		OptionsUiView::BackToPauseMenuButtonCenterPoint,
		OptionsUiView::BackToPauseMenuButtonWidth,
		OptionsUiView::BackToPauseMenuButtonHeight,
		OptionsUiController::onBackToPauseMenuButtonSelected);
	this->tabButton = Button<OptionsPanel&, OptionsUiModel::Tab*, OptionsUiModel::Tab>(
		OptionsUiController::onTabButtonSelected);

	const auto &options = game.getOptions();

	// Create graphics options.
	auto windowModeOption = std::make_unique<OptionsUiModel::IntOption>(
		OptionsUiModel::WINDOW_MODE_NAME,
		options.getGraphics_WindowMode(),
		1,
		Options::MIN_WINDOW_MODE,
		Options::MAX_WINDOW_MODE,
		[this](int value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		auto &renderer = game.getRenderer();
		options.setGraphics_WindowMode(value);

		const Renderer::WindowMode mode = [value]()
		{
			switch (value)
			{
			case 0:
				return Renderer::WindowMode::Window;
			case 1:
				return Renderer::WindowMode::BorderlessFull;
			default:
				DebugUnhandledReturnMsg(Renderer::WindowMode, std::to_string(value));
			}
		}();

		renderer.setWindowMode(mode);
	});

	windowModeOption->setDisplayOverrides({ "Window", "Borderless Full" });
	this->graphicsOptions.push_back(std::move(windowModeOption));

	this->graphicsOptions.push_back(std::make_unique<OptionsUiModel::IntOption>(
		OptionsUiModel::FPS_LIMIT_NAME,
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

	this->graphicsOptions.push_back(std::make_unique<OptionsUiModel::DoubleOption>(
		OptionsUiModel::RESOLUTION_SCALE_NAME,
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

	this->graphicsOptions.push_back(std::make_unique<OptionsUiModel::DoubleOption>(
		OptionsUiModel::VERTICAL_FOV_NAME,
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

	auto letterboxModeOption = std::make_unique<OptionsUiModel::IntOption>(
		OptionsUiModel::LETTERBOX_MODE_NAME,
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

	this->graphicsOptions.push_back(std::make_unique<OptionsUiModel::DoubleOption>(
		OptionsUiModel::CURSOR_SCALE_NAME,
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

	this->graphicsOptions.push_back(std::make_unique<OptionsUiModel::BoolOption>(
		OptionsUiModel::MODERN_INTERFACE_NAME,
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
			auto &player = game.getGameState().getPlayer();
			player.setDirectionToHorizon();
		}

		// Resize the game world rendering.
		auto &renderer = game.getRenderer();
		const Int2 windowDimensions = renderer.getWindowDimensions();
		const bool fullGameWindow = isModernMode;
		renderer.resize(windowDimensions.x, windowDimensions.y,
			options.getGraphics_ResolutionScale(), fullGameWindow);
	}));

	auto renderThreadsModeOption = std::make_unique<OptionsUiModel::IntOption>(
		OptionsUiModel::RENDER_THREADS_MODE_NAME,
		"Determines the number of CPU threads to use for rendering.\nThis has a significant impact on performance.\nVery Low: one, Low: 1/4, Medium: 1/2, High: 3/4,\nVery High: all but one, Max: all",
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

	renderThreadsModeOption->setDisplayOverrides({ "Very Low", "Low", "Medium", "High", "Very High", "Max" });
	this->graphicsOptions.push_back(std::move(renderThreadsModeOption));

	// Create audio options.
	this->audioOptions.push_back(std::make_unique<OptionsUiModel::IntOption>(
		OptionsUiModel::SOUND_CHANNELS_NAME,
		"Determines max number of concurrent sounds (including music).\nChanges are applied on next program start.",
		options.getAudio_SoundChannels(),
		1,
		Options::MIN_SOUND_CHANNELS,
		std::numeric_limits<int>::max(),
		[this](int value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		options.setAudio_SoundChannels(value);
	}));

	auto soundResamplingOption = std::make_unique<OptionsUiModel::IntOption>(
		OptionsUiModel::SOUND_RESAMPLING_NAME,
		"Affects quality of sounds. Results may vary depending on OpenAL\nversion.",
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

	this->audioOptions.push_back(std::make_unique<OptionsUiModel::BoolOption>(
		OptionsUiModel::IS_3D_AUDIO_NAME,
		"Determines whether sounds in the game world have a 3D position.\nSet to false for classic behavior.",
		options.getAudio_Is3DAudio(),
		[this](bool value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		options.setAudio_Is3DAudio(value);

		auto &audioManager = game.getAudioManager();
		audioManager.set3D(value);
	}));

	// Create input options.
	this->inputOptions.push_back(std::make_unique<OptionsUiModel::DoubleOption>(
		OptionsUiModel::HORIZONTAL_SENSITIVITY_NAME,
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

	this->inputOptions.push_back(std::make_unique<OptionsUiModel::DoubleOption>(
		OptionsUiModel::VERTICAL_SENSITIVITY_NAME,
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

	this->inputOptions.push_back(std::make_unique<OptionsUiModel::DoubleOption>(
		OptionsUiModel::CAMERA_PITCH_LIMIT_NAME,
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
		auto &player = game.getGameState().getPlayer();
		player.setDirectionToHorizon();
	}));

	this->inputOptions.push_back(std::make_unique<OptionsUiModel::BoolOption>(
		OptionsUiModel::PIXEL_PERFECT_SELECTION_NAME,
		"Changes entity selection so only clicks on opaque places are\nregistered, if enabled.",
		options.getInput_PixelPerfectSelection(),
		[this](bool value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		options.setInput_PixelPerfectSelection(value);
	}));

	// Create miscellaneous options.
	this->miscOptions.push_back(std::make_unique<OptionsUiModel::BoolOption>(
		OptionsUiModel::SHOW_COMPASS_NAME,
		options.getMisc_ShowCompass(),
		[this](bool value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		options.setMisc_ShowCompass(value);
	}));

	this->miscOptions.push_back(std::make_unique<OptionsUiModel::BoolOption>(
		OptionsUiModel::SHOW_INTRO_NAME,
		"Shows startup logo and related screens.",
		options.getMisc_ShowIntro(),
		[this](bool value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		options.setMisc_ShowIntro(value);
	}));

	this->miscOptions.push_back(std::make_unique<OptionsUiModel::DoubleOption>(
		OptionsUiModel::TIME_SCALE_NAME,
		"Affects speed of gameplay. Lower this to simulate the speed of\nlower cycles in DOSBox.",
		options.getMisc_TimeScale(),
		0.050,
		Options::MIN_TIME_SCALE,
		Options::MAX_TIME_SCALE,
		2,
		[this](double value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		options.setMisc_TimeScale(value);
	}));

	this->miscOptions.push_back(std::make_unique<OptionsUiModel::IntOption>(
		OptionsUiModel::CHUNK_DISTANCE_NAME,
		"Affects how many chunks away from the player chunks are\nsimulated and rendered.",
		options.getMisc_ChunkDistance(),
		1,
		Options::MIN_CHUNK_DISTANCE,
		std::numeric_limits<int>::max(),
		[this](int value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		options.setMisc_ChunkDistance(value);
	}));

	auto starDensityOption = std::make_unique<OptionsUiModel::IntOption>(
		OptionsUiModel::STAR_DENSITY_NAME,
		"Determines number of stars in the sky. Changes take effect the next\ntime stars are generated.",
		options.getMisc_StarDensity(),
		1,
		Options::MIN_STAR_DENSITY_MODE,
		Options::MAX_STAR_DENSITY_MODE,
		[this](int value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		options.setMisc_StarDensity(value);
	});

	starDensityOption->setDisplayOverrides({ "Classic", "Moderate", "High" });
	this->miscOptions.push_back(std::move(starDensityOption));

	this->miscOptions.push_back(std::make_unique<OptionsUiModel::BoolOption>(
		OptionsUiModel::PLAYER_HAS_LIGHT_NAME,
		"Whether the player has a light attached like in the original game.",
		options.getMisc_PlayerHasLight(),
		[this](bool value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		options.setMisc_PlayerHasLight(value);
	}));

	// Create developer options.
	this->devOptions.push_back(std::make_unique<OptionsUiModel::BoolOption>(
		OptionsUiModel::COLLISION_NAME,
		"Enables player collision (not fully implemented yet).",
		options.getMisc_Collision(),
		[this](bool value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		options.setMisc_Collision(value);
	}));

	this->devOptions.push_back(std::make_unique<OptionsUiModel::IntOption>(
		OptionsUiModel::PROFILER_LEVEL_NAME,
		"Displays varying levels of profiler information in the game world.",
		options.getMisc_ProfilerLevel(),
		1,
		Options::MIN_PROFILER_LEVEL,
		Options::MAX_PROFILER_LEVEL,
		[this](int value)
	{
		auto &game = this->getGame();
		auto &options = game.getOptions();
		options.setMisc_ProfilerLevel(value);
	}));

	// Set initial tab.
	this->tab = OptionsUiModel::Tab::Graphics;

	// Initialize all option text boxes for the initial tab.
	this->updateVisibleOptionTextBoxes();
}

std::vector<std::unique_ptr<OptionsUiModel::Option>> &OptionsPanel::getVisibleOptions()
{
	if (this->tab == OptionsUiModel::Tab::Graphics)
	{
		return this->graphicsOptions;
	}
	else if (this->tab == OptionsUiModel::Tab::Audio)
	{
		return this->audioOptions;
	}
	else if (this->tab == OptionsUiModel::Tab::Input)
	{
		return this->inputOptions;
	}
	else if (this->tab == OptionsUiModel::Tab::Misc)
	{
		return this->miscOptions;
	}
	else if (this->tab == OptionsUiModel::Tab::Dev)
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

	const auto &fontLibrary = game.getFontLibrary();
	const RichTextString richText(
		visibleOption->getName() + ": " + visibleOption->getDisplayedValue(),
		FontName::Arena,
		Color::White,
		TextAlignment::Left,
		fontLibrary);

	const Int2 &point = OptionsUiView::ListOrigin;
	this->currentTabTextBoxes.at(index) = std::make_unique<TextBox>(
		point.x,
		point.y + (richText.getDimensions().y * index),
		richText,
		fontLibrary,
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

void OptionsPanel::drawReturnButtonsAndTabs(Renderer &renderer)
{
	auto &textureManager = this->getGame().getTextureManager();
	const Rect &graphicsTabRect = OptionsUiView::GraphicsTabRect;
	Texture tabBackground = TextureUtils::generate(TextureUtils::PatternType::Custom1,
		graphicsTabRect.getWidth(), graphicsTabRect.getHeight(), textureManager, renderer);

	for (int i = 0; i < 5; i++)
	{
		const int tabX = graphicsTabRect.getLeft();
		const int tabY = graphicsTabRect.getTop() + (tabBackground.getHeight() * i);
		renderer.drawOriginal(tabBackground, tabX, tabY);
	}

	Texture returnBackground = TextureUtils::generate(TextureUtils::PatternType::Custom1,
		this->backToPauseMenuButton.getWidth(), this->backToPauseMenuButton.getHeight(),
		textureManager, renderer);
	renderer.drawOriginal(returnBackground, this->backToPauseMenuButton.getX(),
		this->backToPauseMenuButton.getY());
}

void OptionsPanel::drawText(Renderer &renderer)
{
	renderer.drawOriginal(this->titleTextBox->getTexture(),
		this->titleTextBox->getX(), this->titleTextBox->getY());
	renderer.drawOriginal(this->backToPauseMenuTextBox->getTexture(),
		this->backToPauseMenuTextBox->getX(), this->backToPauseMenuTextBox->getY());

	// Tabs.
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
}

void OptionsPanel::drawTextOfOptions(Renderer &renderer)
{
	const auto &visibleOptions = this->getVisibleOptions();
	std::optional<int> highlightedOptionIndex;
	for (int i = 0; i < static_cast<int>(visibleOptions.size()); i++)
	{
		const auto &optionTextBox = this->currentTabTextBoxes.at(i);
		const int optionTextBoxHeight = optionTextBox->getRect().getHeight();
		const Rect optionRect(
			OptionsUiView::ListOrigin.x,
			OptionsUiView::ListOrigin.y + (optionTextBoxHeight * i),
			OptionsUiView::ListDimensions.x,
			optionTextBoxHeight);

		const auto &inputManager = this->getGame().getInputManager();
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 originalPosition = renderer.nativeToOriginal(mousePosition);
		const bool optionRectContainsMouse = optionRect.contains(originalPosition);

		// If the options rect contains the mouse cursor, highlight it before drawing text.
		if (optionRectContainsMouse)
		{
			const Color highlightColor = OptionsUiView::BackgroundColor + Color(20, 20, 20);
			renderer.fillOriginalRect(highlightColor,
				optionRect.getLeft(), optionRect.getTop(),
				optionRect.getWidth(), optionRect.getHeight());

			// Store the highlighted option index for tooltip drawing.
			highlightedOptionIndex = i;
		}

		// Draw option text.
		renderer.drawOriginal(optionTextBox->getTexture(),
			optionTextBox->getX(), optionTextBox->getY());

		// Draw description if hovering over an option with a non-empty tooltip.
		if (highlightedOptionIndex.has_value())
		{
			const auto &visibleOption = visibleOptions.at(*highlightedOptionIndex);
			const std::string &tooltip = visibleOption->getTooltip();

			// Only draw if the tooltip has text.
			if (!tooltip.empty())
			{
				this->drawDescription(tooltip, renderer);
			}
		}
	}
}

void OptionsPanel::drawDescription(const std::string &text, Renderer &renderer)
{
	auto &game = this->getGame();
	const auto &fontLibrary = game.getFontLibrary();
	const RichTextString richText(
		text,
		FontName::Arena,
		Color::White,
		TextAlignment::Left,
		fontLibrary);

	const Int2 &point = OptionsUiView::DescriptionOrigin;
	auto descriptionTextBox = std::make_unique<TextBox>(
		point.x,
		point.y,
		richText,
		fontLibrary,
		game.getRenderer());

	renderer.drawOriginal(descriptionTextBox->getTexture(),
		descriptionTextBox->getX(), descriptionTextBox->getY());
}

std::optional<Panel::CursorData> OptionsPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void OptionsPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	const bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	const bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
	const bool rightClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_RIGHT);

	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPoint = this->getGame().getRenderer().nativeToOriginal(mousePosition);

	if (escapePressed)
	{
		this->backToPauseMenuButton.click(this->getGame());
	}
	else if (leftClick)
	{
		// Check for various button clicks.
		// @todo: the tab rects should be pretty easy to iterate over
		if (this->backToPauseMenuButton.contains(originalPoint))
		{
			this->backToPauseMenuButton.click(this->getGame());
		}
		else if (OptionsUiView::GraphicsTabRect.contains(originalPoint))
		{
			this->tabButton.click(*this, &this->tab, OptionsUiModel::Tab::Graphics);
		}
		else if (OptionsUiView::AudioTabRect.contains(originalPoint))
		{
			this->tabButton.click(*this, &this->tab, OptionsUiModel::Tab::Audio);
		}
		else if (OptionsUiView::InputTabRect.contains(originalPoint))
		{
			this->tabButton.click(*this, &this->tab, OptionsUiModel::Tab::Input);
		}
		else if (OptionsUiView::MiscTabRect.contains(originalPoint))
		{
			this->tabButton.click(*this, &this->tab, OptionsUiModel::Tab::Misc);
		}
		else if (OptionsUiView::DevTabRect.contains(originalPoint))
		{
			this->tabButton.click(*this, &this->tab, OptionsUiModel::Tab::Dev);
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
				OptionsUiView::ListOrigin.x,
				OptionsUiView::ListOrigin.y + (optionTextBoxHeight * i),
				OptionsUiView::ListDimensions.x,
				optionTextBoxHeight);

			// See if the option's rectangle contains the mouse click.
			if (optionRect.contains(originalPoint))
			{
				auto &option = visibleOptions.at(i);

				// Lambdas for modifying the option value based on what it is and whether
				// to try and increment it or decrement it (if that has any meaning).
				auto tryIncrement = [&option]()
				{
					if (option->getType() == OptionsUiModel::OptionType::Bool)
					{
						OptionsUiModel::BoolOption *boolOpt = static_cast<OptionsUiModel::BoolOption*>(option.get());
						boolOpt->toggle();
					}
					else if (option->getType() == OptionsUiModel::OptionType::Int)
					{
						OptionsUiModel::IntOption *intOpt = static_cast<OptionsUiModel::IntOption*>(option.get());
						intOpt->set(intOpt->getNext());
					}
					else if (option->getType() == OptionsUiModel::OptionType::Double)
					{
						OptionsUiModel::DoubleOption *doubleOpt = static_cast<OptionsUiModel::DoubleOption*>(option.get());
						doubleOpt->set(doubleOpt->getNext());
					}
					else if (option->getType() == OptionsUiModel::OptionType::String)
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
					if (option->getType() == OptionsUiModel::OptionType::Bool)
					{
						OptionsUiModel::BoolOption *boolOpt = static_cast<OptionsUiModel::BoolOption*>(option.get());
						boolOpt->toggle();
					}
					else if (option->getType() == OptionsUiModel::OptionType::Int)
					{
						OptionsUiModel::IntOption *intOpt = static_cast<OptionsUiModel::IntOption*>(option.get());
						intOpt->set(intOpt->getPrev());
					}
					else if (option->getType() == OptionsUiModel::OptionType::Double)
					{
						OptionsUiModel::DoubleOption *doubleOpt = static_cast<OptionsUiModel::DoubleOption*>(option.get());
						doubleOpt->set(doubleOpt->getPrev());
					}
					else if (option->getType() == OptionsUiModel::OptionType::String)
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

	// Draw solid background.
	renderer.clearOriginal(OptionsUiView::BackgroundColor);

	// Draw elements.
	this->drawReturnButtonsAndTabs(renderer);
	this->drawText(renderer);
	this->drawTextOfOptions(renderer);
}
