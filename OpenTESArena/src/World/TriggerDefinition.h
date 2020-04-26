#ifndef TRIGGER_DEFINITION_H
#define TRIGGER_DEFINITION_H

#include <string_view>

#include "VoxelUtils.h"

class TriggerDefinition
{
public:
	enum class Type { Sound, Text };

	class SoundDef
	{
	private:
		char name[32];
	public:
		void init(const char *name);

		std::string_view getName() const;
	};

	class TextDef
	{
	private:
		int textID; // ID in map info text.
		bool displayedOnce;
	public:
		void init(int textID, bool displayedOnce);

		int getTextID() const;
		bool isDisplayedOnce() const;
	};
private:
	WEInt x;
	int y;
	SNInt z;
	Type type;

	union
	{
		SoundDef sound;
		TextDef text;
	};

	void init(WEInt x, int y, SNInt z, Type type);
public:
	static TriggerDefinition makeSound(WEInt x, int y, SNInt z, const char *name);
	static TriggerDefinition makeText(WEInt x, int y, SNInt z, int textID, bool displayedOnce);

	WEInt getX() const;
	int getY() const;
	SNInt getZ() const;
	Type getType() const;
	const SoundDef &getSoundDef() const;
	const TextDef &getTextDef() const;
};

#endif
