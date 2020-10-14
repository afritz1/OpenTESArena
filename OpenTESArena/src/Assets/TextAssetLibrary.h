#ifndef TEXT_ASSET_LIBRARY_H
#define TEXT_ASSET_LIBRARY_H

#include <array>
#include <string>
#include <vector>

#include "../Game/CharacterQuestion.h"

// Contains assets that are generally human-readable.

// @todo: refactor text processing so we never convert '\r' to '\n'. The engine should handle
// carriage returns.

class ArenaRandom;

class TextAssetLibrary
{
public:
	// Each artifact text file (ARTFACT1.DAT, ARTFACT2.DAT) contains conversation strings
	// about artifacts. Supposedly ARTFACT2.DAT is used when the player declines and
	// returns to the individual later.
	// - The format is like: [[3][3][3][3][3]] ... [[3][3][3][3][3]]
	// - Only the first string of barter success is used.
	struct ArtifactTavernText
	{
		std::array<std::string, 3> greetingStrs, barterSuccessStrs,
			offerRefusedStrs, barterFailureStrs, counterOfferStrs;
	};

	using ArtifactTavernTextArray = std::array<ArtifactTavernText, 16>;
	using DungeonTxtEntry = std::pair<std::string, std::string>;
	using NameChunkEntry = std::vector<std::string>;
	using SpellMakerDescriptionArray = std::array<std::string, 43>;

	// TEMPLATE.DAT stores various strings for in-game text and conversations.
	// Strings #0000 through #0004 have three copies in the file, one for each tileset.
	class TemplateDat
	{
	public:
		struct Entry
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
	private:
		// One vector for each tileset. Most entries are independent of the current
		// season/weather.
		std::vector<std::vector<Entry>> entryLists;
	public:
		const Entry &getEntry(int key) const;
		const Entry &getEntry(int key, char letter) const;
		const Entry &getTilesetEntry(int tileset, int key, char letter) const;

		bool init();
	};

	// Each trade text file (EQUIP.DAT, MUGUILD.DAT, SELLING.DAT, TAVERN.DAT) is an array
	// of 75 null-terminated strings. Each function array wraps conversation behaviors
	// (introduction, price agreement, etc.). Each personality array wraps personalities.
	// Each random array contains three strings for each personality.
	// - The format is like: [[3][3][3][3][3]] ... [[3][3][3][3][3]]
	struct TradeText
	{
		using RandomArray = std::array<std::string, 3>;
		using PersonalityArray = std::array<RandomArray, 5>;
		using FunctionArray = std::array<PersonalityArray, 5>;
		FunctionArray equipment, magesGuild, selling, tavern;
	};
private:
	ArtifactTavernTextArray artifactTavernText1, artifactTavernText2;
	std::vector<DungeonTxtEntry> dungeonTxt;
	std::vector<NameChunkEntry> nameChunks;
	std::vector<CharacterQuestion> questionTxt;
	SpellMakerDescriptionArray spellMakerDescriptions; // From SPELLMKR.TXT.
	TemplateDat templateDat;
	TradeText tradeText;

	// Loads ARTFACT1.DAT and ARTFACT2.DAT.
	bool initArtifactText();

	// Load DUNGEON.TXT and pair each dungeon name with its description.
	bool initDungeonTxt();

	// Loads NAMECHNK.DAT into a jagged list of name chunks.
	bool initNameChunks();

	// Load QUESTION.TXT and separate each question by its number.
	bool initQuestionTxt();

	// Loads SPELLMKR.TXT.
	bool initSpellMakerDescriptions();

	// Loads TEMPLATE.DAT.
	bool initTemplateDat();

	// Loads EQUIP.DAT, MUGUILD.DAT, SELLING.DAT, and TAVERN.DAT.
	bool initTradeText();
public:
	bool init();

	// Gets the artifact text used in tavern conversations.
	const ArtifactTavernTextArray &getArtifactTavernText1() const;
	const ArtifactTavernTextArray &getArtifactTavernText2() const;

	// Returns all of the main quest dungeon names paired with their description. 
	// These are just the dungeons with a unique icon on the world map, not the 
	// lesser dungeons.
	const std::vector<DungeonTxtEntry> &getDungeonTxtDungeons() const;

	// Returns all of the questions in QUESTION.TXT.
	const std::vector<CharacterQuestion> &getQuestionTxtQuestions() const;

	// Gets the list of spell maker description strings.
	const SpellMakerDescriptionArray &getSpellMakerDescriptions() const;

	// Gets the TEMPLATE.DAT data for accessing strings by their ID and optional letter.
	const TemplateDat &getTemplateDat() const;

	// Gets the trade text object for trade conversations.
	const TradeText &getTradeText() const;

	// Creates a random NPC name from the given parameters.
	std::string generateNpcName(int raceID, bool isMale, ArenaRandom &random) const;
};

#endif
