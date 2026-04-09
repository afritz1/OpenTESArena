#include <algorithm>
#include <optional>

#include "CharacterCreationUiMVC.h"
#include "CharacterSheetUiMVC.h"
#include "ChooseAttributesUiState.h"
#include "ChooseClassCreationUiState.h"
#include "ChooseClassUiState.h"
#include "ChooseGenderUiState.h"
#include "ChooseNameUiState.h"
#include "ChooseRaceUiState.h"
#include "GameWorldUiState.h"
#include "MainMenuUiState.h"
#include "TextCinematicUiState.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaPortraitUtils.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Assets/TextureBuilder.h"
#include "../Assets/TextureFileMetadata.h"
#include "../Assets/TextureManager.h"
#include "../Audio/AudioManager.h"
#include "../Audio/MusicDefinition.h"
#include "../Audio/MusicLibrary.h"
#include "../Audio/MusicUtils.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Player/CharacterCreationState.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../Stats/CharacterRaceLibrary.h"
#include "../UI/FontDefinition.h"
#include "../UI/FontLibrary.h"
#include "../UI/TextEntry.h"

#include "components/utilities/String.h"

std::string CharacterCreationUiModel::getPlayerName(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	return std::string(charCreationState.name);
}

std::string CharacterCreationUiModel::getPlayerRaceName(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const int raceIndex = charCreationState.raceIndex;
	const CharacterRaceLibrary &characterRaceLibrary = CharacterRaceLibrary::getInstance();
	const CharacterRaceDefinition &characterRaceDefinition = characterRaceLibrary.getDefinition(raceIndex);
	return std::string(characterRaceDefinition.singularName);
}

std::string CharacterCreationUiModel::getPlayerClassName(Game &game)
{
	const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const int defID = charCreationState.classDefID;
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(defID);
	return charClassDef.name;
}

const PrimaryAttributes &CharacterCreationUiModel::getPlayerAttributes(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const PrimaryAttributes &attributes = charCreationState.attributes;
	return attributes;
}

std::string CharacterCreationUiModel::getPlayerLevel(Game &game)
{
	return std::to_string(1);
}

std::string CharacterCreationUiModel::getPlayerExperience(Game &game)
{
	return std::to_string(0);
}

std::string ChooseClassCreationUiModel::getTitleText(Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	std::string text = exeData.charCreation.chooseClassCreation;
	text = String::replace(text, '\r', '\n');
	return text;
}

std::string ChooseClassCreationUiModel::getGenerateButtonText(Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	return exeData.charCreation.chooseClassCreationGenerate;
}

std::string ChooseClassCreationUiModel::getSelectButtonText(Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	return exeData.charCreation.chooseClassCreationSelect;
}

std::string ChooseClassUiModel::getTitleText(Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	return exeData.charCreation.chooseClassList;
}

std::string ChooseClassUiModel::getArmorTooltipText(const CharacterClassDefinition &charClassDef)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();

	// The original game doesn't list the armor materials by themselves... have to make up something.
	const std::string armorMaterialStrings[] =
	{
		exeData.equipment.leatherArmorNames[0].substr(0, 7),
		exeData.equipment.chainArmorNames[0].substr(0, 5),
		exeData.equipment.plateArmorNames[0].substr(0, 5)
	};

	std::vector<int> allowedArmors(charClassDef.getAllowedArmorCount());
	for (int i = 0; i < static_cast<int>(allowedArmors.size()); i++)
	{
		allowedArmors[i] = charClassDef.getAllowedArmor(i);
	}

	std::sort(allowedArmors.begin(), allowedArmors.end());

	std::string armorString;

	// Decide what the armor string says.
	if (allowedArmors.size() == 0)
	{
		armorString = "None";
	}
	else
	{
		int lengthCounter = 0;

		// Collect all allowed armor display names for the class.
		for (int i = 0; i < static_cast<int>(allowedArmors.size()); i++)
		{
			const int materialType = allowedArmors[i];
			DebugAssertIndex(armorMaterialStrings, materialType);
			const std::string &materialString = armorMaterialStrings[materialType];
			lengthCounter += static_cast<int>(materialString.size());
			armorString.append(materialString);

			// If not the last element, add a comma.
			if (i < (static_cast<int>(allowedArmors.size()) - 1))
			{
				armorString.append(", ");

				// If too long, add a new line.
				if (lengthCounter > ChooseClassUiView::MaxTooltipLineLength)
				{
					lengthCounter = 0;
					armorString.append("\n   ");
				}
			}
		}
	}

	armorString.append(".");
	return armorString;
}

