#ifndef CHARACTER_RACE_H
#define CHARACTER_RACE_H

#include <string>

#include "CharacterRaceName.h"

enum class ProvinceName;

class CharacterRace
{
private:
	CharacterRaceName raceName;
public:
	CharacterRace(CharacterRaceName raceName);
	~CharacterRace();

	const CharacterRaceName &getRaceName() const;
	std::string toString() const;
	ProvinceName getHomeProvinceName() const;
};

#endif
