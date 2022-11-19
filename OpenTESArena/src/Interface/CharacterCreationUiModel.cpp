#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Entities/PrimaryAttribute.h"
#include "../Game/Game.h"
#include "../Items/ArmorMaterial.h"
#include "../Items/ArmorMaterialType.h"
#include "../Items/MetalType.h"
#include "../Items/Shield.h"
#include "../Items/ShieldType.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

std::string CharacterCreationUiModel::getPlayerName(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	return std::string(charCreationState.getName());
}

std::string CharacterCreationUiModel::getPlayerRaceName(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const ExeData &exeData = game.getBinaryAssetLibrary().getExeData();

	const auto &singularRaceNames = exeData.races.singularNames;
	const int raceNameIndex = charCreationState.getRaceIndex();
	DebugAssertIndex(singularRaceNames, raceNameIndex);
	return singularRaceNames[raceNameIndex];
}

std::string CharacterCreationUiModel::getPlayerClassName(Game &game)
{
	const CharacterClassLibrary &charClassLibrary = game.getCharacterClassLibrary();
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const int defID = charCreationState.getClassDefID();
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(defID);
	return charClassDef.getName();
}

std::vector<PrimaryAttribute> CharacterCreationUiModel::getPlayerAttributes(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const PrimaryAttributeSet &attributeSet = charCreationState.getAttributes();
	return attributeSet.getAll();
}

std::string ChooseClassCreationUiModel::getTitleText(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	std::string text = exeData.charCreation.chooseClassCreation;
	text = String::replace(text, '\r', '\n');
	return text;
}

std::string ChooseClassCreationUiModel::getGenerateButtonText(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	return exeData.charCreation.chooseClassCreationGenerate;
}

std::string ChooseClassCreationUiModel::getGenerateButtonTooltipText()
{
	return "Answer questions\n(not implemented)";
}

std::string ChooseClassCreationUiModel::getSelectButtonText(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	return exeData.charCreation.chooseClassCreationSelect;
}

std::string ChooseClassCreationUiModel::getSelectButtonTooltipText()
{
	return "Choose from a list";
}

std::string ChooseClassUiModel::getTitleText(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	return exeData.charCreation.chooseClassList;
}

std::string ChooseClassUiModel::getArmorTooltipText(const CharacterClassDefinition &charClassDef)
{
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
			auto materialString = ArmorMaterial::typeToString(
				static_cast<ArmorMaterialType>(materialType));
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
			MetalType dummyMetal = MetalType::Iron;
			auto typeString = Shield(static_cast<ShieldType>(shieldType), dummyMetal).typeToString();
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
	// Get weapon names from the executable.
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	const auto &weaponStrings = exeData.equipment.weaponNames;

	std::vector<int> allowedWeapons(charClassDef.getAllowedWeaponCount());
	for (int i = 0; i < static_cast<int>(allowedWeapons.size()); i++)
	{
		allowedWeapons[i] = charClassDef.getAllowedWeapon(i);
	}

	std::sort(allowedWeapons.begin(), allowedWeapons.end(),
		[&weaponStrings](int a, int b)
	{
		const std::string &aStr = weaponStrings.at(a);
		const std::string &bStr = weaponStrings.at(b);
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
			const std::string &weaponName = weaponStrings.at(weaponID);
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
	// Doesn't look like the category name is easy to get from the original data. Potentially could attach something
	// to the char class definition like a bool saying "the class name is also a category name".
	constexpr std::array<const char*, 3> ClassCategoryNames =
	{
		"Mage", "Thief", "Warrior"
	};

	const int categoryIndex = charClassDef.getCategoryID();
	DebugAssertIndex(ClassCategoryNames, categoryIndex);
	const std::string categoryName = ClassCategoryNames[categoryIndex];
	const std::string text = charClassDef.getName() + " (" + categoryName + " class)" + "\n\n" +
		(charClassDef.canCastMagic() ? "Can" : "Cannot") + " cast magic" + "\n" +
		"Health die: " + "d" + std::to_string(charClassDef.getHealthDie()) + "\n" +
		"Armors: " + ChooseClassUiModel::getArmorTooltipText(charClassDef) + "\n" +
		"Shields: " + ChooseClassUiModel::getShieldTooltipText(charClassDef) + "\n" +
		"Weapons: " + ChooseClassUiModel::getWeaponTooltipText(charClassDef, game);

	return text;
}

std::string ChooseGenderUiModel::getTitleText(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	return exeData.charCreation.chooseGender;
}

std::string ChooseGenderUiModel::getMaleText(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	return exeData.charCreation.chooseGenderMale;
}

std::string ChooseGenderUiModel::getFemaleText(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	return exeData.charCreation.chooseGenderFemale;
}

std::string ChooseNameUiModel::getTitleText(Game &game)
{
	const auto &charCreationState = game.getCharacterCreationState();
	const auto &charClassLibrary = game.getCharacterClassLibrary();
	const int charClassDefID = charCreationState.getClassDefID();
	const auto &charClassDef = charClassLibrary.getDefinition(charClassDefID);

	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	std::string text = exeData.charCreation.chooseName;
	text = String::replace(text, "%s", charClassDef.getName());
	return text;
}

bool ChooseNameUiModel::isCharacterAccepted(char c)
{
	// Only letters and spaces are allowed.
	return (c == ' ') || ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z'));
}

std::string ChooseRaceUiModel::getTitleText(Game &game)
{
	const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
	const auto &exeData = binaryAssetLibrary.getExeData();
	std::string text = exeData.charCreation.chooseRace;
	text = String::replace(text, '\r', '\n');

	const auto &charCreationState = game.getCharacterCreationState();
	const auto &charClassLibrary = game.getCharacterClassLibrary();
	const int charClassDefID = charCreationState.getClassDefID();
	const auto &charClassDef = charClassLibrary.getDefinition(charClassDefID);

	// Replace first "%s" with player name.
	size_t index = text.find("%s");
	text.replace(index, 2, charCreationState.getName());

	// Replace second "%s" with character class.
	index = text.find("%s");
	text.replace(index, 2, charClassDef.getName());

	return text;
}

std::string ChooseRaceUiModel::getProvinceConfirmTitleText(Game &game)
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

std::string ChooseRaceUiModel::getProvinceTooltipText(Game &game, int provinceID)
{
	// Get the race name associated with the province.
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	const auto &pluralNames = exeData.races.pluralNames;
	DebugAssertIndex(pluralNames, provinceID);
	const std::string &raceName = pluralNames[provinceID];
	return "Land of the " + raceName;
}

std::string ChooseRaceUiModel::getProvinceConfirmedFirstText(Game &game)
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
}

std::string ChooseRaceUiModel::getProvinceConfirmedSecondText(Game &game)
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
}