std::string ChooseClassUiModel::getShieldTooltipText(const CharacterClassDefinition &charClassDef)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	const Span<const std::string> shieldStrings(exeData.equipment.armorNames + 7, 4);

	std::vector<int> allowedShields(charClassDef.getAllowedShieldCount());
	for (int i = 0; i < static_cast<int>(allowedShields.size()); i++)
	{
		allowedShields[i] = charClassDef.getAllowedShield(i);
	}

	std::sort(allowedShields.begin(), allowedShields.end());

	std::string shieldsString;

	// Decide what the shield string says.
	if (allowedShields.size() == 0)
	{
		shieldsString = "None";
	}
	else
	{
		int lengthCounter = 0;

		// Collect all allowed shield display names for the class.
		for (int i = 0; i < static_cast<int>(allowedShields.size()); i++)
		{
			const int shieldType = allowedShields[i];
			const std::string &typeString = shieldStrings[shieldType];
			lengthCounter += static_cast<int>(typeString.size());
			shieldsString.append(typeString);

			// If not the last element, add a comma.
			if (i < (static_cast<int>(allowedShields.size()) - 1))
			{
				shieldsString.append(", ");

				// If too long, add a new line.
				if (lengthCounter > ChooseClassUiView::MaxTooltipLineLength)
				{
					lengthCounter = 0;
					shieldsString.append("\n   ");
				}
			}
		}
	}

	shieldsString.append(".");
	return shieldsString;
}

std::string ChooseClassUiModel::getWeaponTooltipText(const CharacterClassDefinition &charClassDef, Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	const auto &weaponStrings = exeData.equipment.weaponNames;

	std::vector<int> allowedWeapons(charClassDef.getAllowedWeaponCount());
	for (int i = 0; i < static_cast<int>(allowedWeapons.size()); i++)
	{
		allowedWeapons[i] = charClassDef.getAllowedWeapon(i);
	}

	std::sort(allowedWeapons.begin(), allowedWeapons.end(),
		[&weaponStrings](int a, int b)
	{
		DebugAssertIndex(weaponStrings, a);
		DebugAssertIndex(weaponStrings, b);
		const std::string &aStr = weaponStrings[a];
		const std::string &bStr = weaponStrings[b];
		return aStr.compare(bStr) < 0;
	});

	std::string weaponsString;

	// Decide what the weapon string says.
	if (allowedWeapons.size() == 0)
	{
		// If the class is allowed zero weapons, it still doesn't exclude fists, I think.
		weaponsString = "None";
	}
	else
	{
		int lengthCounter = 0;

		// Collect all allowed weapon display names for the class.
		for (int i = 0; i < static_cast<int>(allowedWeapons.size()); i++)
		{
			const int weaponID = allowedWeapons.at(i);
			DebugAssertIndex(weaponStrings, weaponID);
			const std::string &weaponName = weaponStrings[weaponID];
			lengthCounter += static_cast<int>(weaponName.size());
			weaponsString.append(weaponName);

			// If not the the last element, add a comma.
			if (i < (static_cast<int>(allowedWeapons.size()) - 1))
			{
				weaponsString.append(", ");

				// If too long, add a new line.
				if (lengthCounter > ChooseClassUiView::MaxTooltipLineLength)
				{
					lengthCounter = 0;
					weaponsString.append("\n   ");
				}
			}
		}
	}

	weaponsString.append(".");
	return weaponsString;
}

std::string ChooseClassUiModel::getFullTooltipText(const CharacterClassDefinition &charClassDef, Game &game)
{
	const char *castsMagicPrefix = charClassDef.castsMagic ? "Can" : "Cannot";
	const std::string armorTooltipText = ChooseClassUiModel::getArmorTooltipText(charClassDef);
	const std::string shieldTooltipText = ChooseClassUiModel::getShieldTooltipText(charClassDef);
	const std::string weaponTooltipText = ChooseClassUiModel::getWeaponTooltipText(charClassDef, game);

	char buffer[1024];
	std::snprintf(buffer, sizeof(buffer), "%s (%s class)\n\n%s cast magic\nHealth die: d%d\nArmors: %s\nShields: %s\nWeapons: %s",
		charClassDef.name, charClassDef.categoryName, castsMagicPrefix, charClassDef.healthDie,
		armorTooltipText.c_str(), shieldTooltipText.c_str(), weaponTooltipText.c_str());

	return std::string(buffer);
}

std::string ChooseGenderUiModel::getTitleText(Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	return exeData.charCreation.chooseGender;
}

std::string ChooseGenderUiModel::getMaleText(Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	return exeData.charCreation.chooseGenderMale;
}

