#ifndef PORTRAIT_FILE_H
#define PORTRAIT_FILE_H

#include <string>

#include "../Math/Vector2.h"

// This static class gets the filenames for images relevant to character portraits.

enum class CharacterGenderName;
enum class CharacterRaceName;

class PortraitFile
{
private:
	PortraitFile() = delete;
	PortraitFile(const PortraitFile&) = delete;
	~PortraitFile() = delete;
public:
	// Gets the heads filename for a given gender, race, and whether the heads
	// are for the character sheet (not trimmed) or the game interface (trimmed).
	static const std::string &getHeads(CharacterGenderName gender, 
		CharacterRaceName race, bool trimmed);

	// Gets the unclothed character background filename for a given gender and race.
	static const std::string &getBody(CharacterGenderName gender, CharacterRaceName race);

	// Gets the shirt image filename for a given gender and magic affinity.
	static const std::string &getShirt(CharacterGenderName gender, bool magic);

	// Gets the pants image filename for a given gender.
	static const std::string &getPants(CharacterGenderName gender);

	// Gets the equipment images filename for a given gender.
	static const std::string &getEquipment(CharacterGenderName gender);

	// Gets the pixel offset for drawing a shirt texture in the equipment screen.
	static const Int2 &getShirtOffset(CharacterGenderName gender, bool magic);

	// Gets the pixel offset for drawing a pants texture in the equipment screen.
	static const Int2 &getPantsOffset(CharacterGenderName gender);
};

#endif
