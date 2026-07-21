#include "DialogueFunctions.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Game/Game.h"
#include "../Player/Player.h"
#include "../Stats/CharacterRaceLibrary.h"
#include "../WorldMap/ArenaLocationUtils.h"

#include "components/utilities/String.h"

std::string DialogueFunctions::get_a(const Game &game)
{
	return "%a";
}

std::string DialogueFunctions::get_adj(const Game &game)
{
	return "%adj";
}

std::string DialogueFunctions::get_adn(const Game &game)
{
	return "%adn";
}

std::string DialogueFunctions::get_adv(const Game &game)
{
	return "%adv";
}

std::string DialogueFunctions::get_amn(const Game &game)
{
	return "%amn";
}

std::string DialogueFunctions::get_an(const Game &game)
{
	return "%an";
}

std::string DialogueFunctions::get_at(const Game &game)
{
	return "%at";
}

std::string DialogueFunctions::get_art(const Game &game)
{
	return "%art";
}

std::string DialogueFunctions::get_apr(const Game &game)
{
	return "%apr";
}

std::string DialogueFunctions::get_arc(const Game &game)
{
	return "%arc";
}

std::string DialogueFunctions::get_ba(const Game &game)
{
	return "%ba";
}

std::string DialogueFunctions::get_ccs(const Game &game)
{
	return "%ccs";
}

std::string DialogueFunctions::get_cll(const Game &game)
{
	return "%cll";
}

std::string DialogueFunctions::get_cn(const Game &game)
{
	return "%cn";
}

std::string DialogueFunctions::get_cn2(const Game &game)
{
	return "%cn2";
}

std::string DialogueFunctions::get_cp(const Game &game)
{
	return "%cp";
}

std::string DialogueFunctions::get_ct(const Game &game)
{
	const GameState &gameState = game.gameState;
	const LocationDefinition &locationDef = gameState.getLocationDefinition();
	if (locationDef.getType() != LocationDefinitionType::City)
	{
		return std::string();
	}

	const LocationCityDefinition &cityDef = locationDef.getCityDefinition();
	return cityDef.typeDisplayName;
}

std::string DialogueFunctions::get_da(const Game &game)
{
	return "%da";
}

std::string DialogueFunctions::get_de(const Game &game)
{
	return "%de";
}

std::string DialogueFunctions::get_des(const Game &game)
{
	return "%des";
}

std::string DialogueFunctions::get_di(const Game &game)
{
	return "%di";
}

std::string DialogueFunctions::get_dit(const Game &game)
{
	return "%dit";
}

std::string DialogueFunctions::get_doc(const Game &game)
{
	const DialogueManager &dialogueManager = game.dialogueManager;
	constexpr int baseEntryKey = 263;
	const int keyOffset = dialogueManager.getEntityOccupationIndex();
	return dialogueManager.getRandomTemplateDatEntryValue(baseEntryKey + keyOffset);
}

std::string DialogueFunctions::get_ds(const Game &game)
{
	return "%ds";
}

std::string DialogueFunctions::get_du(const Game &game)
{
	return "%du";
}

std::string DialogueFunctions::get_dnl(const Game &game)
{
	return "%dnl";
}

std::string DialogueFunctions::get_ef(const Game &game)
{
	return "%ef";
}

std::string DialogueFunctions::get_en(const Game &game)
{
	return "%en";
}

std::string DialogueFunctions::get_fq(const Game &game)
{
	return "%fq";
}

std::string DialogueFunctions::get_fn(const Game &game)
{
	const std::string entityName = DialogueFunctions::get_n(game);
	return String::split(entityName)[0];
}

std::string DialogueFunctions::get_g(const Game &game)
{
	return "%g";
}

std::string DialogueFunctions::get_g2(const Game &game)
{
	return "%g2";
}

std::string DialogueFunctions::get_g3(const Game &game)
{
	return "%g3";
}

std::string DialogueFunctions::get_hc(const Game &game)
{
	return "%hc";
}

std::string DialogueFunctions::get_hod(const Game &game)
{
	return "%hod";
}

std::string DialogueFunctions::get_jok(const Game &game)
{
	const DialogueManager &dialogueManager = game.dialogueManager;
	return dialogueManager.getRandomTemplateDatEntryValue(363);
}

std::string DialogueFunctions::get_lp(const Game &game)
{
	const GameState &gameState = game.gameState;
	const ProvinceDefinition &provinceDef = gameState.getProvinceDefinition();
	return provinceDef.getName();
}

std::string DialogueFunctions::get_mi(const Game &game)
{
	return "%mi";
}

