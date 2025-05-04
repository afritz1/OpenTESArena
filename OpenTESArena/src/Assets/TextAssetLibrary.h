#ifndef TEXT_ASSET_LIBRARY_H
#define TEXT_ASSET_LIBRARY_H

#include <array>
#include <string>
#include <vector>

#include "../Player/CharacterQuestion.h"

#include "components/utilities/BufferView.h"
#include "components/utilities/Singleton.h"

class ArenaRandom;

// Each artifact text file (ARTFACT1.DAT, ARTFACT2.DAT) contains conversation strings
// about artifacts. Supposedly ARTFACT2.DAT is used when the player declines and
// returns to the individual later.
// - The format is like: [[3][3][3][3][3]] ... [[3][3][3][3][3]]
// - Only the first string of barter success is used.
struct ArenaArtifactTavernText
{
	std::array<std::string, 3> greetingStrs, barterSuccessStrs, offerRefusedStrs, barterFailureStrs, counterOfferStrs;
};

using ArenaArtifactTavernTextArray = std::array<ArenaArtifactTavernText, 16>;
using ArenaDungeonTxtEntry = std::pair<std::string, std::string>;
using ArenaNameChunkEntry = std::vector<std::string>;
using ArenaSpellMakerDescriptionArray = std::array<std::string, 43>;

struct ArenaTemplateDatEntry
{
	static constexpr int NO_KEY = -1;
	static constexpr char NO_LETTER = -1;

	// Value after the '#' character, excluding any letter at the end of the line.
	int key;

	// Strings #0000-#0004 and #0014 have a letter to further divide each series
	// by the current season + weather. -1 if unused.
	char letter;

	// Ampersand-separated strings.
	std::vector<std::string> values;
};

// TEMPLATE.DAT stores various strings for in-game text and conversations.
// Strings #0000 through #0004 have three copies in the file, one for each tileset.
class ArenaTemplateDat
{
private:
	// One vector for each tileset. Most entries are independent of the current
	// season/weather.
	std::vector<std::vector<ArenaTemplateDatEntry>> entryLists;
public:
	const ArenaTemplateDatEntry &getEntry(int key) const;
	const ArenaTemplateDatEntry &getEntry(int key, char letter) const;
	const ArenaTemplateDatEntry &getTilesetEntry(int tileset, int key, char letter) const;

	bool init();
};

// Each trade text file (EQUIP.DAT, MUGUILD.DAT, SELLING.DAT, TAVERN.DAT) is an array
// of 75 null-terminated strings. Each function array wraps conversation behaviors
// (introduction, price agreement, etc.). Each personality array wraps personalities.
// Each random array contains three strings for each personality.
// - The format is like: [[3][3][3][3][3]] ... [[3][3][3][3][3]]
struct ArenaTradeText
{
	using RandomArray = std::array<std::string, 3>;
	using PersonalityArray = std::array<RandomArray, 5>;
	using FunctionArray = std::array<PersonalityArray, 5>;
	FunctionArray equipment, magesGuild, selling, tavern;
};

// Contains assets that are generally human-readable.
// @todo: refactor text processing so we never convert '\r' to '\n'. The engine should handle
// carriage returns.
class TextAssetLibrary : public Singleton<TextAssetLibrary>
{
public:
	ArenaArtifactTavernTextArray artifactTavernText1, artifactTavernText2;
	std::vector<ArenaDungeonTxtEntry> dungeonTxt;
	std::vector<ArenaNameChunkEntry> nameChunks;
	std::vector<CharacterQuestion> questionTxt;
	ArenaSpellMakerDescriptionArray spellMakerDescriptions; // From SPELLMKR.TXT.
	ArenaTemplateDat templateDat;
	ArenaTradeText tradeText;
private:
	// Gets the artifact text used in tavern conversations. Loads ARTFACT1.DAT and ARTFACT2.DAT.
	bool initArtifactText();

	// Gets all the main quest dungeon names paired with their description. These are just the dungeons with a unique icon
	// on the world map, not the lesser dungeons.
	bool initDungeonTxt();

	// Loads NAMECHNK.DAT into a jagged list of name chunks.
	bool initNameChunks();

	// Gets all the character creation questions in QUESTION.TXT.
	bool initQuestionTxt();

	// Gets the list of spell maker description strings. Loads SPELLMKR.TXT.
	bool initSpellMakerDescriptions();

	// Gets the TEMPLATE.DAT data for accessing strings by their ID and optional letter.
	bool initTemplateDat();

	// Gets the trade text object for trade conversations. Loads EQUIP.DAT, MUGUILD.DAT, SELLING.DAT, and TAVERN.DAT.
	bool initTradeText();
public:
	bool init();

	// Creates a random NPC name from the given parameters.
	std::string generateNpcName(int raceID, bool isMale, ArenaRandom &random) const;
};

#endif
