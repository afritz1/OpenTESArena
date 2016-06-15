#ifndef CHARACTER_RACE_H
#define CHARACTER_RACE_H

#include <string>

enum class CharacterRaceName;
enum class ProvinceName;

class CharacterRace
{
private:
	CharacterRaceName raceName;
public:
	CharacterRace(CharacterRaceName raceName);
	~CharacterRace();

	CharacterRaceName getRaceName() const;
	ProvinceName getHomeProvinceName() const;
	std::string toString() const;
};

#endif