std::string DialogueFunctions::get_mn(const Game &game)
{
	return "%mn";
}

std::string DialogueFunctions::get_mpr(const Game &game)
{
	return "%mpr";
}

std::string DialogueFunctions::get_mt(const Game &game)
{
	return "%mt";
}

std::string DialogueFunctions::get_n(const Game &game)
{
	const DialogueManager &dialogueManager = game.dialogueManager;
	const EntityInstance &entityInst = dialogueManager.getEntityInstance();

	const EntityChunkManager &entityChunkManager = game.sceneManager.entityChunkManager;
	DebugAssert(entityInst.npcNameID >= 0);
	const EntityNpcName &npcName = entityChunkManager.npcNames.get(entityInst.npcNameID);
	return npcName.name;
}

std::string DialogueFunctions::get_nap(const Game &game)
{
	return "%nap";
}

std::string DialogueFunctions::get_nc(const Game &game)
{
	return "%nc";
}

std::string DialogueFunctions::get_ne(const Game &game)
{
	return "%ne";
}

std::string DialogueFunctions::get_nh(const Game &game)
{
	return "%nh";
}

std::string DialogueFunctions::get_nhd(const Game &game)
{
	return "%nhd";
}

std::string DialogueFunctions::get_non(const Game &game)
{
	return "%non";
}

std::string DialogueFunctions::get_nr(const Game &game)
{
	return "%nr";
}

std::string DialogueFunctions::get_nt(const Game &game)
{
	return "%nt";
}

std::string DialogueFunctions::get_o(const Game &game)
{
	return "%o";
}

std::string DialogueFunctions::get_oap(const Game &game)
{
	return "%oap";
}

std::string DialogueFunctions::get_oc(const Game &game)
{
	const DialogueManager &dialogueManager = game.dialogueManager;
	constexpr int entryKey = 262;
	const int index = dialogueManager.getEntityOccupationIndex();
	return dialogueManager.getTemplateDatEntryValueAtIndex(entryKey, index);
}

std::string DialogueFunctions::get_omq(const Game &game)
{
	return "%omq";
}

std::string DialogueFunctions::get_opp(const Game &game)
{
	return "%opp";
}

std::string DialogueFunctions::get_oth(const Game &game)
{
	return "%oth";
}

std::string DialogueFunctions::get_pcn(const Game &game)
{
	return game.player.displayName;
}

std::string DialogueFunctions::get_pcf(const Game &game)
{
	return game.player.firstName;
}

std::string DialogueFunctions::get_pre(const Game &game)
{
	return "%pre";
}

std::string DialogueFunctions::get_qc(const Game &game)
{
	return "%qc";
}

std::string DialogueFunctions::get_qt(const Game &game)
{
	return "%qt";
}

std::string DialogueFunctions::get_qf(const Game &game)
{
	return "%qf";
}

std::string DialogueFunctions::get_qmn(const Game &game)
{
	return "%qmn";
}

std::string DialogueFunctions::get_r(const Game &game)
{
	return "%r";
}

std::string DialogueFunctions::get_ra(const Game &game)
{
	const Player &player = game.player;
	const CharacterRaceDefinition &charRaceDef = CharacterRaceLibrary::getInstance().getDefinition(player.raceID);
	return charRaceDef.singularName;
}

std::string DialogueFunctions::get_rcn(const Game &game)
{
	return "%rcn";
}

std::string DialogueFunctions::get_rf(const Game &game)
{
	return "%rf";
}

std::string DialogueFunctions::get_rpn(const Game &game)
{
	return "%rpn";
}

std::string DialogueFunctions::get_sn(const Game &game)
{
	return "%sn";
}

std::string DialogueFunctions::get_st(const Game &game)
{
	return "%st";
}

std::string DialogueFunctions::get_suf(const Game &game)
{
	return "%suf";
}

std::string DialogueFunctions::get_t(const Game &game)
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

std::string DialogueFunctions::get_tan(const Game &game)
{
	return "%tan";
}

std::string DialogueFunctions::get_tc(const Game &game)
{
	return "%tc";
}

std::string DialogueFunctions::get_tem(const Game &game)
{
	return "%tem";
}

std::string DialogueFunctions::get_tg(const Game &game)
{
	return "%tg";
}

std::string DialogueFunctions::get_ti(const Game &game)
{
	return "%ti";
}

std::string DialogueFunctions::get_tl(const Game &game)
{
	return "%tl";
}

std::string DialogueFunctions::get_tq(const Game &game)
{
	return "%tq";
}

std::string DialogueFunctions::get_tt(const Game &game)
{
	return "%tt";
}