std::string ChooseGenderUiModel::getFemaleText(Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	return exeData.charCreation.chooseGenderFemale;
}

std::string ChooseNameUiModel::getTitleText(Game &game)
{
	const auto &charCreationState = game.getCharacterCreationState();
	const auto &charClassLibrary = CharacterClassLibrary::getInstance();
	const int charClassDefID = charCreationState.classDefID;
	const auto &charClassDef = charClassLibrary.getDefinition(charClassDefID);

	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	std::string text = exeData.charCreation.chooseName;
	text = String::replace(text, "%s", charClassDef.name);
	return text;
}

bool ChooseNameUiModel::isCharacterAccepted(char c)
{
	// Only letters and spaces are allowed.
	return (c == ' ') || ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z'));
}

std::string ChooseRaceUiModel::getTitleText(Game &game)
{
	const auto &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const auto &exeData = binaryAssetLibrary.getExeData();
	std::string text = exeData.charCreation.chooseRace;
	text = String::replace(text, '\r', '\n');

	const auto &charCreationState = game.getCharacterCreationState();
	const auto &charClassLibrary = CharacterClassLibrary::getInstance();
	const int charClassDefID = charCreationState.classDefID;
	const auto &charClassDef = charClassLibrary.getDefinition(charClassDefID);

	// Replace first "%s" with player name.
	size_t index = text.find("%s");
	text.replace(index, 2, charCreationState.name);

	// Replace second "%s" with character class.
	index = text.find("%s");
	text.replace(index, 2, charClassDef.name);

	return text;
}

std::string ChooseRaceUiModel::getProvinceConfirmTitleText(Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	std::string text = exeData.charCreation.confirmRace;
	text = String::replace(text, '\r', '\n');

	const auto &charCreationState = game.getCharacterCreationState();
	const int raceIndex = charCreationState.raceIndex;

	const auto &charCreationProvinceNames = exeData.locations.charCreationProvinceNames;
	DebugAssertIndex(charCreationProvinceNames, raceIndex);
	const std::string &provinceName = charCreationProvinceNames[raceIndex];

	const CharacterRaceLibrary &characterRaceLibrary = CharacterRaceLibrary::getInstance();
	const CharacterRaceDefinition &characterRaceDefinition = characterRaceLibrary.getDefinition(raceIndex);
	const std::string pluralRaceName = characterRaceDefinition.pluralName;

	// Replace first %s with province name.
	size_t index = text.find("%s");
	text.replace(index, 2, provinceName);

	// Replace second %s with plural race name.
	index = text.find("%s");
	text.replace(index, 2, pluralRaceName);

	return text;
}

std::string ChooseRaceUiModel::getProvinceConfirmYesText(Game &game)
{
	return "Yes"; // @todo: get from ExeData
}

std::string ChooseRaceUiModel::getProvinceConfirmNoText(Game &game)
{
	return "No"; // @todo: get from ExeData
}

std::string ChooseRaceUiModel::getProvinceConfirmedFirstText(Game &game)
{
	const auto &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const auto &exeData = binaryAssetLibrary.getExeData();
	std::string segment = exeData.charCreation.confirmedRace1;
	segment = String::replace(segment, '\r', '\n');

	const auto &charCreationState = game.getCharacterCreationState();
	const int raceIndex = charCreationState.raceIndex;

	const auto &charCreationProvinceNames = exeData.locations.charCreationProvinceNames;
	DebugAssertIndex(charCreationProvinceNames, raceIndex);
	const std::string &provinceName = charCreationProvinceNames[raceIndex];

	const CharacterRaceLibrary &characterRaceLibrary = CharacterRaceLibrary::getInstance();
	const CharacterRaceDefinition &characterRaceDefinition = characterRaceLibrary.getDefinition(raceIndex);
	const std::string pluralRaceName = characterRaceDefinition.pluralName;

	const auto &charClassLibrary = CharacterClassLibrary::getInstance();
	const int charClassDefID = charCreationState.classDefID;
	const auto &charClassDef = charClassLibrary.getDefinition(charClassDefID);

	// Replace first %s with player class.
	size_t index = segment.find("%s");
	segment.replace(index, 2, charClassDef.name);

	// Replace second %s with player name.
	index = segment.find("%s");
	segment.replace(index, 2, charCreationState.name);

	// Replace third %s with province name.
	index = segment.find("%s");
	segment.replace(index, 2, provinceName);

	// Replace fourth %s with plural race name.
	index = segment.find("%s");
	segment.replace(index, 2, pluralRaceName);

	// If player is female, replace "his" with "her".
	if (!charCreationState.male)
	{
		index = segment.rfind("his");
		segment.replace(index, 3, "her");
	}

	return segment;
}

