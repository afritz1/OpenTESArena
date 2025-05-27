#include "TextCinematicDefinition.h"

#include "components/debug/Debug.h"

void DeathTextCinematicDefinition::init(DeathTextCinematicType type)
{
	this->type = type;
}

void MainQuestTextCinematicDefinition::init(int progress)
{
	this->progress = progress;
}

void TextCinematicDefinition::initDeath(int templateDatKey, const std::string &animFilename, const Color &fontColor, DeathTextCinematicType type)
{
	this->type = TextCinematicDefinitionType::Death;
	this->templateDatKey = templateDatKey;
	this->animFilename = animFilename;
	this->fontColor = fontColor;
	this->death.init(type);
}

void TextCinematicDefinition::initMainQuest(int templateDatKey, const std::string &animFilename, const Color &fontColor, int progress)
{
	this->type = TextCinematicDefinitionType::MainQuest;
	this->templateDatKey = templateDatKey;
	this->animFilename = animFilename;
	this->fontColor = fontColor;
	this->mainQuest.init(progress);
}
