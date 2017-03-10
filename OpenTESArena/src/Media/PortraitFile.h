#ifndef PORTRAIT_FILE_H
#define PORTRAIT_FILE_H

#include <string>

#include "../Math/Vector2.h"

// This static class gets the filenames for images relevant to character portraits.

enum class GenderName;

class PortraitFile
{
private:
	PortraitFile() = delete;
	PortraitFile(const PortraitFile&) = delete;
	~PortraitFile() = delete;
public:
	// Gets the heads filename for a given gender, race, and whether the heads
	// are for the character sheet (not trimmed) or the game interface (trimmed).
	static std::string getHeads(GenderName gender, int raceID, bool trimmed);

	// Gets the unclothed character background filename for a given gender and race.
	static std::string getBody(GenderName gender, int raceID);

	// Gets the shirt image filename for a given gender and magic affinity.
	static const std::string &getShirt(GenderName gender, bool magic);

	// Gets the pants image filename for a given gender.
	static const std::string &getPants(GenderName gender);

	// Gets the equipment images filename for a given gender.
	static const std::string &getEquipment(GenderName gender);

	// Gets the pixel offset for drawing a shirt texture in the equipment screen.
	static const Int2 &getShirtOffset(GenderName gender, bool magic);

	// Gets the pixel offset for drawing a pants texture in the equipment screen.
	static const Int2 &getPantsOffset(GenderName gender);
};

#endif
