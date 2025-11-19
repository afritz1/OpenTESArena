#include "DialogueDefinition.h"

#include <algorithm>
#include <array>

#include "components/debug/Debug.h"

namespace
{
	using DialogueVariableDefinitionArray = std::array<DialogueVariableDefinition,
		static_cast<size_t>(DialogueVariableType::Count)>;

	constexpr DialogueVariableDefinition makeVar(DialogueVariableType type, std::string_view token,
		std::string_view description, DialogueVariableAttributes attributes = DialogueVariableAttribute::None)
	{
		return DialogueVariableDefinition{ type, token, description, attributes };
	}

	constexpr DialogueVariableDefinitionArray DialogueVariableDefinitions =
	{
		makeVar(DialogueVariableType::ActiveQuestReward, "%a", "Active quest reward amount."),
		makeVar(DialogueVariableType::NpcNameAdjective, "%adj", "NPC name adjective, paired with the prefix data."),
		makeVar(DialogueVariableType::UnknownAdn, "%adn", "Undocumented variable (mirrors %adj data)."),
		makeVar(DialogueVariableType::NpcNameAdverb, "%adv", "NPC name adverb, paired with the prefix data."),
		makeVar(DialogueVariableType::UnknownAmn, "%amn", "Undocumented variable related to NPC names."),
		makeVar(DialogueVariableType::UnknownAn, "%an", "Undocumented variable related to NPC names."),
		makeVar(DialogueVariableType::ActiveQuestDungeonType, "%at", "Active quest dungeon type (Mine, Cavern, Labyrinth, etc.)."),
		makeVar(DialogueVariableType::CurrentArtifactName, "%art", "Current artifact name."),
		makeVar(DialogueVariableType::ArtifactProvinceName, "%apr", "Province name referenced by the current artifact rumor."),
		makeVar(DialogueVariableType::UnknownArc, "%arc", "Undocumented variable referenced by the artifact quest."),
		makeVar(DialogueVariableType::ActiveQuestBonusReward, "%ba", "Active quest bonus reward (unused in classic Arena)."),
		makeVar(DialogueVariableType::CurrentMainQuestCity, "%ccs", "Current main quest city."),
		makeVar(DialogueVariableType::CurrentMainQuestDungeon, "%ccl", "Current main quest dungeon."),
		makeVar(DialogueVariableType::CurrentCityOrProvince, "%cn", "Current city name, or current province name while in the wilderness."),
		makeVar(DialogueVariableType::NearestSettlementName, "%cn2", "Nearest settlement name of the same type as the current city."),
		makeVar(DialogueVariableType::CurrentMainQuestProvince, "%cp", "Current main quest province."),
		makeVar(DialogueVariableType::CurrentCityType, "%ct", "Current city type (city-state, town, village, etc.)."),
		makeVar(DialogueVariableType::ActiveQuestDeadlineDate, "%da", "Active quest deadline date (short format)."),
		makeVar(DialogueVariableType::ActiveQuestDeadlineDays, "%de", "Active quest deadline in days."),
		makeVar(DialogueVariableType::NpcDescription, "%des", "NPC description (gender-aware string)."),
		makeVar(DialogueVariableType::DirectionToDialoguePoint, "%di", "Direction to the dialogue point being referenced."),
		makeVar(DialogueVariableType::UnknownDit, "%dit", "Undocumented variable related to directions."),
		makeVar(DialogueVariableType::UnknownDoc, "%doc", "Undocumented variable."),
		makeVar(DialogueVariableType::ActiveQuestGiverDescription, "%ds", "Active quest giver description (matches NPC descriptor data)."),
		makeVar(DialogueVariableType::UnknownDu, "%du", "Undocumented variable."),
		makeVar(DialogueVariableType::DialogueNpcLocalNameFirstWord, "%dnl", "First word of the %nc value."),
		makeVar(DialogueVariableType::UnknownEf, "%ef", "Undocumented variable."),
		makeVar(DialogueVariableType::UnknownEn, "%en", "Undocumented variable."),
		makeVar(DialogueVariableType::ActiveQuestGiverFirstName, "%fq", "Active quest giver's first name (female if the giver seed & 3)."),
		makeVar(DialogueVariableType::UnknownFn, "%fn", "Undocumented variable."),
		makeVar(DialogueVariableType::SpeakerPronounSubject, "%g", "Speaker pronoun he/she/it depending on dlgGender."),
		makeVar(DialogueVariableType::SpeakerPronounObject, "%g2", "Speaker pronoun him/her/it depending on dlgGender."),
		makeVar(DialogueVariableType::SpeakerPronounPossessive, "%g3", "Speaker pronoun his/her/its depending on dlgGender."),
		makeVar(DialogueVariableType::UnknownHp, "%hp", "Undocumented variable."),
		makeVar(DialogueVariableType::PlayerHomeCity, "%hc", "Player's home city."),
		makeVar(DialogueVariableType::NearestHolidayString, "%hod", "Nearest holiday text entry (random normalized #169+holidayId).",
			DialogueVariableAttribute::NormalizedWhitespace | DialogueVariableAttribute::RandomizedSelection),
		makeVar(DialogueVariableType::RandomJoke, "%jok", "Random joke from TEMPLATE.DAT (normalized #363).",
			DialogueVariableAttribute::NormalizedWhitespace | DialogueVariableAttribute::RandomizedSelection),
		makeVar(DialogueVariableType::CurrentProvince, "%lp", "Current province name."),
		makeVar(DialogueVariableType::NobleQuestItemName, "%mi", "Item name for the active noble quest."),
		makeVar(DialogueVariableType::ActiveQuestMonsterName, "%mn", "Monster name for the active quest."),
		makeVar(DialogueVariableType::ArtifactMapProvinceName, "%mpr", "Province where the artifact map is located."),
		makeVar(DialogueVariableType::UnknownMqt, "%mqt", "Undocumented variable."),
		makeVar(DialogueVariableType::ActiveQuestMonsterType, "%mt", "Monster type for the active quest (\"Troll\", etc.)."),
		makeVar(DialogueVariableType::UnknownN, "%n", "Undocumented variable."),
		makeVar(DialogueVariableType::ArtifactPrice, "%nap", "Artifact price."),
		makeVar(DialogueVariableType::ActiveQuestLocalName, "%nc", "Local name for the active quest."),
		makeVar(DialogueVariableType::UnknownNcl, "%ncl", "Undocumented variable."),
		makeVar(DialogueVariableType::ActiveQuestLocalAlias, "%ne", "Alternate local name for the active quest."),
		makeVar(DialogueVariableType::NearestHolidayName, "%nh", "Nearest holiday name."),
		makeVar(DialogueVariableType::NearestHolidayDate, "%nhd", "Nearest holiday date without the year."),
		makeVar(DialogueVariableType::EmptyString, "%non", "Empty string placeholder."),
		makeVar(DialogueVariableType::ActiveQuestLocalAliasAlt, "%nr", "Alternate local name for the active quest (same as %ne)."),
		makeVar(DialogueVariableType::UnknownNt, "%nt", "Undocumented variable."),
		makeVar(DialogueVariableType::RivalFactionName, "%o", "Rival faction name for the active quest, or %qf when not available."),
		makeVar(DialogueVariableType::UnknownOap, "%oap", "Undocumented variable."),
		makeVar(DialogueVariableType::UnknownOc, "%oc", "Undocumented variable."),
		makeVar(DialogueVariableType::ActiveQuestObjectiveItem, "%omq", "Quest item for the active quest (destination dependent)."),
		makeVar(DialogueVariableType::UnknownOpp, "%opp", "Undocumented variable."),
		makeVar(DialogueVariableType::Oath, "%oth", "Oath string pulled from the province oath table.",
			DialogueVariableAttribute::RandomizedSelection),
		makeVar(DialogueVariableType::PlayerFullName, "%pcn", "Player full name (sets dlgGender).",
			DialogueVariableAttribute::SetsSpeakerGender),
		makeVar(DialogueVariableType::PlayerFirstName, "%pcf", "Player first name (sets dlgGender).",
			DialogueVariableAttribute::SetsSpeakerGender),
		makeVar(DialogueVariableType::NpcNamePrefix, "%pre", "NPC name prefix from the descriptor table."),
		makeVar(DialogueVariableType::ActiveQuestCityName, "%qc", "City where the active quest was taken."),
		makeVar(DialogueVariableType::ActiveQuestTypeDescription, "%qt", "Description of the active quest type."),
		makeVar(DialogueVariableType::ActiveQuestOpponentName, "%qf", "Title + first name for the opponent in the active quest."),
		makeVar(DialogueVariableType::UnknownQmn, "%qmn", "Undocumented variable."),
		makeVar(DialogueVariableType::ActiveQuestRelation, "%r", "Relation type for the active quest (\"daughter\", etc.)."),
		makeVar(DialogueVariableType::PlayerRaceName, "%ra", "Player race name."),
		makeVar(DialogueVariableType::RandomCityStateInProvince, "%rcn", "Random city-state name in the current province.",
			DialogueVariableAttribute::RandomizedSelection),
		makeVar(DialogueVariableType::RulerFirstName, "%rf", "Province ruler first name, or \"Uriel Septim\" when applicable."),
		makeVar(DialogueVariableType::RandomProvinceName, "%rpn", "Random province name.",
			DialogueVariableAttribute::RandomizedSelection),
		makeVar(DialogueVariableType::UnknownSn, "%sn", "Undocumented variable."),
		makeVar(DialogueVariableType::StateOfWarOrPeace, "%st", "Current war/peace state (see War/Peace algorithm)."),
		makeVar(DialogueVariableType::NpcNameSuffix, "%suf", "NPC name suffix from the descriptor data."),
		makeVar(DialogueVariableType::RulerTitle, "%t", "Province ruler title."),
		makeVar(DialogueVariableType::TargetDungeonName, "%tan", "Dungeon name tied to the dialogue context."),
		makeVar(DialogueVariableType::TargetCityName, "%tc", "Target city for the active quest."),
		makeVar(DialogueVariableType::UnknownTem, "%tem", "Undocumented variable."),
		makeVar(DialogueVariableType::TargetFactionName, "%tg", "Target faction name for the active quest."),
		makeVar(DialogueVariableType::UnknownTi, "%ti", "Undocumented variable (same as %du)."),
		makeVar(DialogueVariableType::TargetLocationName, "%tl", "Target venue name for the active quest, or %dnl fallback."),
		makeVar(DialogueVariableType::ActiveQuestGiverTitle, "%tq", "Quest giver title for the active quest."),
		makeVar(DialogueVariableType::ActiveQuestNpcClass, "%tt", "Class of the NPC involved in the active quest.")
	};
}

std::span<const DialogueVariableDefinition> DialogueDefinitionLibrary::getVariableDefinitions()
{
	return DialogueVariableDefinitions;
}

const DialogueVariableDefinition *DialogueDefinitionLibrary::tryGetVariableDefinitionByToken(std::string_view token)
{
	const auto iter = std::find_if(DialogueVariableDefinitions.begin(), DialogueVariableDefinitions.end(),
		[token](const DialogueVariableDefinition &definition)
	{
		return definition.token == token;
	});

	return (iter != DialogueVariableDefinitions.end()) ? &(*iter) : nullptr;
}

const DialogueVariableDefinition &DialogueDefinitionLibrary::getVariableDefinition(DialogueVariableType type)
{
	const size_t index = static_cast<size_t>(type);
	DebugAssertIndex(DialogueVariableDefinitions, index);
	return DialogueVariableDefinitions[index];
}
