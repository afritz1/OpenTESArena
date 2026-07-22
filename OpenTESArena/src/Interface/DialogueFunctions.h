#pragma once

#include <functional>
#include <string>

class Game;

using DialogueFunction = std::function<std::string(Game&)>;

// Substitution token replacement functions for conversations. Some functions modify global dialogue
// state like the current gender of a subject (not necessarily the conversation entity).
namespace DialogueFunctions
{
	std::string get_a(Game &game); // Active quest reward
	std::string get_adj(Game &game); // NPC name adjective (same structure as the prefix)
	std::string get_adn(Game &game); // artifact dungeon name
	std::string get_adv(Game &game); // NPC name adverb (same structure as the prefix)
	std::string get_amn(Game &game); // artifact dungeon name?
	std::string get_an(Game &game); // NPC name that gives artifact quest
	std::string get_at(Game &game); // Active quest dungeon type (Mine, Cavern, Labyrinth, ...)
	std::string get_art(Game &game); // current artifact name
	std::string get_apr(Game &game); // artifact province name
	std::string get_arc(Game &game); // artifact leaving player's possession
	std::string get_ba(Game &game); // Active quest bonus reward (unused)
	std::string get_ccs(Game &game); // current MQ city
	std::string get_cll(Game &game); // current MQ dungeon
	std::string get_cn(Game &game); // if in wilderness, current province name; otherwise, the current city name
	std::string get_cn2(Game &game); // the nearest settlement name (same city type as the current)
	std::string get_cp(Game &game); // current MQ province
	std::string get_ct(Game &game); // current city type
	std::string get_da(Game &game); // Active quest deadline date (short format)
	std::string get_de(Game &game); // Active quest deadline, in days
	std::string get_des(Game &game); // NPC description (same structure as the prefix)
	std::string get_di(Game &game); // cardinal direction to dlgPoint
	std::string get_dit(Game &game); // number of days quest NPC waits for player
	std::string get_doc(Game &game); // NPC occupation / profession details
	std::string get_ds(Game &game); // Active quest giver description (see below)
	std::string get_du(Game &game); // number of days quest NPC says fast travel will take
	std::string get_dnl(Game &game); // first word of %nc value
	std::string get_ef(Game &game); // used with building names?
	std::string get_en(Game &game); // NPC's home equipment store name
	std::string get_fq(Game &game); // active quest giver's first name (female if not quest giver seed & 3)
	std::string get_fn(Game &game); // NPC first name (similar to %n)
	std::string get_g(Game &game); // he / she / it depending on dlgGender
	std::string get_g2(Game &game); // him / her / it depending on dlgGender
	std::string get_g3(Game &game); // his / her / its depending on dlgGender
	std::string get_hc(Game &game); // player's home province
	std::string get_hod(Game &game); // nearest holiday string : random normalized #169 + holidayId
	std::string get_jok(Game &game); // joke: random normalized #363
	std::string get_lp(Game &game); // current province
	std::string get_mi(Game &game); // item name for the active noble quest
	std::string get_mn(Game &game); // monster name for the active quest
	std::string get_mpr(Game &game); // province where the artifact map is
	std::string get_mt(Game &game); // monster for the active quest ("Troll" etc.)
	std::string get_n(Game &game); // NPC name
	std::string get_nap(Game &game); // gold cost amount to get artifact info from NPC (see %oap)
	std::string get_nc(Game &game); // a local name for the active quest
	std::string get_ne(Game &game); // a local name for the active quest
	std::string get_nh(Game &game); // nearest holiday name
	std::string get_nhd(Game &game); // nearest holiday date, without the year
	std::string get_non(Game &game); // empty string
	std::string get_nr(Game &game); // (same as %ne)
	std::string get_nt(Game &game); // NPC's home tavern name
	std::string get_o(Game &game); // a rival faction name for the active quest, or "%qf"
	std::string get_oap(Game &game); // gold cost amount to get artifact info from NPC (see %nap)
	std::string get_oc(Game &game); // NPC occupation / profession
	std::string get_omq(Game &game); // quest item for the active quest (depends on the destination)
	std::string get_opp(Game &game); // assassin from hostile guild
	std::string get_oth(Game &game); // oath: random line from oaths[prov].prov is the current province or random if imp. city
	std::string get_pcn(Game &game); // PC full name. Sets dlgGender
	std::string get_pcf(Game &game); // PC first name. Sets dlgGender
	std::string get_pre(Game &game); // NPC name prefix (from a special data structure)
	std::string get_qc(Game &game); // city where the active quest was taken
	std::string get_qt(Game &game); // description of the active quest type ("deliver something" etc.)
	std::string get_qf(Game &game); // title + first name for the opponent in the active quest
	std::string get_qmn(Game &game); // notification that quest object is nearby
	std::string get_r(Game &game); // relation type for the active quest ("daughter" etc.)
	std::string get_ra(Game &game); // PC race
	std::string get_rcn(Game &game); // random city-state name in the current province
	std::string get_rf(Game &game); // ruler first name, or emperor's name if Imp. city
	std::string get_rpn(Game &game); // random province name
	std::string get_sn(Game &game); // name of snake charmer NPC's pet
	std::string get_st(Game &game); // "war/peace" based on algorithm
	std::string get_suf(Game &game); // NPC name suffix (same structure as the prefix)
	std::string get_t(Game &game); // ruler title
	std::string get_tan(Game &game); // $dlgDungeonName
	std::string get_tc(Game &game); // target city name for the active quest
	std::string get_tem(Game &game); // NPC's home temple name
	std::string get_tg(Game &game); // target faction name for the active quest
	std::string get_ti(Game &game); // (same as %du)
	std::string get_tl(Game &game); // target venue name for the active quest, or "%dnl"
	std::string get_tq(Game &game); // quest giver title for the active quest
	std::string get_tt(Game &game); // class of the npc for the active quest

