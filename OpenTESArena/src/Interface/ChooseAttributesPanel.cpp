#include <array>

#include "SDL.h"

#include "ChooseAttributesPanel.h"
#include "ChooseRacePanel.h"
#include "CursorAlignment.h"
#include "GameWorldPanel.h"
#include "MessageBoxSubPanel.h"
#include "RichTextString.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "TextCinematicPanel.h"
#include "TextSubPanel.h"
#include "Texture.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/CIFFile.h"
#include "../Assets/ExeData.h"
#include "../Assets/MIFFile.h"
#include "../Entities/Player.h"
#include "../Game/CardinalDirection.h"
#include "../Game/GameData.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Game/PlayerInterface.h"
#include "../Math/Random.h"
#include "../Media/Color.h"
#include "../Media/FontLibrary.h"
#include "../Media/FontName.h"
#include "../Media/MusicUtils.h"
#include "../Media/PortraitFile.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../World/ClimateType.h"
#include "../World/LocationType.h"
#include "../World/LocationUtils.h"
#include "../World/SkyUtils.h"
#include "../World/WeatherType.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

ChooseAttributesPanel::ChooseAttributesPanel(Game &game)
	: Panel(game)
{
	auto &charCreationState = game.getCharacterCreationState();

	this->nameTextBox = [&game, &charCreationState]()
	{
		const int x = 10;
		const int y = 8;

		const std::string_view name = charCreationState.getName();

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			std::string(name),
			FontName::Arena,
			Color(199, 199, 199),
			TextAlignment::Left,
			fontLibrary);

		return std::make_unique<TextBox>(x, y, richText, fontLibrary, game.getRenderer());
	}();

	this->raceTextBox = [&game, &charCreationState]()
	{
		const int x = 10;
		const int y = 17;

		const auto &exeData = game.getBinaryAssetLibrary().getExeData();
		const auto &singularNames = exeData.races.singularNames;
		const int raceIndex = charCreationState.getRaceIndex();
		DebugAssertIndex(singularNames, raceIndex);
		const std::string &text = singularNames[raceIndex];

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			text,
			FontName::Arena,
			Color(199, 199, 199),
			TextAlignment::Left,
			fontLibrary);

		return std::make_unique<TextBox>(x, y, richText, fontLibrary, game.getRenderer());
	}();

	this->classTextBox = [&game, &charCreationState]()
	{
		const int x = 10;
		const int y = 26;

		const int charClassDefID = charCreationState.getClassDefID();
		const auto &charClassLibrary = game.getCharacterClassLibrary();
		const auto &charClassDef = charClassLibrary.getDefinition(charClassDefID);

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			charClassDef.getName(),
			FontName::Arena,
			Color(199, 199, 199),
			TextAlignment::Left,
			fontLibrary);

		return std::make_unique<TextBox>(x, y, richText, fontLibrary, game.getRenderer());
	}();

	this->backToRaceButton = []()
	{
		auto function = [](Game &game)
		{
			game.setPanel<ChooseRacePanel>(game);
		};

		return Button<Game&>(function);
	}();

	this->doneButton = [this]()
	{
		const Int2 center(25, ArenaRenderUtils::SCREEN_HEIGHT - 15);
		const int width = 21;
		const int height = 12;

		auto function = [this](Game &game)
		{
			// Generate the race selection message box.
			auto &textureManager = game.getTextureManager();
			auto &renderer = game.getRenderer();

			MessageBoxSubPanel::Title messageBoxTitle;
			messageBoxTitle.textBox = [&game, &renderer]()
			{
				const auto &exeData = game.getBinaryAssetLibrary().getExeData();
				const std::string &text = exeData.charCreation.chooseAttributes;

				const Color textColor(199, 199, 199);

				const auto &fontLibrary = game.getFontLibrary();
				const RichTextString richText(
					text,
					FontName::A,
					textColor,
					TextAlignment::Center,
					fontLibrary);

				const Int2 center(
					(ArenaRenderUtils::SCREEN_WIDTH / 2),
					(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 22);

				return std::make_unique<TextBox>(center, richText, fontLibrary, renderer);
			}();

			messageBoxTitle.texture = [&textureManager, &renderer, &messageBoxTitle]()
			{
				const int width = messageBoxTitle.textBox->getRect().getWidth() + 12;
				const int height = 24;
				return TextureUtils::generate(TextureUtils::PatternType::Dark,
					width, height, textureManager, renderer);
			}();

			messageBoxTitle.textureX = (ArenaRenderUtils::SCREEN_WIDTH / 2) -
				(messageBoxTitle.texture.getWidth() / 2) - 1;
			messageBoxTitle.textureY = (ArenaRenderUtils::SCREEN_HEIGHT / 2) -
				(messageBoxTitle.texture.getHeight() / 2) - 21;

			const Color buttonTextColor(190, 113, 0);

			MessageBoxSubPanel::Element messageBoxSave;
			messageBoxSave.textBox = [&game, &renderer, &buttonTextColor]()
			{
				const auto &exeData = game.getBinaryAssetLibrary().getExeData();
				std::string text = exeData.charCreation.chooseAttributesSave;

				// @todo: use the formatting characters in the string for color.
				// - For now, just delete them.
				text.erase(1, 2);

				const auto &fontLibrary = game.getFontLibrary();
				const RichTextString richText(
					text,
					FontName::A,
					buttonTextColor,
					TextAlignment::Center,
					fontLibrary);

				const Int2 center(
					(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
					(ArenaRenderUtils::SCREEN_HEIGHT / 2) + 2);

				return std::make_unique<TextBox>(center, richText, fontLibrary, renderer);
			}();

			messageBoxSave.texture = [&textureManager, &renderer, &messageBoxTitle]()
			{
				const int width = messageBoxTitle.texture.getWidth();
				const int height = messageBoxTitle.texture.getHeight();
				return TextureUtils::generate(TextureUtils::PatternType::Dark,
					width, height, textureManager, renderer);
			}();

			messageBoxSave.function = [this](Game &game)
			{
				// Confirming the chosen stats will bring up a text sub-panel, and
				// the next time the done button is clicked, it starts the game.
				game.popSubPanel();

				const Color color(199, 199, 199);

				const std::string text = [&game]()
				{
					const auto &exeData = game.getBinaryAssetLibrary().getExeData();
					std::string segment = exeData.charCreation.chooseAppearance;
					segment = String::replace(segment, '\r', '\n');

					return segment;
				}();

				const int lineSpacing = 1;

				const RichTextString richText(
					text,
					FontName::Arena,
					color,
					TextAlignment::Center,
					lineSpacing,
					game.getFontLibrary());

				Texture texture = TextureUtils::generate(TextureUtils::PatternType::Dark,
					richText.getDimensions().x + 10, richText.getDimensions().y + 12,
					game.getTextureManager(), game.getRenderer());

				const Int2 textureCenter(
					(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
					(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);

				// The done button is replaced after the player confirms their stats,
				// and it then leads to the main quest opening cinematic.
				auto newDoneFunction = [this](Game &game)
				{
					game.popSubPanel();

					auto gameDataFunction = [this](Game &game)
					{
						// Initialize 3D renderer.
						auto &renderer = game.getRenderer();
						const auto &options = game.getOptions();
						const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
						const bool fullGameWindow = options.getGraphics_ModernInterface();
						renderer.initializeWorldRendering(
							options.getGraphics_ResolutionScale(),
							fullGameWindow,
							options.getGraphics_RenderThreadsMode());

						std::unique_ptr<GameData> gameData = [this, &game, &binaryAssetLibrary]()
						{
							const auto &exeData = binaryAssetLibrary.getExeData();

							// Initialize player data (independent of the world).
							Player player = [this, &game, &exeData]()
							{
								const CoordDouble3 dummyPosition(ChunkInt2::Zero, VoxelDouble3::Zero);
								const Double3 direction(
									CardinalDirection::North.x,
									0.0,
									CardinalDirection::North.y);
								const Double3 velocity = Double3::Zero;

								const auto &charCreationState = game.getCharacterCreationState();
								const std::string_view name = charCreationState.getName();
								const bool male = charCreationState.isMale();
								const int raceIndex = charCreationState.getRaceIndex();

								const auto &charClassLibrary = game.getCharacterClassLibrary();
								const int charClassDefID = charCreationState.getClassDefID();
								const auto &charClassDef = charClassLibrary.getDefinition(charClassDefID);

								const int portraitIndex = charCreationState.getPortraitIndex();

								const int allowedWeaponCount = charClassDef.getAllowedWeaponCount();
								const int weaponID = charClassDef.getAllowedWeapon(
									game.getRandom().next(allowedWeaponCount));

								return Player(std::string(name), male, raceIndex, charClassDefID,
									portraitIndex, dummyPosition, direction, velocity,
									Player::DEFAULT_WALK_SPEED, Player::DEFAULT_RUN_SPEED, weaponID,
									exeData);
							}();

							return std::make_unique<GameData>(std::move(player), binaryAssetLibrary);
						}();

						// Find starting dungeon location definition.
						const int provinceIndex = LocationUtils::CENTER_PROVINCE_ID;
						const WorldMapDefinition &worldMapDef = gameData->getWorldMapDefinition();
						const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceIndex);

						const LocationDefinition *locationDefPtr = nullptr;
						for (int i = 0; i < provinceDef.getLocationCount(); i++)
						{
							const LocationDefinition &locationDef = provinceDef.getLocationDef(i);
							if (locationDef.getType() == LocationDefinition::Type::MainQuestDungeon)
							{
								const LocationDefinition::MainQuestDungeonDefinition &mainQuestDungeonDef =
									locationDef.getMainQuestDungeonDefinition();

								if (mainQuestDungeonDef.type == LocationDefinition::MainQuestDungeonDefinition::Type::Start)
								{
									locationDefPtr = &locationDef;
									break;
								}
							}
						}

						DebugAssertMsg(locationDefPtr != nullptr, "Couldn't find start dungeon location definition.");

						// Load starting dungeon.
						const LocationDefinition::MainQuestDungeonDefinition &mainQuestDungeonDef =
							locationDefPtr->getMainQuestDungeonDefinition();
						const std::string mifName = mainQuestDungeonDef.mapFilename;

						MIFFile mif;
						if (!mif.init(mifName.c_str()))
						{
							DebugCrash("Could not init .MIF file \"" + mifName + "\".");
						}

						if (!gameData->loadInterior(*locationDefPtr, provinceDef, ArenaTypes::InteriorType::Dungeon,
							mif, game.getEntityDefinitionLibrary(), game.getCharacterClassLibrary(),
							game.getBinaryAssetLibrary(), game.getRandom(), game.getTextureManager(),
							renderer))
						{
							DebugCrash("Couldn't load interior \"" + locationDefPtr->getName() + "\".");
						}

						// Set the game data before constructing the game world panel.
						game.setGameData(std::move(gameData));
					};

					auto cinematicFunction = [gameDataFunction](Game &game)
					{
						gameDataFunction(game);

						auto gameFunction = [](Game &game)
						{
							// Create the function that will be called when the player leaves
							// the starting dungeon.
							auto onLevelUpVoxelEnter = [](Game &game)
							{
								// Teleport the player to a random location based on their race.
								auto &gameData = game.getGameData();
								auto &player = gameData.getPlayer();
								player.setVelocityToZero();

								const int localCityID = game.getRandom().next(32);
								const int provinceID = gameData.getPlayer().getRaceID();

								const WorldMapDefinition &worldMapDef = gameData.getWorldMapDefinition();
								const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceID);
								const LocationDefinition &locationDef = provinceDef.getLocationDef(localCityID);

								// Random weather for now.
								// - @todo: make it depend on the location (no need to prevent
								//   deserts from having snow since the climates are still hardcoded).
								const WeatherType weatherType = [&game]()
								{
									constexpr std::array<WeatherType, 8> Weathers =
									{
										WeatherType::Clear,
										WeatherType::Overcast,
										WeatherType::Rain,
										WeatherType::Snow,
										WeatherType::SnowOvercast,
										WeatherType::Rain2,
										WeatherType::Overcast2,
										WeatherType::SnowOvercast2
									};

									const int index = game.getRandom().next(static_cast<int>(Weathers.size()));
									DebugAssertIndex(Weathers, index);
									return Weathers[index];
								}();

								const int starCount = SkyUtils::getStarCountFromDensity(
									game.getOptions().getMisc_StarDensity());

								auto &renderer = game.getRenderer();
								if (!gameData.loadCity(locationDef, provinceDef, weatherType, starCount,
									game.getEntityDefinitionLibrary(), game.getCharacterClassLibrary(),
									game.getBinaryAssetLibrary(), game.getTextAssetLibrary(), game.getRandom(),
									game.getTextureManager(), renderer))
								{
									DebugCrash("Couldn't load city \"" + locationDef.getName() + "\".");
								}

								// Set music based on weather and time.
								const MusicDefinition *musicDef = [&game, &gameData, weatherType]()
								{
									const MusicLibrary &musicLibrary = game.getMusicLibrary();
									if (!gameData.nightMusicIsActive())
									{
										return musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Weather,
											game.getRandom(), [weatherType](const MusicDefinition &def)
										{
											DebugAssert(def.getType() == MusicDefinition::Type::Weather);
											const auto &weatherMusicDef = def.getWeatherMusicDefinition();
											return weatherMusicDef.type == weatherType;
										});
									}
									else
									{
										return musicLibrary.getRandomMusicDefinition(
											MusicDefinition::Type::Night, game.getRandom());
									}
								}();

								if (musicDef == nullptr)
								{
									DebugLogWarning("Missing exterior music.");
								}

								AudioManager &audioManager = game.getAudioManager();
								audioManager.setMusic(musicDef);
							};

							// Set the *LEVELUP voxel enter event.
							auto &gameData = game.getGameData();
							gameData.getOnLevelUpVoxelEnter() = std::move(onLevelUpVoxelEnter);

							// Initialize the game world panel.
							auto gameWorldPanel = std::make_unique<GameWorldPanel>(game);
							game.setPanel(std::move(gameWorldPanel));

							// Choose random dungeon music.
							const MusicLibrary &musicLibrary = game.getMusicLibrary();
							const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinition(
								MusicDefinition::Type::Dungeon, game.getRandom());

							if (musicDef == nullptr)
							{
								DebugLogWarning("Missing dungeon music.");
							}

							AudioManager &audioManager = game.getAudioManager();
							audioManager.setMusic(musicDef);
						};

						const auto &cinematicLibrary = game.getCinematicLibrary();
						int textCinematicDefIndex;
						const bool success = cinematicLibrary.findTextDefinitionIndexIf(
							[](const TextCinematicDefinition &def)
						{
							if (def.getType() == TextCinematicDefinition::Type::MainQuest)
							{
								const auto &mainQuestCinematicDef = def.getMainQuestDefinition();
								const bool isMainQuestStartCinematic = mainQuestCinematicDef.progress == 0;
								if (isMainQuestStartCinematic)
								{
									return true;
								}
							}

							return false;
						}, &textCinematicDefIndex);

						if (!success)
						{
							DebugCrash("Couldn't find main quest start text cinematic definition.");
						}

						game.setCharacterCreationState(nullptr);
						game.setPanel<TextCinematicPanel>(game, textCinematicDefIndex, 0.171, gameFunction);

						// Play dream music.
						const MusicLibrary &musicLibrary = game.getMusicLibrary();
						const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinitionIf(
							MusicDefinition::Type::Cinematic, game.getRandom(), [](const MusicDefinition &def)
						{
							DebugAssert(def.getType() == MusicDefinition::Type::Cinematic);
							const auto &cinematicMusicDef = def.getCinematicMusicDefinition();
							return cinematicMusicDef.type == MusicDefinition::CinematicMusicDefinition::Type::DreamGood;
						});

						if (musicDef == nullptr)
						{
							DebugLogWarning("Missing vision music.");
						}

						AudioManager &audioManager = game.getAudioManager();
						audioManager.setMusic(musicDef);
					};

					const Int2 center(25, ArenaRenderUtils::SCREEN_HEIGHT - 15);
					const int width = 21;
					const int height = 12;

					this->doneButton = Button<Game&>(center, width, height, cinematicFunction);
					this->canChangePortrait = true;
				};

				auto appearanceSubPanel = std::make_unique<TextSubPanel>(
					game, textureCenter, richText, newDoneFunction,
					std::move(texture), textureCenter);

				game.pushSubPanel(std::move(appearanceSubPanel));
			};

			messageBoxSave.textureX = messageBoxTitle.textureX;
			messageBoxSave.textureY = messageBoxTitle.textureY +
				messageBoxTitle.texture.getHeight();

			MessageBoxSubPanel::Element messageBoxReroll;
			messageBoxReroll.textBox = [&game, &renderer, &buttonTextColor]()
			{
				const auto &exeData = game.getBinaryAssetLibrary().getExeData();
				std::string text = exeData.charCreation.chooseAttributesReroll;

				// @todo: use the formatting characters in the string for color.
				// - For now, just delete them.
				text.erase(1, 2);

				const auto &fontLibrary = game.getFontLibrary();
				const RichTextString richText(
					text,
					FontName::A,
					buttonTextColor,
					TextAlignment::Center,
					fontLibrary);

				const Int2 center(
					(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
					(ArenaRenderUtils::SCREEN_HEIGHT / 2) + 26);

				return std::make_unique<TextBox>(center, richText, fontLibrary, renderer);
			}();

			messageBoxReroll.texture = [&textureManager, &renderer, &messageBoxSave]()
			{
				const int width = messageBoxSave.texture.getWidth();
				const int height = messageBoxSave.texture.getHeight();
				return TextureUtils::generate(TextureUtils::PatternType::Dark,
					width, height, textureManager, renderer);
			}();

			messageBoxReroll.function = [](Game &game)
			{
				// @todo: reroll attributes.
				game.popSubPanel();
			};

			messageBoxReroll.textureX = messageBoxSave.textureX;
			messageBoxReroll.textureY = messageBoxSave.textureY +
				messageBoxSave.texture.getHeight();

			auto cancelFunction = messageBoxReroll.function;

			// Push message box sub panel.
			std::vector<MessageBoxSubPanel::Element> messageBoxElements;
			messageBoxElements.push_back(std::move(messageBoxSave));
			messageBoxElements.push_back(std::move(messageBoxReroll));

			auto messageBox = std::make_unique<MessageBoxSubPanel>(
				game, std::move(messageBoxTitle), std::move(messageBoxElements),
				cancelFunction);

			game.pushSubPanel(std::move(messageBox));
		};

		return Button<Game&>(center, width, height, function);
	}();

	this->portraitButton = []()
	{
		const Int2 center(ArenaRenderUtils::SCREEN_WIDTH - 72, 25);
		const int width = 60;
		const int height = 42;
		auto function = [](ChooseAttributesPanel &panel, bool increment)
		{
			const int minID = 0; // @todo: de-hardcode so it relies on portraits list
			const int maxID = 9;

			auto &charCreationState = panel.getGame().getCharacterCreationState();
			const int oldPortraitIndex = charCreationState.getPortraitIndex();			
			const int newPortraitIndex = increment ?
				((oldPortraitIndex == maxID) ? minID : (oldPortraitIndex + 1)) :
				((oldPortraitIndex == minID) ? maxID : (oldPortraitIndex - 1));

			charCreationState.setPortraitIndex(newPortraitIndex);
		};

		return Button<ChooseAttributesPanel&, bool>(center, width, height, function);
	}();

	// Get pixel offsets for each head.
	const std::string headsFilename = PortraitFile::getHeads(
		charCreationState.isMale(), charCreationState.getRaceIndex(), false);
	CIFFile cifFile;
	if (!cifFile.init(headsFilename.c_str()))
	{
		DebugCrash("Could not init .CIF file \"" + headsFilename + "\".");
	}

	for (int i = 0; i < cifFile.getImageCount(); i++)
	{
		this->headOffsets.push_back(Int2(cifFile.getXOffset(i), cifFile.getYOffset(i)));
	}

	charCreationState.setPortraitIndex(0);
	this->canChangePortrait = false;

	// Push the initial text pop-up onto the sub-panel stack.
	std::unique_ptr<Panel> textSubPanel = [&game]()
	{
		const Int2 center(
			(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
			(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 2);
		const Color color(199, 199, 199);

		const std::string text = [&game]()
		{
			const auto &exeData = game.getBinaryAssetLibrary().getExeData();
			std::string segment = exeData.charCreation.distributeClassPoints;
			segment = String::replace(segment, '\r', '\n');
			return segment;
		}();

		const int lineSpacing = 1;
		
		const RichTextString richText(
			text,
			FontName::Arena,
			color,
			TextAlignment::Center,
			lineSpacing,
			game.getFontLibrary());

		Texture texture = TextureUtils::generate(TextureUtils::PatternType::Dark, 183, 42,
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
	}();

	game.pushSubPanel(std::move(textSubPanel));
}

std::optional<Panel::CursorData> ChooseAttributesPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void ChooseAttributesPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);

	if (escapePressed)
	{
		this->backToRaceButton.click(this->getGame());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
	bool rightClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_RIGHT);
		
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 mouseOriginalPoint = this->getGame().getRenderer()
		.nativeToOriginal(mousePosition);

	if (leftClick)
	{
		if (this->doneButton.contains(mouseOriginalPoint))
		{
			this->doneButton.click(this->getGame());
		}
		else if (this->portraitButton.contains(mouseOriginalPoint) &&
			this->canChangePortrait)
		{
			// Pass 'true' to increment the portrait ID.
			this->portraitButton.click(*this, true);
		}
	}

	if (rightClick)
	{
		if (this->portraitButton.contains(mouseOriginalPoint) &&
			this->canChangePortrait)
		{
			// Pass 'false' to decrement the portrait ID.
			this->portraitButton.click(*this, false);
		}
	}	
}

void ChooseAttributesPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	auto &game = this->getGame();
	const auto &charCreationState = game.getCharacterCreationState();
	const bool male = charCreationState.isMale();
	const int raceIndex = charCreationState.getRaceIndex();
	const int portraitIndex = charCreationState.getPortraitIndex();
	const auto &charClassDef = [&game, &charCreationState]() -> const CharacterClassDefinition&
	{
		const auto &charClassLibrary = game.getCharacterClassLibrary();
		const int charClassDefID = charCreationState.getClassDefID();
		return charClassLibrary.getDefinition(charClassDefID);
	}();

	auto &textureManager = game.getTextureManager();
	const std::string &charSheetPaletteFilename = ArenaPaletteName::CharSheet;
	const std::optional<PaletteID> charSheetPaletteID = textureManager.tryGetPaletteID(charSheetPaletteFilename.c_str());
	if (!charSheetPaletteID.has_value())
	{
		DebugLogError("Couldn't get character sheet palette ID \"" + charSheetPaletteFilename + "\".");
		return;
	}

	// Get the filenames for the portrait and clothes.
	const std::string &headsFilename = PortraitFile::getHeads(male, raceIndex, false);
	const std::string &bodyFilename = PortraitFile::getBody(male, raceIndex);
	const std::string &shirtFilename = PortraitFile::getShirt(male, charClassDef.canCastMagic());
	const std::string &pantsFilename = PortraitFile::getPants(male);

	// Get pixel offsets for each clothes texture.
	const Int2 shirtOffset = PortraitFile::getShirtOffset(male, charClassDef.canCastMagic());
	const Int2 pantsOffset = PortraitFile::getPantsOffset(male);

	// Draw the current portrait and clothes.
	const Int2 &headOffset = this->headOffsets.at(portraitIndex);
	const std::optional<TextureBuilderIdGroup> headTextureBuilderIDs =
		textureManager.tryGetTextureBuilderIDs(headsFilename.c_str());
	const std::optional<TextureBuilderID> bodyTextureBuilderID =
		textureManager.tryGetTextureBuilderID(bodyFilename.c_str());
	const std::optional<TextureBuilderID> shirtTextureBuilderID =
		textureManager.tryGetTextureBuilderID(shirtFilename.c_str());
	const std::optional<TextureBuilderID> pantsTextureBuilderID =
		textureManager.tryGetTextureBuilderID(pantsFilename.c_str());
	DebugAssert(headTextureBuilderIDs.has_value());
	DebugAssert(bodyTextureBuilderID.has_value());
	DebugAssert(shirtTextureBuilderID.has_value());
	DebugAssert(pantsTextureBuilderID.has_value());
	const TextureBuilderID headTextureBuilderID = headTextureBuilderIDs->getID(portraitIndex);

	const int bodyTextureX = [&textureManager, &bodyTextureBuilderID]()
	{
		const TextureBuilder &bodyTexture = textureManager.getTextureBuilderHandle(*bodyTextureBuilderID);
		return ArenaRenderUtils::SCREEN_WIDTH - bodyTexture.getWidth();
	}();

	renderer.drawOriginal(*bodyTextureBuilderID, *charSheetPaletteID, bodyTextureX, 0, textureManager);
	renderer.drawOriginal(*pantsTextureBuilderID, *charSheetPaletteID, pantsOffset.x, pantsOffset.y, textureManager);
	renderer.drawOriginal(headTextureBuilderID, *charSheetPaletteID, headOffset.x, headOffset.y, textureManager);
	renderer.drawOriginal(*shirtTextureBuilderID, *charSheetPaletteID, shirtOffset.x, shirtOffset.y, textureManager);

	// Draw attributes texture.
	const std::string &charStatsBgFilename = ArenaTextureName::CharacterStats;
	const std::optional<TextureBuilderID> attributesBgTextureBuilderID =
		textureManager.tryGetTextureBuilderID(charStatsBgFilename.c_str());
	DebugAssert(attributesBgTextureBuilderID.has_value());
	renderer.drawOriginal(*attributesBgTextureBuilderID, *charSheetPaletteID, textureManager);

	// Draw text boxes: player name, race, class.
	renderer.drawOriginal(this->nameTextBox->getTexture(),
		this->nameTextBox->getX(), this->nameTextBox->getY());
	renderer.drawOriginal(this->raceTextBox->getTexture(),
		this->raceTextBox->getX(), this->raceTextBox->getY());
	renderer.drawOriginal(this->classTextBox->getTexture(),
		this->classTextBox->getX(), this->classTextBox->getY());
}
