#include "DialogueFunctions.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Game/Game.h"
#include "../Math/ArenaMathUtils.h"
#include "../Player/Player.h"
#include "../Stats/CharacterRaceLibrary.h"
#include "../World/MapType.h"
#include "../WorldMap/ArenaLocationUtils.h"

#include "components/utilities/String.h"

namespace
{
	constexpr int DialogueGenderMale = 0;
	constexpr int DialogueGenderFemale = 1;
	constexpr int DialogueGenderNeutral = 2;
}

std::string DialogueFunctions::get_a(Game &game)
{
	return "%a";
}

std::string DialogueFunctions::get_adj(Game &game)
{
	return "%adj";
}

std::string DialogueFunctions::get_adn(Game &game)
{
	return "%adn";
}

std::string DialogueFunctions::get_adv(Game &game)
{
	return "%adv";
}

std::string DialogueFunctions::get_amn(Game &game)
{
	return "%amn";
}

std::string DialogueFunctions::get_an(Game &game)
{
	return "%an";
}

std::string DialogueFunctions::get_at(Game &game)
{
	return "%at";
}

std::string DialogueFunctions::get_art(Game &game)
{
	return "%art";
}

std::string DialogueFunctions::get_apr(Game &game)
{
	return "%apr";
}

std::string DialogueFunctions::get_arc(Game &game)
{
	return "%arc";
}

std::string DialogueFunctions::get_ba(Game &game)
{
	return "%ba";
}

std::string DialogueFunctions::get_ccs(Game &game)
{
	return "%ccs";
}

std::string DialogueFunctions::get_cll(Game &game)
{
	return "%cll";
}

std::string DialogueFunctions::get_cn(Game &game)
{
	const GameState &gameState = game.gameState;
	const MapType mapType = gameState.getActiveMapType();

	if (mapType == MapType::Wilderness || (gameState.isActiveMapNested() && gameState.getExteriorMapType() == MapType::Wilderness))
	{
		const ProvinceDefinition &provinceDef = gameState.getProvinceDefinition();
		return provinceDef.getName();
	}
	else
	{
		const LocationDefinition &locationDef = gameState.getLocationDefinition();
		return locationDef.getName();
	}
}

std::string DialogueFunctions::get_cn2(Game &game)
{
	const GameState &gameState = game.gameState;

	auto getLocationGlobalPoint = [](const LocationDefinition &locationDef, const ProvinceDefinition &provinceDef)
	{
		const Int2 localPoint(locationDef.getScreenX(), locationDef.getScreenY());
		return ArenaLocationUtils::getGlobalPoint(localPoint, provinceDef.getGlobalRect());
	};

	const ProvinceDefinition &playerProvinceDef = gameState.getProvinceDefinition();
	const LocationDefinition &playerLocationDef = gameState.getLocationDefinition();
	const LocationCityDefinition &playerCityDef = playerLocationDef.getCityDefinition();
	const Int2 playerGlobalPoint = getLocationGlobalPoint(playerLocationDef, playerProvinceDef);

	const ProvinceLibrary &provinceLibrary = ProvinceLibrary::getInstance();
	const ProvinceDefinition *closestProvinceDef = nullptr;
	const LocationDefinition *closestLocationDef = nullptr;
	for (int provinceIndex = 0; provinceIndex < provinceLibrary.getProvinceCount(); provinceIndex++)
	{
		const ProvinceDefinition &provinceDef = provinceLibrary.getProvinceDef(provinceIndex);
		for (int locationIndex = 0; locationIndex < provinceDef.getLocationCount(); locationIndex++)
		{
			const LocationDefinition &locationDef = provinceDef.getLocationDef(locationIndex);
			if (locationDef.getType() != LocationDefinitionType::City)
			{
				continue;
			}

			const LocationCityDefinition &locationCityDef = locationDef.getCityDefinition();
			if (locationCityDef.type != playerCityDef.type)
			{
				continue;
			}

			if (&locationDef == &playerLocationDef)
			{
				continue;
			}

			if (closestLocationDef == nullptr)
			{
				closestProvinceDef = &provinceDef;
				closestLocationDef = &locationDef;
				continue;
			}

			const Int2 globalPoint = getLocationGlobalPoint(locationDef, provinceDef);
			const Int2 closestGlobalPoint = getLocationGlobalPoint(*closestLocationDef, *closestProvinceDef);
			const int distance = ArenaMathUtils::getApproximateEuclideanDistance(globalPoint, playerGlobalPoint);
			const int closestDistance = ArenaMathUtils::getApproximateEuclideanDistance(closestGlobalPoint, playerGlobalPoint);
			if (distance < closestDistance)
			{
				closestProvinceDef = &provinceDef;
				closestLocationDef = &locationDef;
			}
		}
	}

	if (closestLocationDef == nullptr)
	{
		DebugLogErrorFormat("Couldn't find closest matching location to %s.", playerLocationDef.getName().c_str());
		return std::string();
	}

	return closestLocationDef->getName();
}

