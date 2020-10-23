#ifndef TRIGGER_DEFINITION_H
#define TRIGGER_DEFINITION_H

#include <string>

#include "VoxelUtils.h"

// Can have a sound and/or text definition.

class TriggerDefinition
{
public:
	class SoundDef
	{
	private:
		std::string filename;
	public:
		void init(std::string &&filename);

		const std::string &getFilename() const;
	};

	class TextDef
	{
	private:
		std::string text;
		bool displayedOnce;
	public:
		void init(std::string &&text, bool displayedOnce);

		const std::string &getText() const;
		bool isDisplayedOnce() const;
	};
private:
	SoundDef sound;
	TextDef text;
	SNInt x;
	int y;
	WEInt z;
public:
	TriggerDefinition();

	void init(SNInt x, int y, WEInt z);

	SNInt getX() const;
	int getY() const;
	WEInt getZ() const;
	const SoundDef &getSoundDef() const;
	const TextDef &getTextDef() const;

	void setSoundDef(std::string &&filename);
	void setTextDef(std::string &&text, bool displayedOnce);
};

#endif
