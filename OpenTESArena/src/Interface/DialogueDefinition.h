#ifndef DIALOGUE_DEFINITION_H
#define DIALOGUE_DEFINITION_H

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

#include "components/utilities/Enum.h"

enum class DialogueSpeakerType
{
	Citizen,
	StaticNpc,
	Guard,
	Shopkeeper,
	GuildMember,
	Noble,
	QuestGiver,
	MainQuestNpc,
	Unknown
};

enum class DialogueTopicType
{
	Greeting,
	Rumor,
	Direction,
	Quest,
	Event,
	Trade,
	Joke,
	Unknown
};

enum class DialogueVariableType
{
	ActiveQuestReward,
	NpcNameAdjective,
	UnknownAdn,
	NpcNameAdverb,
	UnknownAmn,
	UnknownAn,
	ActiveQuestDungeonType,
	CurrentArtifactName,
	ArtifactProvinceName,
	UnknownArc,
	ActiveQuestBonusReward,
	CurrentMainQuestCity,
	CurrentMainQuestDungeon,
	CurrentCityOrProvince,
	NearestSettlementName,
	CurrentMainQuestProvince,
	CurrentCityType,
	ActiveQuestDeadlineDate,
	ActiveQuestDeadlineDays,
	NpcDescription,
	DirectionToDialoguePoint,
	UnknownDit,
	UnknownDoc,
	ActiveQuestGiverDescription,
	UnknownDu,
	DialogueNpcLocalNameFirstWord,
	UnknownEf,
	UnknownEn,
	ActiveQuestGiverFirstName,
	UnknownFn,
	SpeakerPronounSubject,
	SpeakerPronounObject,
	SpeakerPronounPossessive,
	UnknownHp,
	PlayerHomeCity,
	NearestHolidayString,
	RandomJoke,
	CurrentProvince,
	NobleQuestItemName,
	ActiveQuestMonsterName,
	ArtifactMapProvinceName,
	UnknownMqt,
	ActiveQuestMonsterType,
	UnknownN,
	ArtifactPrice,
	ActiveQuestLocalName,
	UnknownNcl,
	ActiveQuestLocalAlias,
	NearestHolidayName,
	NearestHolidayDate,
	EmptyString,
	ActiveQuestLocalAliasAlt,
	UnknownNt,
	RivalFactionName,
	UnknownOap,
	UnknownOc,
	ActiveQuestObjectiveItem,
	UnknownOpp,
	Oath,
	PlayerFullName,
	PlayerFirstName,
	NpcNamePrefix,
	ActiveQuestCityName,
	ActiveQuestTypeDescription,
	ActiveQuestOpponentName,
	UnknownQmn,
	ActiveQuestRelation,
	PlayerRaceName,
	RandomCityStateInProvince,
	RulerFirstName,
	RandomProvinceName,
	UnknownSn,
	StateOfWarOrPeace,
	NpcNameSuffix,
	RulerTitle,
	TargetDungeonName,
	TargetCityName,
	UnknownTem,
	TargetFactionName,
	UnknownTi,
	TargetLocationName,
	ActiveQuestGiverTitle,
	ActiveQuestNpcClass,
	Count
};

enum class DialogueVariableAttribute : uint32_t
{
	None = 0,
	NormalizedWhitespace = 1 << 0,
	RandomizedSelection = 1 << 1,
	SetsSpeakerGender = 1 << 2
};
AllowEnumFlags(DialogueVariableAttribute);
using DialogueVariableAttributes = EnumFlags<DialogueVariableAttribute>;

struct DialogueVariableDefinition
{
	DialogueVariableType type;
	std::string_view token;
	std::string_view description;
	DialogueVariableAttributes attributes;
};

struct DialogueBlockDefinition
{
	DialogueSpeakerType speakerType;
	DialogueTopicType topicType;
	int templateDatKey;
	std::optional<char> letter;
};

class DialogueDefinitionLibrary
{
public:
	static std::span<const DialogueVariableDefinition> getVariableDefinitions();
	static const DialogueVariableDefinition *tryGetVariableDefinitionByToken(std::string_view token);
	static const DialogueVariableDefinition &getVariableDefinition(DialogueVariableType type);
};

#endif