std::string DialogueFunctions::get_cp(Game &game)
{
	return "%cp";
}

std::string DialogueFunctions::get_ct(Game &game)
{
	const GameState &gameState = game.gameState;
	const LocationDefinition &locationDef = gameState.getLocationDefinition();
	if (locationDef.getType() != LocationDefinitionType::City)
	{
		return std::string();
	}

	const LocationCityDefinition &cityDef = locationDef.getCityDefinition();
	return cityDef.typeDisplayNameLowercase;
}

std::string DialogueFunctions::get_da(Game &game)
{
	return "%da";
}

std::string DialogueFunctions::get_de(Game &game)
{
	return "%de";
}

std::string DialogueFunctions::get_des(Game &game)
{
	return "%des";
}

std::string DialogueFunctions::get_di(Game &game)
{
	const DialogueManager &dialogueManager = game.dialogueManager;
	const std::optional<CardinalDirectionName> &directionName = dialogueManager.dialogueDirection;
	if (!directionName.has_value())
	{
		DebugLogError("Dialogue direction is not set for %di.");
		return "<missing direction>";
	}

	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
	const Span<const std::string> cardinalDirectionStrings = exeData.dialogue.cardinalDirections;
	const int index = static_cast<int>(*directionName);
	return cardinalDirectionStrings[index];
}

std::string DialogueFunctions::get_dit(Game &game)
{
	return "%dit";
}

std::string DialogueFunctions::get_doc(Game &game)
{
	const DialogueManager &dialogueManager = game.dialogueManager;
	constexpr int baseEntryKey = 263;
	const int keyOffset = dialogueManager.getEntityOccupationIndex();
	return dialogueManager.getRandomTemplateDatEntryValue(baseEntryKey + keyOffset);
}

std::string DialogueFunctions::get_ds(Game &game)
{
	return "%ds";
}

std::string DialogueFunctions::get_du(Game &game)
{
	return "%du";
}

std::string DialogueFunctions::get_dnl(Game &game)
{
	return "%dnl";
}

std::string DialogueFunctions::get_ef(Game &game)
{
	return "%ef";
}

std::string DialogueFunctions::get_en(Game &game)
{
	const DialogueManager &dialogueManager = game.dialogueManager;
	return dialogueManager.getNearestEquipmentStoreName();
}

std::string DialogueFunctions::get_fq(Game &game)
{
	return "%fq";
}

std::string DialogueFunctions::get_fn(Game &game)
{
	const std::string entityName = DialogueFunctions::get_n(game);
	return String::split(entityName)[0];
}

std::string DialogueFunctions::get_g(Game &game)
{
	const DialogueManager &dialogueManager = game.dialogueManager;
	if (!dialogueManager.isDialogueGenderValid())
	{
		DebugLogError("Dialogue gender is not set for %g.");
		return "<missing subject gender>";
	}

	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
	const Span<const std::string> pronouns = exeData.dialogue.subjectPronouns;
	return pronouns[dialogueManager.dialogueGender];
}

std::string DialogueFunctions::get_g2(Game &game)
{
	const DialogueManager &dialogueManager = game.dialogueManager;
	if (!dialogueManager.isDialogueGenderValid())
	{
		DebugLogError("Dialogue gender is not set for %g2.");
		return "<missing object gender>";
	}

	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
	const Span<const std::string> pronouns = exeData.dialogue.objectPronouns;
	return pronouns[dialogueManager.dialogueGender];
}

std::string DialogueFunctions::get_g3(Game &game)
{
	const DialogueManager &dialogueManager = game.dialogueManager;
	if (!dialogueManager.isDialogueGenderValid())
	{
		DebugLogError("Dialogue gender is not set for %g3.");
		return "<missing possessive gender>";
	}

	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
	const Span<const std::string> pronouns = exeData.dialogue.possessivePronouns;
	return pronouns[dialogueManager.dialogueGender];
}

std::string DialogueFunctions::get_hc(Game &game)
{
	const Player &player = game.player;
	const ProvinceLibrary &provinceLibrary = ProvinceLibrary::getInstance();
	const ProvinceDefinition &playerHomeProvince = provinceLibrary.getProvinceDef(player.raceID);
	return playerHomeProvince.getName();
}

std::string DialogueFunctions::get_hod(Game &game)
{
	return "%hod";
}

std::string DialogueFunctions::get_jok(Game &game)
{
	const DialogueManager &dialogueManager = game.dialogueManager;
	return dialogueManager.getRandomTemplateDatEntryValue(363);
}

