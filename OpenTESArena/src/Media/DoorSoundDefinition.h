#ifndef DOOR_SOUND_DEFINITION_H
#define DOOR_SOUND_DEFINITION_H

#include <string>

class DoorSoundDefinition
{
public:
	// Each door has a certain behavior for playing sounds when closing.
	enum class CloseType
	{
		OnClosed,
		OnClosing
	};

	struct OpenDef
	{
		std::string soundFilename;

		void init(std::string &&soundFilename);
	};

	struct CloseDef
	{
		CloseType closeType;
		std::string soundFilename;

		void init(CloseType closeType, std::string &&soundFilename);
	};
private:
	OpenDef open;
	CloseDef close;
public:
	void init(std::string &&openSoundFilename, CloseType closeType, std::string &&closeSoundFilename);

	const OpenDef &getOpen() const;
	const CloseDef &getClose() const;
};

#endif