std::string ChooseRaceUiModel::getProvinceConfirmedSecondText(Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	std::string segment = exeData.charCreation.confirmedRace2;
	segment = String::replace(segment, '\r', '\n');

	const auto &charCreationState = game.getCharacterCreationState();
	const int raceIndex = charCreationState.raceIndex;

	// Get race description from TEMPLATE.DAT.
	const ArenaTemplateDat &templateDat = TextAssetLibrary::getInstance().templateDat;
	constexpr int raceTemplateIDs[] =
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
}

std::string ChooseRaceUiModel::getProvinceConfirmedThirdText(Game &game)
{
	const auto &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const auto &exeData = binaryAssetLibrary.getExeData();
	std::string segment = exeData.charCreation.confirmedRace3;
	segment = String::replace(segment, '\r', '\n');

	const auto &charCreationState = game.getCharacterCreationState();
	const auto &charClassLibrary = CharacterClassLibrary::getInstance();
	const int charClassDefID = charCreationState.classDefID;
	const auto &charClassDef = charClassLibrary.getDefinition(charClassDefID);

	const auto &preferredAttributes = exeData.charClasses.preferredAttributes;
	DebugAssertIndex(preferredAttributes, charClassDefID);
	const std::string &preferredAttributesStr = preferredAttributes[charClassDefID];

	// Replace first %s with desired class attributes.
	size_t index = segment.find("%s");
	segment.replace(index, 2, preferredAttributesStr);

	// Replace second %s with class name.
	index = segment.find("%s");
	segment.replace(index, 2, charClassDef.name);

	return segment;
}

std::string ChooseRaceUiModel::getProvinceConfirmedFourthText(Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	std::string segment = exeData.charCreation.confirmedRace4;
	segment = String::replace(segment, '\r', '\n');

	return segment;
}

int ChooseAttributesUiModel::rollClassic(int n, ArenaRandom &random)
{
	auto throwXdY = [&random](int x, int y)
	{
		int total = 0;
		for (int i = 0; i < x; i++)
		{
			total += 1 + (random.next() % y);
		}

		return total;
	};

	if (n <= 8)
	{
		return random.next() % n;
	}

	// Not fully understood but probably a normal distribution.
	int test = n;
	int x = 0, y = 0;
	while (true)
	{
		if (test % 6 == 0)
		{
			x = test / 6;
			y = 6;
			break;
		}
		else if (test % 5 == 0)
		{
			x = test / 5;
			y = 5;
			break;
		}
		else if (test % 4 == 0)
		{
			x = test / 4;
			y = 4;
			break;
		}
		else
		{
			test++;
		}
	}

	int value = 0;
	do
	{
		value = throwXdY(x, y);
	} while (value > n);

	return value;
}

std::string ChooseAttributesUiModel::getPlayerHealth(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const double maxHealth = static_cast<double>(charCreationState.maxHealth);
	return CharacterSheetUiModel::getStatusValueCurrentAndMaxString(maxHealth, maxHealth);
}

std::string ChooseAttributesUiModel::getPlayerStamina(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const double maxStamina = static_cast<double>(charCreationState.maxStamina);
	return CharacterSheetUiModel::getStatusValueCurrentAndMaxString(maxStamina, maxStamina);
}

std::string ChooseAttributesUiModel::getPlayerSpellPoints(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const double maxSpellPoints = static_cast<double>(charCreationState.maxSpellPoints);
	return CharacterSheetUiModel::getStatusValueCurrentAndMaxString(maxSpellPoints, maxSpellPoints);
}

std::string ChooseAttributesUiModel::getPlayerGold(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	return std::to_string(charCreationState.gold);
}

std::string ChooseAttributesUiModel::getInitialText(Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	std::string text = exeData.charCreation.distributeClassPoints;
	text = String::replace(text, '\r', '\n');

	// @temp show not implemented for attributes
	text += "\n\n(gameplay functionality not implemented)";

	return text;
}

std::string ChooseAttributesUiModel::getMessageBoxTitleText(Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	return exeData.charCreation.chooseAttributes;
}

std::string ChooseAttributesUiModel::getMessageBoxSaveText(Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	std::string text = exeData.charCreation.chooseAttributesSave;

	// Delete color override characters.
	// @todo: maybe transform the string in a better way so it works with Arena '\t' colors and some kind of modern format.
	text.erase(3, 2);
	text.erase(0, 2);

	return text;
}

std::string ChooseAttributesUiModel::getMessageBoxRerollText(Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	std::string text = exeData.charCreation.chooseAttributesReroll;

	// Delete color override characters.
	// @todo: maybe transform the string in a better way so it works with Arena '\t' colors and some kind of modern format.
	text.erase(3, 2);
	text.erase(0, 2);

	return text;
}

