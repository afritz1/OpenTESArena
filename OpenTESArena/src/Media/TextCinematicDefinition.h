#ifndef TEXT_CINEMATIC_DEFINITION_H
#define TEXT_CINEMATIC_DEFINITION_H

#include <string>
#include <vector>

#include "Color.h"

// Intended for text cinematics with speech.

class TextCinematicDefinition
{
public:
	enum class Type
	{
		Death,
		MainQuest
	};

	struct DeathDefinition
	{
		enum class Type
		{
			Good,
			Bad
		};

		DeathDefinition::Type type;

		void init(DeathDefinition::Type type);
	};

	struct MainQuestDefinition
	{
		int progress; // Current point in main quest.

		void init(int progress);
	};
private:
	Type type;
	int templateDatKey; // Maps to TEMPLATE.DAT text and used with .VOC filenames.
	std::string animFilename;
	Color fontColor;
	// @todo: maybe some isFloppyVersion bool to support floppy/CD endings.

	union
	{
		DeathDefinition death;
		MainQuestDefinition mainQuest;
	};

	void init(Type type, int templateDatKey, std::string &&animFilename, const Color &fontColor);
public:
	void initDeath(int templateDatKey, std::string &&animFilename, const Color &fontColor,
		DeathDefinition::Type type);
	void initMainQuest(int templateDatKey, std::string &&animFilename, const Color &fontColor,
		int progress);

	Type getType() const;
	int getTemplateDatKey() const;
	const std::string &getAnimationFilename() const;
	const Color &getFontColor() const;

	const DeathDefinition &getDeathDefinition() const;
	const MainQuestDefinition &getMainQuestDefinition() const;
};

#endif