std::string DialogueFunctions::get_lp(Game &game)
{
	const GameState &gameState = game.gameState;
	const ProvinceDefinition &provinceDef = gameState.getProvinceDefinition();
	return provinceDef.getName();
}

std::string DialogueFunctions::get_mi(Game &game)
{
	return "%mi";
}

std::string DialogueFunctions::get_mn(Game &game)
{
	return "%mn";
}

std::string DialogueFunctions::get_mpr(Game &game)
{
	return "%mpr";
}

std::string DialogueFunctions::get_mt(Game &game)
{
	return "%mt";
}

std::string DialogueFunctions::get_n(Game &game)
{
	const DialogueManager &dialogueManager = game.dialogueManager;
	const EntityInstance &entityInst = dialogueManager.getEntityInstance();

	const EntityChunkManager &entityChunkManager = game.sceneManager.entityChunkManager;
	DebugAssert(entityInst.npcNameID >= 0);
	const EntityNpcName &npcName = entityChunkManager.npcNames.get(entityInst.npcNameID);
	return npcName.name;
}

std::string DialogueFunctions::get_nap(Game &game)
{
	return "%nap";
}

std::string DialogueFunctions::get_nc(Game &game)
{
	return "%nc";
}

std::string DialogueFunctions::get_ne(Game &game)
{
	return "%ne";
}

std::string DialogueFunctions::get_nh(Game &game)
{
	return "%nh";
}

std::string DialogueFunctions::get_nhd(Game &game)
{
	return "%nhd";
}

std::string DialogueFunctions::get_non(Game &game)
{
	return "%non";
}

std::string DialogueFunctions::get_nr(Game &game)
{
	return "%nr";
}

std::string DialogueFunctions::get_nt(Game &game)
{
	const DialogueManager &dialogueManager = game.dialogueManager;
	return dialogueManager.getNearestTavernName();
}

std::string DialogueFunctions::get_o(Game &game)
{
	return "%o";
}

std::string DialogueFunctions::get_oap(Game &game)
{
	return "%oap";
}

std::string DialogueFunctions::get_oc(Game &game)
{
	const DialogueManager &dialogueManager = game.dialogueManager;
	constexpr int entryKey = 262;
	const int index = dialogueManager.getEntityOccupationIndex();
	return dialogueManager.getTemplateDatEntryValueAtIndex(entryKey, index);
}

std::string DialogueFunctions::get_omq(Game &game)
{
	return "%omq";
}

std::string DialogueFunctions::get_opp(Game &game)
{
	return "%opp";
}

std::string DialogueFunctions::get_oth(Game &game)
{
	return "%oth";
}

std::string DialogueFunctions::get_pcn(Game &game)
{
	const Player &player = game.player;
	DialogueManager &dialogueManager = game.dialogueManager;
	dialogueManager.dialogueGender = player.male ? DialogueGenderMale : DialogueGenderFemale;

	return player.displayName;
}

std::string DialogueFunctions::get_pcf(Game &game)
{
	const Player &player = game.player;
	DialogueManager &dialogueManager = game.dialogueManager;
	dialogueManager.dialogueGender = player.male ? DialogueGenderMale : DialogueGenderFemale;

	return game.player.firstName;
}

std::string DialogueFunctions::get_pre(Game &game)
{
	return "%pre";
}

std::string DialogueFunctions::get_qc(Game &game)
{
	return "%qc";
}

std::string DialogueFunctions::get_qt(Game &game)
{
	return "%qt";
}

std::string DialogueFunctions::get_qf(Game &game)
{
	return "%qf";
}

std::string DialogueFunctions::get_qmn(Game &game)
{
	return "%qmn";
}

std::string DialogueFunctions::get_r(Game &game)
{
	return "%r";
}

std::string DialogueFunctions::get_ra(Game &game)
{
	const Player &player = game.player;
	const CharacterRaceDefinition &charRaceDef = CharacterRaceLibrary::getInstance().getDefinition(player.raceID);
	return charRaceDef.singularName;
}

std::string DialogueFunctions::get_rcn(Game &game)
{
	const GameState &gameState = game.gameState;
	const ProvinceDefinition &provinceDef = gameState.getProvinceDefinition();
	ArenaRandom tempRandom(game.arenaRandom.getSeed());
	const int randomCityStateIndex = tempRandom.next(8);
	const LocationDefinition &selectedLocationDef = provinceDef.getLocationDef(randomCityStateIndex);
	return selectedLocationDef.getName();
}

