#ifndef TEXT_CINEMATIC_DEFINITION_H
#define TEXT_CINEMATIC_DEFINITION_H

#include <string>
#include <vector>

#include "../Utilities/Color.h"

enum class TextCinematicDefinitionType
{
	Death,
	MainQuest
};

enum class DeathTextCinematicType
{
	Good,
	Bad
};

struct DeathTextCinematicDefinition
{
	DeathTextCinematicType type;

	void init(DeathTextCinematicType type);
};

struct MainQuestTextCinematicDefinition
{
	int progress; // Current point in main quest.

	void init(int progress);
};

// Intended for text cinematics with speech.
struct TextCinematicDefinition
{
	TextCinematicDefinitionType type;
	int templateDatKey; // Maps to TEMPLATE.DAT text and used with .VOC filenames.
	std::string animFilename;
	Color fontColor;
	// @todo: maybe some isFloppyVersion bool to support floppy/CD endings.

	union
	{
		DeathTextCinematicDefinition death;
		MainQuestTextCinematicDefinition mainQuest;
	};

	void initDeath(int templateDatKey, const std::string &animFilename, const Color &fontColor, DeathTextCinematicType type);
	void initMainQuest(int templateDatKey, const std::string &animFilename, const Color &fontColor, int progress);
};

#endif