TextRenderColorOverrideInfo ChooseAttributesUiModel::getMessageBoxSaveColorOverrideInfo(Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	std::string text = exeData.charCreation.chooseAttributesSave;

	auto &textureManager = game.textureManager;
	const std::string &paletteName = ArenaPaletteName::Default;
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteName.c_str());
	if (!paletteID.has_value())
	{
		DebugCrashFormat("Couldn't get palette ID for \"%s\".", paletteName.c_str());
	}

	const Palette &palette = textureManager.getPaletteHandle(*paletteID);
	return TextRenderColorOverrideInfo::makeFromTabColorText(text, palette);
}

TextRenderColorOverrideInfo ChooseAttributesUiModel::getMessageBoxRerollColorOverrideInfo(Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	std::string text = exeData.charCreation.chooseAttributesReroll;

	auto &textureManager = game.textureManager;
	const std::string &paletteName = ArenaPaletteName::Default;
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteName.c_str());
	if (!paletteID.has_value())
	{
		DebugCrashFormat("Couldn't get palette ID for \"%s\".", paletteName.c_str());
	}

	const Palette &palette = textureManager.getPaletteHandle(*paletteID);
	return TextRenderColorOverrideInfo::makeFromTabColorText(text, palette);
}

std::string ChooseAttributesUiModel::getBonusPointsRemainingText(Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	const std::string text = exeData.charCreation.chooseAttributesBonusPointsRemaining;
	return text;
}

std::string ChooseAttributesUiModel::getAppearanceText(Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	std::string text = exeData.charCreation.chooseAppearance;
	text = String::replace(text, '\r', '\n');
	return text;
}

// ------------------------

TextureAsset CharacterCreationUiView::getNightSkyTextureAsset()
{
	return TextureAsset(ArenaTextureName::CharacterCreation);
}

UiTextureID CharacterCreationUiView::allocNightSkyTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = CharacterCreationUiView::getNightSkyTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, textureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for background texture.");
	}

	return textureID;
}

Rect ChooseClassUiView::getListRect(Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	const auto &chooseClassListUI = exeData.ui.chooseClassList;
	return Rect(
		chooseClassListUI.area.x,
		chooseClassListUI.area.y,
		chooseClassListUI.area.w,
		chooseClassListUI.area.h);
}

Rect ChooseClassUiView::getUpButtonRect(Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	const auto &chooseClassListUI = exeData.ui.chooseClassList;
	return Rect(
		chooseClassListUI.buttonUp.x,
		chooseClassListUI.buttonUp.y,
		chooseClassListUI.buttonUp.w,
		chooseClassListUI.buttonUp.h);
}

Rect ChooseClassUiView::getDownButtonRect(Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	const auto &chooseClassListUI = exeData.ui.chooseClassList;
	return Rect(
		chooseClassListUI.buttonDown.x,
		chooseClassListUI.buttonDown.y,
		chooseClassListUI.buttonDown.w,
		chooseClassListUI.buttonDown.h);
}

UiListBoxInitInfo ChooseClassUiView::makeListBoxProperties(const FontLibrary &fontLibrary)
{
	const char *fontName = ArenaFontName::A;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName, &fontDefIndex))
	{
		DebugCrash("Couldn't get class list box font \"" + std::string(fontName) + "\".");
	}

	constexpr int maxDisplayedItemCount = 6;
	std::string dummyText;
	for (int i = 0; i < maxDisplayedItemCount; i++)
	{
		if (i > 0)
		{
			dummyText += '\n';
		}

		std::string dummyLine(10, TextRenderUtils::LARGEST_CHAR); // Arbitrary worst-case line size.
		dummyText += dummyLine;
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(dummyText, fontDef);

	UiListBoxInitInfo listBoxInitInfo;
	listBoxInitInfo.textureWidth = textureGenInfo.width;
	listBoxInitInfo.textureHeight = textureGenInfo.height;
	listBoxInitInfo.itemPixelSpacing = 0;
	listBoxInitInfo.fontName = fontName;
	listBoxInitInfo.defaultTextColor = Color(85, 44, 20);
	return listBoxInitInfo;
}

TextureAsset ChooseClassUiView::getListBoxTextureAsset()
{
	return TextureAsset(ArenaTextureName::PopUp2);
}

UiTextureID ChooseClassUiView::allocPopUpTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset paletteTextureAsset = CharacterCreationUiView::getNightSkyTextureAsset();
	const TextureAsset textureAsset = ChooseClassUiView::getListBoxTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for class pop-up.");
	}

	return textureID;
}