	const std::pair<const char*, DialogueFunction> FunctionMappings[] =
	{
		{ "%a", DialogueFunctions::get_a },
		{ "%adj", DialogueFunctions::get_adj },
		{ "%adn", DialogueFunctions::get_adn },
		{ "%adv", DialogueFunctions::get_adv },
		{ "%amn", DialogueFunctions::get_amn },
		{ "%an", DialogueFunctions::get_an },
		{ "%at", DialogueFunctions::get_at },
		{ "%art", DialogueFunctions::get_art },
		{ "%apr", DialogueFunctions::get_apr },
		{ "%arc", DialogueFunctions::get_arc },
		{ "%ba", DialogueFunctions::get_ba },
		{ "%ccs", DialogueFunctions::get_ccs },
		{ "%cll", DialogueFunctions::get_cll },
		{ "%cn", DialogueFunctions::get_cn },
		{ "%cn2", DialogueFunctions::get_cn2 },
		{ "%cp", DialogueFunctions::get_cp },
		{ "%ct", DialogueFunctions::get_ct },
		{ "%da", DialogueFunctions::get_da },
		{ "%de", DialogueFunctions::get_de },
		{ "%des", DialogueFunctions::get_des },
		{ "%di", DialogueFunctions::get_di },
		{ "%dit", DialogueFunctions::get_dit },
		{ "%doc", DialogueFunctions::get_doc },
		{ "%ds", DialogueFunctions::get_ds },
		{ "%du", DialogueFunctions::get_du },
		{ "%dnl", DialogueFunctions::get_dnl },
		{ "%ef", DialogueFunctions::get_ef },
		{ "%en", DialogueFunctions::get_en },
		{ "%fq", DialogueFunctions::get_fq },
		{ "%fn", DialogueFunctions::get_fn },
		{ "%g", DialogueFunctions::get_g },
		{ "%g2", DialogueFunctions::get_g2 },
		{ "%g3", DialogueFunctions::get_g3 },
		{ "%hc", DialogueFunctions::get_hc },
		{ "%hod", DialogueFunctions::get_hod },
		{ "%jok", DialogueFunctions::get_jok },
		{ "%lp", DialogueFunctions::get_lp },
		{ "%mi", DialogueFunctions::get_mi },
		{ "%mn", DialogueFunctions::get_mn },
		{ "%mpr", DialogueFunctions::get_mpr },
		{ "%mt", DialogueFunctions::get_mt },
		{ "%n", DialogueFunctions::get_n },
		{ "%nap", DialogueFunctions::get_nap },
		{ "%nc", DialogueFunctions::get_nc },
		{ "%ne", DialogueFunctions::get_ne },
		{ "%nh", DialogueFunctions::get_nh },
		{ "%nhd", DialogueFunctions::get_nhd },
		{ "%non", DialogueFunctions::get_non },
		{ "%nr", DialogueFunctions::get_nr },
		{ "%nt", DialogueFunctions::get_nt },
		{ "%o", DialogueFunctions::get_o },
		{ "%oap", DialogueFunctions::get_oap },
		{ "%oc", DialogueFunctions::get_oc },
		{ "%omq", DialogueFunctions::get_omq },
		{ "%opp", DialogueFunctions::get_opp },
		{ "%oth", DialogueFunctions::get_oth },
		{ "%pcn", DialogueFunctions::get_pcn },
		{ "%pcf", DialogueFunctions::get_pcf },
		{ "%pre", DialogueFunctions::get_pre },
		{ "%qc", DialogueFunctions::get_qc },
		{ "%qt", DialogueFunctions::get_qt },
		{ "%qf", DialogueFunctions::get_qf },
		{ "%qmn", DialogueFunctions::get_qmn },
		{ "%r", DialogueFunctions::get_r },
		{ "%ra", DialogueFunctions::get_ra },
		{ "%rcn", DialogueFunctions::get_rcn },
		{ "%rf", DialogueFunctions::get_rf },
		{ "%rpn", DialogueFunctions::get_rpn },
		{ "%sn", DialogueFunctions::get_sn },
		{ "%st", DialogueFunctions::get_st },
		{ "%suf", DialogueFunctions::get_suf },
		{ "%t", DialogueFunctions::get_t },
		{ "%tan", DialogueFunctions::get_tan },
		{ "%tc", DialogueFunctions::get_tc },
		{ "%tem", DialogueFunctions::get_tem },
		{ "%tg", DialogueFunctions::get_tg },
		{ "%ti", DialogueFunctions::get_ti },
		{ "%tl", DialogueFunctions::get_tl },
		{ "%tq", DialogueFunctions::get_tq },
		{ "%tt", DialogueFunctions::get_tt }
	};
}