std::string ChooseRaceUiModel::getProvinceConfirmedThirdText(Game &game)
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
}

std::string ChooseRaceUiModel::getProvinceConfirmedFourthText(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	std::string segment = exeData.charCreation.confirmedRace4;
	segment = String::replace(segment, '\r', '\n');

	return segment;
}

std::string ChooseAttributesUiModel::getInitialText(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	std::string text = exeData.charCreation.distributeClassPoints;
	text = String::replace(text, '\r', '\n');
	return text;
}

std::string ChooseAttributesUiModel::getMessageBoxTitleText(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	return exeData.charCreation.chooseAttributes;
}

std::string ChooseAttributesUiModel::getMessageBoxSaveText(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	std::string text = exeData.charCreation.chooseAttributesSave;

	// Delete color override characters.
	// @todo: maybe transform the string in a better way so it works with Arena '\t' colors and some kind of modern format.
	text.erase(3, 2);
	text.erase(0, 2);

	return text;
}

std::string ChooseAttributesUiModel::getMessageBoxRerollText(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	std::string text = exeData.charCreation.chooseAttributesReroll;

	// Delete color override characters.
	// @todo: maybe transform the string in a better way so it works with Arena '\t' colors and some kind of modern format.
	text.erase(3, 2);
	text.erase(0, 2);

	return text;
}

std::vector<TextRenderUtils::ColorOverrideInfo::Entry> ChooseAttributesUiModel::getMessageBoxSaveColorOverrides(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	std::string text = exeData.charCreation.chooseAttributesSave;

	auto &textureManager = game.getTextureManager();
	const std::string &paletteName = ArenaPaletteName::Default;
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteName.c_str());
	if (!paletteID.has_value())
	{
		DebugCrash("Couldn't get palette ID for \"" + paletteName + "\".");
	}

	const Palette &palette = textureManager.getPaletteHandle(*paletteID);
	return TextRenderUtils::ColorOverrideInfo::makeEntriesFromText(text, palette);
}

std::vector<TextRenderUtils::ColorOverrideInfo::Entry> ChooseAttributesUiModel::getMessageBoxRerollColorOverrides(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	std::string text = exeData.charCreation.chooseAttributesReroll;
	
	auto &textureManager = game.getTextureManager();
	const std::string &paletteName = ArenaPaletteName::Default;
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteName.c_str());
	if (!paletteID.has_value())
	{
		DebugCrash("Couldn't get palette ID for \"" + paletteName + "\".");
	}

	const Palette &palette = textureManager.getPaletteHandle(*paletteID);
	return TextRenderUtils::ColorOverrideInfo::makeEntriesFromText(text, palette);
}

std::string ChooseAttributesUiModel::getAppearanceText(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	std::string text = exeData.charCreation.chooseAppearance;
	text = String::replace(text, '\r', '\n');
	return text;
}