TextureAsset ChooseRaceUiView::getBackgroundTextureAsset()
{
	return TextureAsset(ArenaTextureName::RaceSelect);
}

TextureAsset ChooseRaceUiView::getNoExitTextureAsset()
{
	return TextureAsset(ArenaTextureName::NoExit);
}

Rect ChooseRaceUiView::getProvinceConfirmTitleTextBoxRect(const std::string_view text, const FontLibrary &fontLibrary)
{
	const std::string &fontName = ChooseRaceUiView::ProvinceConfirmTitleFontName;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
	{
		DebugCrash("Couldn't get font definition for \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(
		text, fontDef, std::nullopt, ChooseRaceUiView::ProvinceConfirmTitleLineSpacing);
	return Rect(
		ChooseRaceUiView::ProvinceConfirmTitleCenterPoint,
		textureGenInfo.width,
		textureGenInfo.height);
}

MessageBoxBackgroundProperties ChooseRaceUiView::getProvinceConfirmMessageBoxBackgroundProperties()
{
	return MessageBoxBackgroundProperties(
		ChooseRaceUiView::ProvinceConfirmTitleTexturePatternType,
		22,
		0,
		std::nullopt,
		60,
		40);
}

MessageBoxTitleProperties ChooseRaceUiView::getProvinceConfirmMessageBoxTitleProperties(const std::string_view text, const FontLibrary &fontLibrary)
{
	const std::string &fontName = ChooseRaceUiView::ProvinceConfirmTitleFontName;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
	{
		DebugCrash("Couldn't get font definition for \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(
		text, fontDef, std::nullopt, ChooseRaceUiView::ProvinceConfirmTitleLineSpacing);
	return MessageBoxTitleProperties(
		fontName,
		textureGenInfo,
		ChooseRaceUiView::ProvinceConfirmTitleTextColor,
		ChooseRaceUiView::ProvinceConfirmTitleLineSpacing);
}

MessageBoxItemsProperties ChooseRaceUiView::getProvinceConfirmMessageBoxItemsProperties(const FontLibrary &fontLibrary)
{
	const std::string dummyText(5, TextRenderUtils::LARGEST_CHAR);
	const std::string &fontName = ChooseRaceUiView::ProvinceConfirmItemFontName;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
	{
		DebugCrash("Couldn't get font definition for \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(dummyText, fontDef);

	constexpr int itemCount = 2;
	return MessageBoxItemsProperties(
		itemCount,
		fontName,
		textureGenInfo,
		ChooseRaceUiView::ProvinceConfirmItemTextColor);
}

Rect ChooseRaceUiView::getProvinceConfirmedFirstTextureRect(int textWidth, int textHeight)
{
	const Int2 center(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);
	return Rect(
		center,
		textWidth + 20,
		std::max(textHeight, 40));
}

Rect ChooseRaceUiView::getProvinceConfirmedSecondTextureRect(int textWidth, int textHeight)
{
	const Int2 center(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);
	return Rect(
		center,
		textWidth + 20,
		std::max(textHeight + 14, 40));
}

Rect ChooseRaceUiView::getProvinceConfirmedThirdTextureRect(int textWidth, int textHeight)
{
	const Int2 center(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);
	return Rect(
		center,
		textWidth + 20,
		std::max(textHeight + 18, 40));
}

Rect ChooseRaceUiView::getProvinceConfirmedFourthTextureRect(int textWidth, int textHeight)
{
	const Int2 center(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);
	return Rect(
		center,
		textWidth + 20,
		std::max(textHeight + 8, 40));
}

Rect ChooseAttributesUiView::getMessageBoxTitleTextBoxRect(const std::string_view text, const FontLibrary &fontLibrary)
{
	const std::string &fontName = ChooseAttributesUiView::MessageBoxTitleFontName;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
	{
		DebugCrash("Couldn't get font definition for \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(text, fontDef);
	return Rect(
		ChooseAttributesUiView::MessageBoxTitleCenterPoint,
		textureGenInfo.width,
		textureGenInfo.height);
}

MessageBoxBackgroundProperties ChooseAttributesUiView::getMessageBoxBackgroundProperties()
{
	return MessageBoxBackgroundProperties(
		ChooseAttributesUiView::MessageBoxPatternType,
		12,
		0,
		std::nullopt,
		24,
		24);
}

MessageBoxTitleProperties ChooseAttributesUiView::getMessageBoxTitleProperties(const std::string_view text, const FontLibrary &fontLibrary)
{
	const std::string &fontName = ChooseAttributesUiView::MessageBoxTitleFontName;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
	{
		DebugCrash("Couldn't get font definition for \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(text, fontDef);
	return MessageBoxTitleProperties(fontName, textureGenInfo, ChooseAttributesUiView::MessageBoxTitleColor);
}

MessageBoxItemsProperties ChooseAttributesUiView::getMessageBoxItemsProperties(const FontLibrary &fontLibrary)
{
	const std::string dummyText(10, TextRenderUtils::LARGEST_CHAR);
	const std::string &fontName = ChooseAttributesUiView::MessageBoxItemFontName;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
	{
		DebugCrash("Couldn't get font definition for \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(dummyText, fontDef);

	constexpr int itemCount = 2;
	return MessageBoxItemsProperties(
		itemCount,
		fontName,
		textureGenInfo,
		ChooseAttributesUiView::MessageBoxItemTextColor);
}

int ChooseAttributesUiView::getDistributePointsTextBoxTextureWidth(int textWidth)
{
	return textWidth + 12;
}

int ChooseAttributesUiView::getDistributePointsTextBoxTextureHeight(int textHeight)
{
	return textHeight + 12;
}

int ChooseAttributesUiView::getAppearanceTextBoxTextureWidth(int textWidth)
{
	return textWidth + 10;
}

int ChooseAttributesUiView::getAppearanceTextBoxTextureHeight(int textHeight)
{
	return textHeight + 12;
}

Int2 ChooseAttributesUiView::getBodyOffset(Game &game)
{
	const TextureAsset textureAsset = ChooseAttributesUiView::getBodyTextureAsset(game);

	TextureManager &textureManager = game.textureManager;
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAsset);
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for body \"" + textureAsset.filename + "\".");
	}

	const TextureBuilder &bodyTexture = textureManager.getTextureBuilderHandle(*textureBuilderID);
	return Int2(ArenaRenderUtils::SCREEN_WIDTH - bodyTexture.width, 0);
}

Int2 ChooseAttributesUiView::getHeadOffset(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.male;
	const int raceID = charCreationState.raceIndex;

	constexpr bool trimmed = false;
	const std::string &headsFilename = ArenaPortraitUtils::getHeads(isMale, raceID, trimmed);

	auto &textureManager = game.textureManager;
	const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(headsFilename.c_str());
	if (!metadataID.has_value())
	{
		DebugCrash("Couldn't get texture file metadata for \"" + headsFilename + "\".");
	}

	const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);
	const int headOffsetIndex = charCreationState.portraitIndex;
	return textureFileMetadata.getOffset(headOffsetIndex);
}

Int2 ChooseAttributesUiView::getShirtOffset(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.male;

	const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
	const int charClassDefID = charCreationState.classDefID;
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(charClassDefID);
	const bool isMagic = charClassDef.castsMagic;

	return ArenaPortraitUtils::getShirtOffset(isMale, isMagic);
}

Int2 ChooseAttributesUiView::getPantsOffset(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.male;
	return ArenaPortraitUtils::getPantsOffset(isMale);
}

TextureAsset ChooseAttributesUiView::getBodyTextureAsset(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.male;
	const int raceID = charCreationState.raceIndex;

	const std::string bodyFilename = ArenaPortraitUtils::getBody(isMale, raceID);
	return TextureAsset(bodyFilename);
}

Buffer<TextureAsset> ChooseAttributesUiView::getHeadTextureAssets(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.male;
	const int raceID = charCreationState.raceIndex;

	constexpr bool trimmed = false;
	std::string headsFilename = ArenaPortraitUtils::getHeads(isMale, raceID, trimmed);

	auto &textureManager = game.textureManager;
	return TextureUtils::makeTextureAssets(headsFilename, textureManager);
}

TextureAsset ChooseAttributesUiView::getShirtTextureAsset(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.male;

	const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
	const int charClassDefID = charCreationState.classDefID;
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(charClassDefID);
	const bool isMagic = charClassDef.castsMagic;

	const std::string shirtFilename = ArenaPortraitUtils::getShirt(isMale, isMagic);
	return TextureAsset(shirtFilename);
}

TextureAsset ChooseAttributesUiView::getPantsTextureAsset(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.male;

	const std::string pantsFilename = ArenaPortraitUtils::getPants(isMale);
	return TextureAsset(pantsFilename);
}

UiTextureID ChooseAttributesUiView::allocBodyTexture(Game &game)
{
	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;

	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();
	const TextureAsset textureAsset = ChooseAttributesUiView::getBodyTextureAsset(game);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for character body.");
	}

	return textureID;
}

UiTextureID ChooseAttributesUiView::allocShirtTexture(Game &game)
{
	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;

	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();
	const TextureAsset textureAsset = ChooseAttributesUiView::getShirtTextureAsset(game);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for character shirt.");
	}

	return textureID;
}

UiTextureID ChooseAttributesUiView::allocPantsTexture(Game &game)
{
	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;

	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();
	const TextureAsset textureAsset = ChooseAttributesUiView::getPantsTextureAsset(game);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for character pants.");
	}

	return textureID;
}

UiTextureID ChooseAttributesUiView::allocHeadTexture(const TextureAsset &textureAsset,
	TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for character head \"" + textureAsset.filename + "\".");
	}

	return textureID;
}

// ------------------------

void ChooseClassCreationUiController::onBackToMainMenuInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		Game &game = values.game;
		game.setCharacterCreationState(nullptr);
		game.setNextContext(MainMenuUI::ContextName);

		const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
		const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinition(MusicType::MainMenu, game.random);
		if (musicDef == nullptr)
		{
			DebugLogWarning("Missing main menu music.");
		}

		AudioManager &audioManager = game.audioManager;
		audioManager.setMusic(musicDef);

		InputManager &inputManager = game.inputManager;
		inputManager.setInputActionMapActive(InputActionMapName::CharacterCreation, false);
	}
}

