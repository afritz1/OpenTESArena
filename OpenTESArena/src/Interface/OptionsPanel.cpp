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
	: Panel(game) { }

bool OptionsPanel::init()
{
	auto &game = this->getGame();

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

	// Create graphics options.
	this->graphicsOptions.emplace_back(OptionsUiModel::makeWindowModeOption(game));
	this->graphicsOptions.emplace_back(OptionsUiModel::makeFpsLimitOption(game));
	this->graphicsOptions.emplace_back(OptionsUiModel::makeResolutionScaleOption(game));
	this->graphicsOptions.emplace_back(OptionsUiModel::makeVerticalFovOption(game));
	this->graphicsOptions.emplace_back(OptionsUiModel::makeLetterboxModeOption(game));
	this->graphicsOptions.emplace_back(OptionsUiModel::makeCursorScaleOption(game));
	this->graphicsOptions.emplace_back(OptionsUiModel::makeModernInterfaceOption(game));
	this->graphicsOptions.emplace_back(OptionsUiModel::makeRenderThreadsModeOption(game));

	// Create audio options.
	this->audioOptions.emplace_back(OptionsUiModel::makeSoundChannelsOption(game));
	this->audioOptions.emplace_back(OptionsUiModel::makeSoundResamplingOption(game));
	this->audioOptions.emplace_back(OptionsUiModel::makeIs3dAudioOption(game));

	// Create input options.
	this->inputOptions.emplace_back(OptionsUiModel::makeHorizontalSensitivityOption(game));
	this->inputOptions.emplace_back(OptionsUiModel::makeVerticalSensitivityOption(game));
	this->inputOptions.emplace_back(OptionsUiModel::makeCameraPitchLimitOption(game));
	this->inputOptions.emplace_back(OptionsUiModel::makePixelPerfectSelectionOption(game));

	// Create miscellaneous options.
	this->miscOptions.emplace_back(OptionsUiModel::makeShowCompassOption(game));
	this->miscOptions.emplace_back(OptionsUiModel::makeShowIntroOption(game));
	this->miscOptions.emplace_back(OptionsUiModel::makeTimeScaleOption(game));
	this->miscOptions.emplace_back(OptionsUiModel::makeChunkDistanceOption(game));
	this->miscOptions.emplace_back(OptionsUiModel::makeStarDensityOption(game));
	this->miscOptions.emplace_back(OptionsUiModel::makePlayerHasLightOption(game));

	// Create developer options.
	this->devOptions.emplace_back(OptionsUiModel::makeCollisionOption(game));
	this->devOptions.emplace_back(OptionsUiModel::makeProfilerLevelOption(game));

	// Set initial tab.
	this->tab = OptionsUiModel::Tab::Graphics;

	// Initialize all option text boxes for the initial tab.
	this->updateVisibleOptionTextBoxes();

	return true;
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
	const auto &visibleOptions = this->getVisibleOptions();
	DebugAssertIndex(visibleOptions, index);
	const auto &visibleOption = visibleOptions[index];

	const auto &fontLibrary = game.getFontLibrary();
	const RichTextString richText(
		visibleOption->getName() + ": " + visibleOption->getDisplayedValue(),
		OptionsUiView::OptionTextBoxFontName,
		OptionsUiView::getOptionTextBoxColor(),
		OptionsUiView::OptionTextBoxTextAlignment,
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
	Texture tabBackground = TextureUtils::generate(
		OptionsUiView::TabBackgroundPatternType,
		graphicsTabRect.getWidth(),
		graphicsTabRect.getHeight(),
		textureManager,
		renderer);

	// @todo: this loop condition should be driven by actual tab count
	for (int i = 0; i < 5; i++)
	{
		const int tabX = graphicsTabRect.getLeft();
		const int tabY = graphicsTabRect.getTop() + (tabBackground.getHeight() * i);
		renderer.drawOriginal(tabBackground, tabX, tabY);
	}

	Texture returnBackground = TextureUtils::generate(
		OptionsUiView::TabBackgroundPatternType,
		this->backToPauseMenuButton.getWidth(),
		this->backToPauseMenuButton.getHeight(),
		textureManager,
		renderer);

	renderer.drawOriginal(returnBackground, this->backToPauseMenuButton.getX(), this->backToPauseMenuButton.getY());
}

void OptionsPanel::drawText(Renderer &renderer)
{
	renderer.drawOriginal(this->titleTextBox->getTexture(), this->titleTextBox->getX(), this->titleTextBox->getY());
	renderer.drawOriginal(this->backToPauseMenuTextBox->getTexture(),
		this->backToPauseMenuTextBox->getX(), this->backToPauseMenuTextBox->getY());

	// Tabs.
	renderer.drawOriginal(this->graphicsTextBox->getTexture(), this->graphicsTextBox->getX(), this->graphicsTextBox->getY());
	renderer.drawOriginal(this->audioTextBox->getTexture(), this->audioTextBox->getX(), this->audioTextBox->getY());
	renderer.drawOriginal(this->inputTextBox->getTexture(), this->inputTextBox->getX(), this->inputTextBox->getY());
	renderer.drawOriginal(this->miscTextBox->getTexture(), this->miscTextBox->getX(), this->miscTextBox->getY());
	renderer.drawOriginal(this->devTextBox->getTexture(), this->devTextBox->getX(), this->devTextBox->getY());
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
			renderer.fillOriginalRect(OptionsUiView::HighlightColor, optionRect.getLeft(),
				optionRect.getTop(), optionRect.getWidth(), optionRect.getHeight());

			// Store the highlighted option index for tooltip drawing.
			highlightedOptionIndex = i;
		}

		// Draw option text.
		renderer.drawOriginal(optionTextBox->getTexture(), optionTextBox->getX(), optionTextBox->getY());

		// Draw description if hovering over an option with a non-empty tooltip.
		if (highlightedOptionIndex.has_value())
		{
			DebugAssertIndex(visibleOptions, *highlightedOptionIndex);
			const auto &visibleOption = visibleOptions[*highlightedOptionIndex];
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
		OptionsUiView::DescriptionTextFontName,
		OptionsUiView::getDescriptionTextColor(),
		OptionsUiView::DescriptionTextAlignment,
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
