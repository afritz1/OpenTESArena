#include "ArenaWeaponUtils.h"
#include "../Assets/ExeData.h"

#include "components/debug/Debug.h"
#include "components/utilities/Span.h"
#include "components/utilities/String.h"

std::string ArenaWeaponUtils::getAnimationFilename(int weaponID, const ExeData & exeData)
{
	int filenameIndex = ArenaWeaponUtils::FistsFilenameIndex;
	if (weaponID >= 0)
	{
		DebugAssertIndex(ArenaWeaponUtils::FilenameIndices, weaponID);
		filenameIndex = ArenaWeaponUtils::FilenameIndices[weaponID];
	}

	const Span<const std::string> animFilenames = exeData.equipment.weaponAnimationFilenames;
	const std::string &filename = animFilenames[filenameIndex];
	return String::toUppercase(filename);
}
