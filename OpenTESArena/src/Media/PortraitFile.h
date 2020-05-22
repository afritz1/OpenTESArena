#ifndef PORTRAIT_FILE_H
#define PORTRAIT_FILE_H

#include <string>

#include "../Math/Vector2.h"

// This namespace gets the filenames for images relevant to character portraits.

namespace PortraitFile
{
	// Gets the heads filename for a given gender, race, and whether the heads
	// are for the character sheet (not trimmed) or the game interface (trimmed).
	std::string getHeads(bool male, int raceID, bool trimmed);

	// Gets the unclothed character background filename for a given gender and race.
	std::string getBody(bool male, int raceID);

	// Gets the shirt image filename for a given gender and magic affinity.
	const std::string &getShirt(bool male, bool magic);

	// Gets the pants image filename for a given gender.
	const std::string &getPants(bool male);

	// Gets the equipment images filename for a given gender.
	const std::string &getEquipment(bool male);

	// Gets the pixel offset for drawing a shirt texture in the equipment screen.
	Int2 getShirtOffset(bool male, bool magic);

	// Gets the pixel offset for drawing a pants texture in the equipment screen.
	Int2 getPantsOffset(bool male);
}

#endif
