#include "SDL.h"

#include "ChooseAttributesPanel.h"
#include "ChooseGenderPanel.h"
#include "ChooseRacePanel.h"
#include "CursorAlignment.h"
#include "MessageBoxSubPanel.h"
#include "RichTextString.h"
#include "Surface.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "TextSubPanel.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/ExeData.h"
#include "../Assets/WorldMapMask.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Math/Rect.h"
#include "../Media/Color.h"
#include "../Media/FontLibrary.h"
#include "../Media/FontName.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../World/LocationUtils.h"

#include "components/utilities/String.h"

ChooseRacePanel::ChooseRacePanel(Game &game)
	: Panel(game)
{
	this->backToGenderButton = []()
	{
		auto function = [](Game &game)
		{
			game.setPanel<ChooseGenderPanel>(game);
		};

		return Button<Game&>(function);
	}();

	this->acceptButton = []()
	{
		auto function = [](Game &game, int raceIndex)
		{
			// Set character creation race index.
			auto &charCreationState = game.getCharacterCreationState();
			charCreationState.setRaceIndex(raceIndex);

			// Generate the race selection message box.
			auto &textureManager = game.getTextureManager();
			auto &renderer = game.getRenderer();

			const Color textColor(52, 24, 8);

			MessageBoxSubPanel::Title messageBoxTitle;
			messageBoxTitle.textBox = [&game, &renderer, &textColor]()
			{
				const auto &exeData = game.getBinaryAssetLibrary().getExeData();
				std::string text = exeData.charCreation.confirmRace;
				text = String::replace(text, '\r', '\n');

				const auto &charCreationState = game.getCharacterCreationState();
				const int raceIndex = charCreationState.getRaceIndex();

				const auto &charCreationProvinceNames = exeData.locations.charCreationProvinceNames;
				DebugAssertIndex(charCreationProvinceNames, raceIndex);
				const std::string &provinceName = charCreationProvinceNames[raceIndex];

				const auto &pluralRaceNames = exeData.races.pluralNames;
				DebugAssertIndex(pluralRaceNames, raceIndex);
				const std::string &pluralRaceName = pluralRaceNames[raceIndex];

				// Replace first %s with province name.
				size_t index = text.find("%s");
				text.replace(index, 2, provinceName);

				// Replace second %s with plural race name.
				index = text.find("%s");
				text.replace(index, 2, pluralRaceName);

				const int lineSpacing = 1;
				const auto &fontLibrary = game.getFontLibrary();
				const RichTextString richText(
					text,
					FontName::A,
					textColor,
					TextAlignment::Center,
					lineSpacing,
					fontLibrary);

				const Int2 center(
					(ArenaRenderUtils::SCREEN_WIDTH / 2),
					(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 22);

				return std::make_unique<TextBox>(center, richText, fontLibrary, renderer);
			}();

			messageBoxTitle.texture = [&textureManager, &renderer, &messageBoxTitle]()
			{
				const int width = messageBoxTitle.textBox->getRect().getWidth() + 22;
				const int height = 60;
				return TextureUtils::generate(TextureUtils::PatternType::Parchment, width, height,
					textureManager, renderer);
			}();

			messageBoxTitle.textureX = (ArenaRenderUtils::SCREEN_WIDTH / 2) -
				(messageBoxTitle.texture.getWidth() / 2) - 1;
			messageBoxTitle.textureY = (ArenaRenderUtils::SCREEN_HEIGHT / 2) -
				(messageBoxTitle.texture.getHeight() / 2) - 21;

			MessageBoxSubPanel::Element messageBoxYes;
			messageBoxYes.textBox = [&game, &renderer, &textColor]()
			{
				const auto &fontLibrary = game.getFontLibrary();
				const RichTextString richText(
					"Yes",
					FontName::A,
					textColor,
					TextAlignment::Center,
					fontLibrary);

				const Int2 center(
					(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
					(ArenaRenderUtils::SCREEN_HEIGHT / 2) + 28);

				return std::make_unique<TextBox>(center, richText, fontLibrary, renderer);
			}();

			messageBoxYes.texture = [&textureManager, &renderer, &messageBoxTitle]()
			{
				const int width = messageBoxTitle.texture.getWidth();
				return TextureUtils::generate(TextureUtils::PatternType::Parchment, width, 40,
					textureManager, renderer);
			}();

			messageBoxYes.function = [](Game &game)
			{
				game.popSubPanel();

				const Color textColor(48, 12, 12);

				// Generate all of the parchments leading up to the attributes panel,
				// and link them together so they appear after each other.
				auto toAttributes = [](Game &game)
				{
					game.popSubPanel();
					game.setPanel<ChooseAttributesPanel>(game);
				};

				auto toFourthSubPanel = [textColor, toAttributes](Game &game)
				{
					game.popSubPanel();

					const Int2 center((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 98);

					const std::string text = [&game]()
					{
						const auto &exeData = game.getBinaryAssetLibrary().getExeData();
						std::string segment = exeData.charCreation.confirmedRace4;
						segment = String::replace(segment, '\r', '\n');

						return segment;
					}();

					const int lineSpacing = 1;

					const RichTextString richText(
						text,
						FontName::Arena,
						textColor,
						TextAlignment::Center,
						lineSpacing,
						game.getFontLibrary());

					const int textureHeight = std::max(richText.getDimensions().y + 8, 40);
					Texture texture = TextureUtils::generate(TextureUtils::PatternType::Parchment,
						richText.getDimensions().x + 20, textureHeight,
						game.getTextureManager(), game.getRenderer());

					const Int2 textureCenter(
						(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
						(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);

					auto fourthSubPanel = std::make_unique<TextSubPanel>(
						game, center, richText, toAttributes, std::move(texture),
						textureCenter);

					game.pushSubPanel(std::move(fourthSubPanel));
				};

				auto toThirdSubPanel = [textColor, toFourthSubPanel](Game &game)
				{
					game.popSubPanel();

					const Int2 center((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 98);

					const std::string text = [&game]()
					{
						const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
						const auto &exeData = binaryAssetLibrary.getExeData();
						std::string segment = exeData.charCreation.confirmedRace3;
						segment = String::replace(segment, '\r', '\n');

						const auto &charCreationState = game.getCharacterCreationState();
						const auto &charClassLibrary = game.getCharacterClassLibrary();
						const int charClassDefID = charCreationState.getClassDefID();
						const auto &charClassDef = charClassLibrary.getDefinition(charClassDefID);

						const auto &preferredAttributes = exeData.charClasses.preferredAttributes;
						DebugAssertIndex(preferredAttributes, charClassDefID);
						const std::string &preferredAttributesStr = preferredAttributes[charClassDefID];

						// Replace first %s with desired class attributes.
						size_t index = segment.find("%s");
						segment.replace(index, 2, preferredAttributesStr);

						// Replace second %s with class name.
						index = segment.find("%s");
						segment.replace(index, 2, charClassDef.getName());

						return segment;
					}();

					const int lineSpacing = 1;

					const RichTextString richText(
						text,
						FontName::Arena,
						textColor,
						TextAlignment::Center,
						lineSpacing,
						game.getFontLibrary());

					const int textureHeight = std::max(richText.getDimensions().y + 18, 40);
					Texture texture = TextureUtils::generate(TextureUtils::PatternType::Parchment,
						richText.getDimensions().x + 20, textureHeight,
						game.getTextureManager(), game.getRenderer());

					const Int2 textureCenter(
						(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
						(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);

					auto thirdSubPanel = std::make_unique<TextSubPanel>(
						game, center, richText, toFourthSubPanel, std::move(texture),
						textureCenter);

					game.pushSubPanel(std::move(thirdSubPanel));
				};

				auto toSecondSubPanel = [textColor, toThirdSubPanel](Game &game)
				{
					game.popSubPanel();

					const Int2 center((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 98);

					const std::string text = [&game]()
					{
						const auto &exeData = game.getBinaryAssetLibrary().getExeData();
						std::string segment = exeData.charCreation.confirmedRace2;
						segment = String::replace(segment, '\r', '\n');

						const auto &charCreationState = game.getCharacterCreationState();
						const int raceIndex = charCreationState.getRaceIndex();

						// Get race description from TEMPLATE.DAT.
						const auto &templateDat = game.getTextAssetLibrary().getTemplateDat();
						constexpr std::array<int, 8> raceTemplateIDs =
						{
							1409, 1410, 1411, 1412, 1413, 1414, 1415, 1416
						};

						DebugAssertIndex(raceTemplateIDs, raceIndex);
						const auto &entry = templateDat.getEntry(raceTemplateIDs[raceIndex]);
						std::string raceDescription = entry.values.front();

						// Re-distribute newlines at 40 character limit.
						raceDescription = String::distributeNewlines(raceDescription, 40);

						// Append race description to text segment.
						segment += "\n" + raceDescription;

						return segment;
					}();

					const int lineSpacing = 1;

					const RichTextString richText(
						text,
						FontName::Arena,
						textColor,
						TextAlignment::Center,
						lineSpacing,
						game.getFontLibrary());

					const int textureHeight = std::max(richText.getDimensions().y + 14, 40);
					Texture texture = TextureUtils::generate(TextureUtils::PatternType::Parchment,
						richText.getDimensions().x + 20, textureHeight,
						game.getTextureManager(), game.getRenderer());

					const Int2 textureCenter(
						(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
						(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);

					auto secondSubPanel = std::make_unique<TextSubPanel>(
						game, center, richText, toThirdSubPanel, std::move(texture),
						textureCenter);

					game.pushSubPanel(std::move(secondSubPanel));
				};

				std::unique_ptr<Panel> firstSubPanel = [&game, &textColor, toSecondSubPanel]()
				{
					const Int2 center((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 98);

					const std::string text = [&game]()
					{
						const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
						const auto &exeData = binaryAssetLibrary.getExeData();
						std::string segment = exeData.charCreation.confirmedRace1;
						segment = String::replace(segment, '\r', '\n');

						const auto &charCreationState = game.getCharacterCreationState();
						const int raceIndex = charCreationState.getRaceIndex();

						const auto &charCreationProvinceNames = exeData.locations.charCreationProvinceNames;						
						DebugAssertIndex(charCreationProvinceNames, raceIndex);
						const std::string &provinceName = charCreationProvinceNames[raceIndex];

						const auto &pluralRaceNames = exeData.races.pluralNames;
						DebugAssertIndex(pluralRaceNames, raceIndex);
						const std::string &pluralRaceName = pluralRaceNames[raceIndex];

						const auto &charClassLibrary = game.getCharacterClassLibrary();
						const int charClassDefID = charCreationState.getClassDefID();
						const auto &charClassDef = charClassLibrary.getDefinition(charClassDefID);

						// Replace first %s with player class.
						size_t index = segment.find("%s");
						segment.replace(index, 2, charClassDef.getName());

						// Replace second %s with player name.
						index = segment.find("%s");
						segment.replace(index, 2, charCreationState.getName());

						// Replace third %s with province name.
						index = segment.find("%s");
						segment.replace(index, 2, provinceName);

						// Replace fourth %s with plural race name.
						index = segment.find("%s");
						segment.replace(index, 2, pluralRaceName);

						// If player is female, replace "his" with "her".
						if (!charCreationState.isMale())
						{
							index = segment.rfind("his");
							segment.replace(index, 3, "her");
						}

						return segment;
					}();

					const int lineSpacing = 1;

					const RichTextString richText(
						text,
						FontName::Arena,
						textColor,
						TextAlignment::Center,
						lineSpacing,
						game.getFontLibrary());

					const int textureHeight = std::max(richText.getDimensions().y, 40);
					Texture texture = TextureUtils::generate(TextureUtils::PatternType::Parchment,
						richText.getDimensions().x + 20, textureHeight,
						game.getTextureManager(), game.getRenderer());

					const Int2 textureCenter(
						(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
						(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);

					return std::make_unique<TextSubPanel>(game, center, richText,
						toSecondSubPanel, std::move(texture), textureCenter);
				}();

				game.pushSubPanel(std::move(firstSubPanel));
			};

			messageBoxYes.textureX = messageBoxTitle.textureX;
			messageBoxYes.textureY = messageBoxTitle.textureY +
				messageBoxTitle.texture.getHeight();

			MessageBoxSubPanel::Element messageBoxNo;
			messageBoxNo.textBox = [&game, &renderer, &textColor]()
			{
				const auto &fontLibrary = game.getFontLibrary();
				const RichTextString richText(
					"No",
					FontName::A,
					textColor,
					TextAlignment::Center,
					fontLibrary);

				const Int2 center(
					(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
					(ArenaRenderUtils::SCREEN_HEIGHT / 2) + 68);

				return std::make_unique<TextBox>(center, richText, fontLibrary, renderer);
			}();

			messageBoxNo.texture = [&textureManager, &renderer, &messageBoxYes]()
			{
				const int width = messageBoxYes.texture.getWidth();
				const int height = messageBoxYes.texture.getHeight();
				return TextureUtils::generate(TextureUtils::PatternType::Parchment, width, height,
					textureManager, renderer);
			}();

			messageBoxNo.function = [](Game &game)
			{
				game.popSubPanel();

				// Push the initial text sub-panel.
				std::unique_ptr<Panel> textSubPanel = ChooseRacePanel::getInitialSubPanel(game);
				game.pushSubPanel(std::move(textSubPanel));
			};

			messageBoxNo.textureX = messageBoxYes.textureX;
			messageBoxNo.textureY = messageBoxYes.textureY + messageBoxYes.texture.getHeight();

			auto cancelFunction = messageBoxNo.function;

			std::vector<MessageBoxSubPanel::Element> messageBoxElements;
			messageBoxElements.push_back(std::move(messageBoxYes));
			messageBoxElements.push_back(std::move(messageBoxNo));

			auto messageBox = std::make_unique<MessageBoxSubPanel>(
				game, std::move(messageBoxTitle), std::move(messageBoxElements),
				cancelFunction);

			game.pushSubPanel(std::move(messageBox));
		};

		return Button<Game&, int>(function);
	}();

	// @todo: maybe allocate std::unique_ptr<std::function> for unravelling the map?
	// When done, set to null and push initial parchment sub-panel?

	// Push the initial text sub-panel.
	std::unique_ptr<Panel> textSubPanel = ChooseRacePanel::getInitialSubPanel(game);
	game.pushSubPanel(std::move(textSubPanel));
}

std::unique_ptr<Panel> ChooseRacePanel::getInitialSubPanel(Game &game)
{
	const Int2 center((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 98);
	const Color color(48, 12, 12);

	const std::string text = [&game]()
	{
		const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
		const auto &exeData = binaryAssetLibrary.getExeData();
		std::string segment = exeData.charCreation.chooseRace;
		segment = String::replace(segment, '\r', '\n');

		const auto &charCreationState = game.getCharacterCreationState();
		const auto &charClassLibrary = game.getCharacterClassLibrary();
		const int charClassDefID = charCreationState.getClassDefID();
		const auto &charClassDef = charClassLibrary.getDefinition(charClassDefID);

		// Replace first "%s" with player name.
		size_t index = segment.find("%s");
		segment.replace(index, 2, charCreationState.getName());

		// Replace second "%s" with character class.
		index = segment.find("%s");
		segment.replace(index, 2, charClassDef.getName());

		return segment;
	}();

	const int lineSpacing = 1;

	const RichTextString richText(
		text,
		FontName::A,
		color,
		TextAlignment::Center,
		lineSpacing,
		game.getFontLibrary());

	Texture texture = TextureUtils::generate(TextureUtils::PatternType::Parchment, 240, 60,
		game.getTextureManager(), game.getRenderer());

	const Int2 textureCenter(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);

	auto function = [](Game &game)
	{
		game.popSubPanel();
	};

	return std::make_unique<TextSubPanel>(game, center, richText, function,
		std::move(texture), textureCenter);
}

int ChooseRacePanel::getProvinceMaskID(const Int2 &position) const
{
	const auto &worldMapMasks = this->getGame().getBinaryAssetLibrary().getWorldMapMasks();
	const int maskCount = static_cast<int>(worldMapMasks.size());
	for (int maskID = 0; maskID < maskCount; maskID++)
	{
		// Ignore the center province and the "Exit" button.
		const int exitButtonID = 9;
		if ((maskID == LocationUtils::CENTER_PROVINCE_ID) || (maskID == exitButtonID))
		{
			continue;
		}

		const WorldMapMask &mapMask = worldMapMasks.at(maskID);
		const Rect &maskRect = mapMask.getRect();

		if (maskRect.contains(position))
		{
			// See if the pixel is set in the bitmask.
			const bool success = mapMask.get(position.x, position.y);

			if (success)
			{
				// Return the mask's ID.
				return maskID;
			}
		}
	}

	// No province mask found at the given location.
	return ChooseRacePanel::NO_ID;
}

std::optional<Panel::CursorData> ChooseRacePanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void ChooseRacePanel::handleEvent(const SDL_Event &e)
{
	auto &game = this->getGame();
	const auto &inputManager = game.getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
	bool rightClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_RIGHT);

	// Interact with the map screen.
	if (escapePressed)
	{
		this->backToGenderButton.click(game);
	}
	else if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 originalPoint = game.getRenderer().nativeToOriginal(mousePosition);

		// Listen for clicks on the map, checking if the mouse is over a province mask.
		const int maskID = this->getProvinceMaskID(originalPoint);
		if (maskID != ChooseRacePanel::NO_ID)
		{
			// Choose the selected province.
			this->acceptButton.click(game, maskID);
		}
	}
}

void ChooseRacePanel::drawProvinceTooltip(int provinceID, Renderer &renderer)
{
	// Get the race name associated with the province.
	const auto &exeData = this->getGame().getBinaryAssetLibrary().getExeData();
	const std::string &raceName = exeData.races.pluralNames.at(provinceID);

	const Texture tooltip = Panel::createTooltip(
		"Land of the " + raceName, FontName::D, this->getGame().getFontLibrary(), renderer);

	const auto &inputManager = this->getGame().getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPosition = renderer.nativeToOriginal(mousePosition);
	const int mouseX = originalPosition.x;
	const int mouseY = originalPosition.y;
	const int x = ((mouseX + 8 + tooltip.getWidth()) < ArenaRenderUtils::SCREEN_WIDTH) ?
		(mouseX + 8) : (mouseX - tooltip.getWidth());
	const int y = ((mouseY + tooltip.getHeight()) < ArenaRenderUtils::SCREEN_HEIGHT) ?
		mouseY : (mouseY - tooltip.getHeight());

	renderer.drawOriginal(tooltip, x, y);
}

void ChooseRacePanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Draw background map.
	auto &textureManager = this->getGame().getTextureManager();
	const std::string &raceSelectFilename = ArenaTextureName::RaceSelect;
	const std::optional<PaletteID> raceSelectPaletteID = textureManager.tryGetPaletteID(raceSelectFilename.c_str());
	if (!raceSelectPaletteID.has_value())
	{
		DebugLogError("Couldn't get race select palette ID for \"" + raceSelectFilename + "\".");
		return;
	}

	const std::optional<TextureBuilderID> raceSelectTextureBuilderID =
		textureManager.tryGetTextureBuilderID(raceSelectFilename.c_str());
	if (!raceSelectTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get race select texture builder ID for \"" + raceSelectFilename + "\".");
		return;
	}

	renderer.drawOriginal(*raceSelectTextureBuilderID, *raceSelectPaletteID, textureManager);

	// Arena just covers up the "exit" text at the bottom right.
	const std::string &exitCoverFilename = ArenaTextureName::NoExit;
	const std::optional<TextureBuilderID> exitCoverTextureBuilderID =
		textureManager.tryGetTextureBuilderID(exitCoverFilename.c_str());
	if (!exitCoverTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get exit cover texture builder ID for \"" + exitCoverFilename + "\".");
		return;
	}

	const TextureBuilder &exitCoverTextureBuilder = textureManager.getTextureBuilderHandle(*exitCoverTextureBuilderID);
	const int exitCoverX = ArenaRenderUtils::SCREEN_WIDTH - exitCoverTextureBuilder.getWidth();
	const int exitCoverY = ArenaRenderUtils::SCREEN_HEIGHT - exitCoverTextureBuilder.getHeight();
	renderer.drawOriginal(*exitCoverTextureBuilderID, *raceSelectPaletteID, exitCoverX, exitCoverY, textureManager);
}

void ChooseRacePanel::renderSecondary(Renderer &renderer)
{
	const auto &inputManager = this->getGame().getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();

	// Draw hovered province tooltip.
	const Int2 originalPoint = this->getGame().getRenderer().nativeToOriginal(mousePosition);

	// Draw tooltip if the mouse is in a province.
	const int maskID = this->getProvinceMaskID(originalPoint);
	if (maskID != ChooseRacePanel::NO_ID)
	{
		this->drawProvinceTooltip(maskID, renderer);
	}
}
