#include <array>
#include <cassert>

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
#include "../Assets/CIFFile.h"
#include "../Assets/ExeData.h"
#include "../Assets/MIFFile.h"
#include "../Assets/MiscAssets.h"
#include "../Entities/Player.h"
#include "../Game/GameData.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Game/PlayerInterface.h"
#include "../Math/Random.h"
#include "../Media/Color.h"
#include "../Media/FontManager.h"
#include "../Media/FontName.h"
#include "../Media/MusicFile.h"
#include "../Media/MusicName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/PortraitFile.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Media/TextureSequenceName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Texture.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"
#include "../World/ClimateType.h"
#include "../World/LocationType.h"
#include "../World/WeatherType.h"

ChooseAttributesPanel::ChooseAttributesPanel(Game &game,
	const CharacterClass &charClass, const std::string &name, 
	GenderName gender, int raceID)
	: Panel(game), charClass(charClass), gender(gender), name(name)
{
	this->nameTextBox = [&game, &name]()
	{
		const int x = 10;
		const int y = 8;

		const RichTextString richText(
			name,
			FontName::Arena,
			Color(199, 199, 199),
			TextAlignment::Left,
			game.getFontManager());

		return std::make_unique<TextBox>(x, y, richText, game.getRenderer());
	}();

	this->raceTextBox = [&game, raceID]()
	{
		const int x = 10;
		const int y = 17;

		const auto &exeData = game.getMiscAssets().getExeData();
		const std::string &text = exeData.races.singularNames.at(raceID);

		const RichTextString richText(
			text,
			FontName::Arena,
			Color(199, 199, 199),
			TextAlignment::Left,
			game.getFontManager());

		return std::make_unique<TextBox>(x, y, richText, game.getRenderer());
	}();

	this->classTextBox = [&game, &charClass]()
	{
		const int x = 10;
		const int y = 26;

		const RichTextString richText(
			charClass.getName(),
			FontName::Arena,
			Color(199, 199, 199),
			TextAlignment::Left,
			game.getFontManager());

		return std::make_unique<TextBox>(x, y, richText, game.getRenderer());
	}();

	this->backToRaceButton = [&charClass, &name, gender]()
	{
		auto function = [charClass, name, gender](Game &game)
		{
			game.setPanel<ChooseRacePanel>(game, charClass, name, gender);
		};
		return Button<Game&>(function);
	}();

	this->doneButton = [this, &charClass, &name, gender, raceID]()
	{
		const Int2 center(25, Renderer::ORIGINAL_HEIGHT - 15);
		const int width = 21;
		const int height = 12;

		auto function = [this, charClass, name, gender, raceID](Game &game)
		{
			// Generate the race selection message box.
			auto &textureManager = game.getTextureManager();
			auto &renderer = game.getRenderer();

			MessageBoxSubPanel::Title messageBoxTitle;
			messageBoxTitle.textBox = [&game, raceID, &renderer]()
			{
				const auto &exeData = game.getMiscAssets().getExeData();
				const std::string &text = exeData.charCreation.chooseAttributes;

				const Color textColor(199, 199, 199);

				const RichTextString richText(
					text,
					FontName::A,
					textColor,
					TextAlignment::Center,
					game.getFontManager());

				const Int2 center(
					(Renderer::ORIGINAL_WIDTH / 2),
					(Renderer::ORIGINAL_HEIGHT / 2) - 22);

				return std::make_unique<TextBox>(center, richText, renderer);
			}();

			messageBoxTitle.texture = [&textureManager, &renderer, &messageBoxTitle]()
			{
				const int width = messageBoxTitle.textBox->getRect().getWidth() + 12;
				const int height = 24;
				return Texture(Texture::generate(Texture::PatternType::Dark,
					width, height, textureManager, renderer));
			}();

			messageBoxTitle.textureX = (Renderer::ORIGINAL_WIDTH / 2) -
				(messageBoxTitle.texture.getWidth() / 2) - 1;
			messageBoxTitle.textureY = (Renderer::ORIGINAL_HEIGHT / 2) -
				(messageBoxTitle.texture.getHeight() / 2) - 21;

			const Color buttonTextColor(190, 113, 0);

			MessageBoxSubPanel::Element messageBoxSave;
			messageBoxSave.textBox = [&game, &renderer, &buttonTextColor]()
			{
				const auto &exeData = game.getMiscAssets().getExeData();
				std::string text = exeData.charCreation.chooseAttributesSave;

				// To do: use the formatting characters in the string for color.
				// - For now, just delete them.
				text.erase(1, 2);

				const RichTextString richText(
					text,
					FontName::A,
					buttonTextColor,
					TextAlignment::Center,
					game.getFontManager());

				const Int2 center(
					(Renderer::ORIGINAL_WIDTH / 2) - 1,
					(Renderer::ORIGINAL_HEIGHT / 2) + 2);

				return std::make_unique<TextBox>(center, richText, renderer);
			}();

			messageBoxSave.texture = [&textureManager, &renderer, &messageBoxTitle]()
			{
				const int width = messageBoxTitle.texture.getWidth();
				const int height = messageBoxTitle.texture.getHeight();
				return Texture(Texture::generate(Texture::PatternType::Dark,
					width, height, textureManager, renderer));
			}();

			messageBoxSave.function = [this, charClass, name, gender, raceID](Game &game)
			{
				// Confirming the chosen stats will bring up a text sub-panel, and
				// the next time the done button is clicked, it starts the game.
				game.popSubPanel();

				const Color color(199, 199, 199);

				const std::string text = [&game]()
				{
					const auto &exeData = game.getMiscAssets().getExeData();
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
					game.getFontManager());

				Texture texture(Texture::generate(Texture::PatternType::Dark,
					richText.getDimensions().x + 10, richText.getDimensions().y + 12,
					game.getTextureManager(), game.getRenderer()));

				const Int2 textureCenter(
					(Renderer::ORIGINAL_WIDTH / 2) - 1,
					(Renderer::ORIGINAL_HEIGHT / 2) - 1);

				// The done button is replaced after the player confirms their stats,
				// and it then leads to the main quest opening cinematic.
				auto newDoneFunction = [this, charClass, name, gender, raceID](Game &game)
				{
					game.popSubPanel();

					auto gameDataFunction = [this, charClass, name, gender, raceID](Game &game)
					{
						// Initialize 3D renderer.
						auto &renderer = game.getRenderer();
						const auto &miscAssets = game.getMiscAssets();
						const bool fullGameWindow = game.getOptions().getModernInterface();
						renderer.initializeWorldRendering(
							game.getOptions().getResolutionScale(), fullGameWindow);

						std::unique_ptr<GameData> gameData = [this, &name, gender, raceID,
							&charClass, &miscAssets]()
						{
							const auto &exeData = miscAssets.getExeData();

							// Initialize player data (independent of the world).
							Player player = [this, &name, gender, raceID, &charClass, &exeData]()
							{
								const Double3 dummyPosition = Double3::Zero;
								const Double3 direction = Double3::UnitX;
								const Double3 velocity = Double3::Zero;

								Random random;
								const auto &allowedWeapons = charClass.getAllowedWeapons();
								const int weaponID = allowedWeapons.at(
									random.next(static_cast<int>(allowedWeapons.size())));

								return Player(name, gender, raceID, charClass, this->portraitID,
									dummyPosition, direction, velocity, Player::DEFAULT_WALK_SPEED,
									Player::DEFAULT_RUN_SPEED, weaponID, exeData);
							}();

							return std::make_unique<GameData>(std::move(player), miscAssets);
						}();

						// Set palette (important for texture loading).
						auto &textureManager = game.getTextureManager();
						textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

						// Load starting dungeon.
						const MIFFile mif("START.MIF");
						const int provinceID = 8;
						const Location location = Location::makeSpecialCase(
							Location::SpecialCaseType::StartDungeon, provinceID);
						gameData->loadInterior(mif, location, textureManager, renderer);

						// Set the game data before constructing the game world panel.
						game.setGameData(std::move(gameData));
					};

					auto cinematicFunction = [gameDataFunction](Game &game)
					{
						gameDataFunction(game);

						// The original game wraps text onto the next screen if the player's name
						// is too long. For example, it causes "listen to me" to go down one line
						// and "Imperial Battle" to go onto the next screen, which then pushes the
						// text for every subsequent screen forward by a little bit.

						// Read cinematic text from TEMPLATE.DAT.
						std::string cinematicText = game.getMiscAssets().getTemplateDatText("#1400");
						cinematicText.append("\n");

						// Replace all instances of %pcf with the player's first name.
						const std::string playerName =
							game.getGameData().getPlayer().getFirstName();
						cinematicText = String::replace(cinematicText, "%pcf", playerName);

						// Some more formatting should be done in the future so the text wraps
						// nicer. That is, replace all new lines with spaces and redistribute new
						// lines given some max line length value.

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

								Random random;
								const int localCityID = random.next(32);
								const int provinceID = gameData.getPlayer().getRaceID();

								// Random weather for now.
								// - To do: make it depend on the location (no need to prevent
								//   deserts from having snow since the climates are still hardcoded).
								const WeatherType weatherType = [&random]()
								{
									const std::array<WeatherType, 8> Weathers =
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

									return Weathers.at(random.next(
										static_cast<int>(Weathers.size())));
								}();

								auto &renderer = game.getRenderer();
								gameData.loadCity(localCityID, provinceID, weatherType,
									game.getMiscAssets(), game.getTextureManager(), renderer);

								// Set music based on weather and time.
								const auto &clock = gameData.getClock();
								const MusicName musicName = clock.nightMusicIsActive() ?
									MusicName::Night : MusicFile::fromWeather(weatherType);
								game.setMusic(musicName);
							};

							// Set the *LEVELUP voxel enter event.
							auto &gameData = game.getGameData();
							gameData.getOnLevelUpVoxelEnter() = std::move(onLevelUpVoxelEnter);

							// Initialize the game world panel.
							auto gameWorldPanel = std::make_unique<GameWorldPanel>(game);
							game.setPanel(std::move(gameWorldPanel));

							// Choose random dungeon music.
							Random random;
							const MusicName musicName = GameData::getDungeonMusicName(random);
							game.setMusic(musicName);
						};

						game.setPanel<TextCinematicPanel>(
							game,
							TextureFile::fromName(TextureSequenceName::Silmane),
							cinematicText,
							0.171,
							gameFunction);
						game.setMusic(MusicName::Vision);
					};

					const Int2 center(25, Renderer::ORIGINAL_HEIGHT - 15);
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
				const auto &exeData = game.getMiscAssets().getExeData();
				std::string text = exeData.charCreation.chooseAttributesReroll;

				// To do: use the formatting characters in the string for color.
				// - For now, just delete them.
				text.erase(1, 2);

				const RichTextString richText(
					text,
					FontName::A,
					buttonTextColor,
					TextAlignment::Center,
					game.getFontManager());

				const Int2 center(
					(Renderer::ORIGINAL_WIDTH / 2) - 1,
					(Renderer::ORIGINAL_HEIGHT / 2) + 26);

				return std::make_unique<TextBox>(center, richText, renderer);
			}();

			messageBoxReroll.texture = [&textureManager, &renderer, &messageBoxSave]()
			{
				const int width = messageBoxSave.texture.getWidth();
				const int height = messageBoxSave.texture.getHeight();
				return Texture(Texture::generate(Texture::PatternType::Dark,
					width, height, textureManager, renderer));
			}();

			messageBoxReroll.function = [](Game &game)
			{
				// To do: reroll attributes.
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
		const Int2 center(Renderer::ORIGINAL_WIDTH - 72, 25);
		const int width = 60;
		const int height = 42;
		auto function = [](ChooseAttributesPanel &panel, bool increment)
		{
			const int minID = 0;
			const int maxID = 9;

			if (increment)
			{
				panel.portraitID = (panel.portraitID == maxID) ?
					minID : (panel.portraitID + 1);
			}
			else
			{
				panel.portraitID = (panel.portraitID == minID) ?
					maxID : (panel.portraitID - 1);
			}
		};
		return Button<ChooseAttributesPanel&, bool>(center, width, height, function);
	}();

	// Get pixel offsets for each head.
	const std::string &headsFilename = PortraitFile::getHeads(gender, raceID, false);
	CIFFile cifFile(headsFilename, Palette());

	for (int i = 0; i < cifFile.getImageCount(); i++)
	{
		this->headOffsets.push_back(Int2(cifFile.getXOffset(i), cifFile.getYOffset(i)));
	}

	this->raceID = raceID;
	this->portraitID = 0;
	this->canChangePortrait = false;

	// Push the initial text pop-up onto the sub-panel stack.
	std::unique_ptr<Panel> textSubPanel = [&game]()
	{
		const Int2 center(
			(Renderer::ORIGINAL_WIDTH / 2) - 1,
			(Renderer::ORIGINAL_HEIGHT / 2) - 2);
		const Color color(199, 199, 199);

		const std::string text = [&game]()
		{
			const auto &exeData = game.getMiscAssets().getExeData();
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
			game.getFontManager());

		Texture texture(Texture::generate(
			Texture::PatternType::Dark, 183, 42, game.getTextureManager(),
			game.getRenderer()));

		const Int2 textureCenter(
			(Renderer::ORIGINAL_WIDTH / 2) - 1,
			(Renderer::ORIGINAL_HEIGHT / 2) - 1);

		auto function = [](Game &game)
		{
			game.popSubPanel();
		};

		return std::make_unique<TextSubPanel>(game, center, richText, function,
			std::move(texture), textureCenter);
	}();

	game.pushSubPanel(std::move(textSubPanel));
}

std::pair<SDL_Texture*, CursorAlignment> ChooseAttributesPanel::getCurrentCursor() const
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &textureManager = game.getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor),
		PaletteFile::fromName(PaletteName::Default), renderer);
	return std::make_pair(texture.get(), CursorAlignment::TopLeft);
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

	// Set palette.
	auto &textureManager = this->getGame().getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::CharSheet));

	// Get the filenames for the portrait and clothes.
	const std::string &headsFilename = PortraitFile::getHeads(
		this->gender, this->raceID, false);
	const std::string &bodyFilename = PortraitFile::getBody(
		this->gender, this->raceID);
	const std::string &shirtFilename = PortraitFile::getShirt(
		this->gender, this->charClass.canCastMagic());
	const std::string &pantsFilename = PortraitFile::getPants(this->gender);

	// Get pixel offsets for each clothes texture.
	const Int2 &shirtOffset = PortraitFile::getShirtOffset(
		this->gender, this->charClass.canCastMagic());
	const Int2 &pantsOffset = PortraitFile::getPantsOffset(this->gender);

	// Draw the current portrait and clothes.
	const Int2 &headOffset = this->headOffsets.at(this->portraitID);
	const auto &head = textureManager.getTextures(headsFilename,
		PaletteFile::fromName(PaletteName::CharSheet), renderer).at(this->portraitID);
	const auto &body = textureManager.getTexture(bodyFilename, renderer);
	const auto &shirt = textureManager.getTexture(shirtFilename, renderer);
	const auto &pants = textureManager.getTexture(pantsFilename, renderer);
	renderer.drawOriginal(body.get(), Renderer::ORIGINAL_WIDTH - body.getWidth(), 0);
	renderer.drawOriginal(pants.get(), pantsOffset.x, pantsOffset.y);
	renderer.drawOriginal(head.get(), headOffset.x, headOffset.y);
	renderer.drawOriginal(shirt.get(), shirtOffset.x, shirtOffset.y);

	// Draw attributes texture.
	const auto &attributesBackground = textureManager.getTexture(
		TextureFile::fromName(TextureName::CharacterStats), renderer);
	renderer.drawOriginal(attributesBackground.get());

	// Draw text boxes: player name, race, class.
	renderer.drawOriginal(this->nameTextBox->getTexture(),
		this->nameTextBox->getX(), this->nameTextBox->getY());
	renderer.drawOriginal(this->raceTextBox->getTexture(),
		this->raceTextBox->getX(), this->raceTextBox->getY());
	renderer.drawOriginal(this->classTextBox->getTexture(),
		this->classTextBox->getX(), this->classTextBox->getY());
}