void ChooseClassCreationUiController::onGenerateButtonSelected(Game &game)
{
	// @todo: eventually go to a ChooseQuestionsUI with "pop-up" message
}

void ChooseClassCreationUiController::onSelectButtonSelected(Game &game)
{
	game.setNextContext(ChooseClassUI::ContextName);
}

void ChooseClassUiController::onBackToChooseClassCreationInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		Game &game = values.game;
		game.setNextContext(ChooseClassCreationUI::ContextName);
	}
}

void ChooseClassUiController::onItemButtonSelected(Game &game, int charClassDefID)
{
	CharacterCreationState &charCreationState = game.getCharacterCreationState();
	charCreationState.classDefID = charClassDefID;

	game.setNextContext(ChooseNameUI::ContextName);
}

void ChooseGenderUiController::onBackToChooseNameInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		Game &game = values.game;
		game.setNextContext(ChooseNameUI::ContextName);
	}
}

void ChooseGenderUiController::onMaleButtonSelected(Game &game)
{
	CharacterCreationState &charCreationState = game.getCharacterCreationState();
	charCreationState.male = true;

	game.setNextContext(ChooseRaceUI::ContextName);
}

void ChooseGenderUiController::onFemaleButtonSelected(Game &game)
{
	CharacterCreationState &charCreationState = game.getCharacterCreationState();
	charCreationState.male = false;

	game.setNextContext(ChooseRaceUI::ContextName);
}

