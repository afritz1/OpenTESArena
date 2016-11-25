#ifndef WEAPON_FILE_H
#define WEAPON_FILE_H

#include <string>

// Static class for accessing Arena's weapon animation filenames.

enum class WeaponType;

class WeaponFile
{
	WeaponFile() = delete;
	WeaponFile(const WeaponFile&) = delete;
	~WeaponFile() = delete;
public:
	static const std::string &fromName(WeaponType weaponType);
};

#endif
