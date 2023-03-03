#include "TextCinematicDefinition.h"

#include "components/debug/Debug.h"

void TextCinematicDefinition::DeathDefinition::init(DeathDefinition::Type type)
{
	this->type = type;
}

void TextCinematicDefinition::MainQuestDefinition::init(int progress)
{
	this->progress = progress;
}

void TextCinematicDefinition::init(Type type, int templateDatKey, std::string &&animFilename,
	const Color &fontColor)
{
	this->type = type;
	this->templateDatKey = templateDatKey;
	this->animFilename = std::move(animFilename);
	this->fontColor = fontColor;
}

void TextCinematicDefinition::initDeath(int templateDatKey, std::string &&animFilename,
	const Color &fontColor, DeathDefinition::Type type)
{
	this->init(Type::Death, templateDatKey, std::move(animFilename), fontColor);
	this->death.init(type);
}

void TextCinematicDefinition::initMainQuest(int templateDatKey, std::string &&animFilename,
	const Color &fontColor, int progress)
{
	this->init(Type::MainQuest, templateDatKey, std::move(animFilename), fontColor);
	this->mainQuest.init(progress);
}

TextCinematicDefinition::Type TextCinematicDefinition::getType() const
{
	return this->type;
}

int TextCinematicDefinition::getTemplateDatKey() const
{
	return this->templateDatKey;
}

const std::string &TextCinematicDefinition::getAnimationFilename() const
{
	return this->animFilename;
}

const Color &TextCinematicDefinition::getFontColor() const
{
	return this->fontColor;
}

const TextCinematicDefinition::DeathDefinition &TextCinematicDefinition::getDeathDefinition() const
{
	DebugAssert(this->type == Type::Death);
	return this->death;
}

const TextCinematicDefinition::MainQuestDefinition &TextCinematicDefinition::getMainQuestDefinition() const
{
	DebugAssert(this->type == Type::MainQuest);
	return this->mainQuest;
}