std::string DialogueFunctions::get_rf(Game &game)
{
	const GameState &gameState = game.gameState;
	const int provinceIndex = gameState.getProvinceDefinition().getRaceID();
	const LocationDefinition &locationDef = gameState.getLocationDefinition();
	const LocationCityDefinition &cityDef = locationDef.getCityDefinition();
	const bool isRulerMale = cityDef.rulerIsMale;
	ArenaRandom tempRandom(cityDef.rulerSeed);

	std::string str;
	if (provinceIndex == ArenaLocationUtils::CENTER_PROVINCE_ID)
	{
		const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
		str = exeData.locations.centerProvinceRulerName;
	}
	else
	{
		const TextAssetLibrary &textAssetLibrary = TextAssetLibrary::getInstance();
		const std::string fullName = textAssetLibrary.generateNpcName(provinceIndex, isRulerMale, tempRandom);
		const Buffer<std::string> tokens = String::split(fullName, ' ');
		str = tokens[0];
	}

	DialogueManager &dialogueManager = game.dialogueManager;
	dialogueManager.dialogueGender = isRulerMale ? DialogueGenderMale : DialogueGenderFemale;

	return str;
}

std::string DialogueFunctions::get_rpn(Game &game)
{
	ArenaRandom tempRandom(game.arenaRandom.getSeed());
	const ProvinceLibrary &provinceLibrary = ProvinceLibrary::getInstance();
	const int randomProvinceIndex = tempRandom.next(provinceLibrary.getProvinceCount());
	const ProvinceDefinition &selectedProvinceDef = provinceLibrary.getProvinceDef(randomProvinceIndex);
	return selectedProvinceDef.getName();
}

std::string DialogueFunctions::get_sn(Game &game)
{
	const GameState &gameState = game.gameState;
	const DialogueManager &dialogueManager = game.dialogueManager;
	const int provinceIndex = gameState.getProvinceDefinition().getRaceID();
	ArenaRandom tempRandom(dialogueManager.entityInstID);
	const TextAssetLibrary &textAssetLibrary = TextAssetLibrary::getInstance();
	const std::string fullName = textAssetLibrary.generateNpcName(provinceIndex, false, tempRandom);
	const Buffer<std::string> tokens = String::split(fullName, ' ');
	return tokens[0];
}

std::string DialogueFunctions::get_st(Game &game)
{
	const GameState &gameState = game.gameState;
	const DialogueManager &dialogueManager = game.dialogueManager;
	const LocationDefinition &locationDef = gameState.getLocationDefinition();
	const LocationCityDefinition &cityDef = locationDef.getCityDefinition();
	const Span<const ArenaWeatherType> globalWeathers = gameState.getWorldMapWeathers();

	// War/peace depends on weather which in theory is a slow-changing global variable.
	const int provinceIndex = gameState.getProvinceDefinition().getRaceID();
	const int locationIndex = gameState.getLocationIndex();
	const int quarterIndex = gameState.getLocationGlobalQuarter(provinceIndex, locationIndex);
	const ArenaWeatherType quarterWeather = globalWeathers[quarterIndex];
	const uint32_t seed1 = cityDef.citySeed;
	const uint32_t seed2 = (seed1 & 0xFF) + (static_cast<int>(quarterWeather) & 0x7);
	const uint32_t seed = (seed1 & 0xFFFFFF00) | (seed2 & 0xFF);
	ArenaRandom tempRandom(seed);
	
	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
	const int index = tempRandom.next(2);
	DebugAssertIndex(exeData.dialogue.neighborWarPeace, index);
	return exeData.dialogue.neighborWarPeace[index];
}

std::string DialogueFunctions::get_suf(Game &game)
{
	return "%suf";
}

std::string DialogueFunctions::get_t(Game &game)
{
	const GameState &gameState = game.gameState;
	const int provinceID = gameState.getProvinceDefinition().getRaceID();
	const LocationDefinition &locationDef = gameState.getLocationDefinition();
	const LocationCityDefinition &cityDef = locationDef.getCityDefinition();
	const int localCityID = gameState.getLocationIndex();
	const ArenaLocationType locationType = ArenaLocationUtils::getCityType(localCityID);
	const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	ArenaRandom random(game.arenaRandom.getSeed());
	return binaryAssetLibrary.getRulerTitle(provinceID, locationType, cityDef.rulerIsMale, random);
}

std::string DialogueFunctions::get_tan(Game &game)
{
	return "%tan";
}

std::string DialogueFunctions::get_tc(Game &game)
{
	return "%tc";
}

std::string DialogueFunctions::get_tem(Game &game)
{
	const DialogueManager &dialogueManager = game.dialogueManager;
	return dialogueManager.getNearestTempleName();
}

std::string DialogueFunctions::get_tg(Game &game)
{
	return "%tg";
}

std::string DialogueFunctions::get_ti(Game &game)
{
	return "%ti";
}

std::string DialogueFunctions::get_tl(Game &game)
{
	return "%tl";
}

std::string DialogueFunctions::get_tq(Game &game)
{
	return "%tq";
}

std::string DialogueFunctions::get_tt(Game &game)
{
	return "%tt";
}