void ChooseNameUiController::onBackToChooseClassInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		Game &game = values.game;
		InputManager &inputManager = game.inputManager;
		inputManager.setTextInputMode(false);

		CharacterCreationState &charCreationState = game.getCharacterCreationState();
		charCreationState.setName(nullptr);

		game.setNextContext(ChooseClassUI::ContextName);
	}
}

void ChooseNameUiController::onTextInput(const std::string_view text, std::string &name, bool *outDirty)
{
	DebugAssert(outDirty != nullptr);
	*outDirty = TextEntry::append(name, text, ChooseNameUiModel::isCharacterAccepted, CharacterCreationState::MAX_NAME_LENGTH);
}

void ChooseNameUiController::onBackspaceInputAction(const InputActionCallbackValues &values, std::string &name, bool *outDirty)
{
	DebugAssert(outDirty != nullptr);

	if (values.performed)
	{
		*outDirty = TextEntry::backspace(name);
	}
}

void ChooseNameUiController::onAcceptInputAction(const InputActionCallbackValues &values, const std::string &name)
{
	if (values.performed)
	{
		if (name.size() > 0)
		{
			Game &game = values.game;
			InputManager &inputManager = game.inputManager;
			inputManager.setTextInputMode(false);

			CharacterCreationState &charCreationState = game.getCharacterCreationState();
			charCreationState.setName(name.c_str());

			game.setNextContext(ChooseGenderUI::ContextName);
		}
	}
}

void ChooseAttributesUiController::onPostCharacterCreationCinematicFinished(Game &game)
{
	game.setNextContext(GameWorldUI::ContextName);

	const MusicDefinition *musicDef = MusicUtils::getRandomDungeonMusicDefinition(game.random);
	if (musicDef == nullptr)
	{
		DebugLogWarning("Missing dungeon music.");
	}

	AudioManager &audioManager = game.audioManager;
	audioManager.setMusic(musicDef);
}
