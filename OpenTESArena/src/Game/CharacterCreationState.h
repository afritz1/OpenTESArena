#ifndef CHARACTER_CREATION_STATE_H
#define CHARACTER_CREATION_STATE_H

#include <array>
#include <string_view>
#include "../Math/Random.h"

class CharacterCreationState
{
public:
	static constexpr int MAX_NAME_LENGTH = 25;
private:
	static constexpr int NO_INDEX = -1;

	std::array<char, MAX_NAME_LENGTH + 1> name;
	int classDefID;
	int raceIndex;
	int portraitIndex;
	bool male;
	int strength;
	int intelligence;
	int willpower;
	int agility;
	int speed;
	int endurance;
	int personality;
	int luck;
	Random random; // Convenience random for ease of use.

	int rollPrimaryAttribute(int base);
public:
	CharacterCreationState();

	const std::string_view getName() const;
	int getClassDefID() const;
	int getRaceIndex() const;
	int getStrength() const;
	int getIntelligence() const;
	int getWillpower() const;
	int getAgility() const;
	int getSpeed() const;
	int getEndurance() const;
	int getPersonality() const;
	int getLuck() const;
	int getPortraitIndex() const;
	bool isMale() const;

	void setName(const char *name);
	void setClassDefID(int id);
	void setRaceIndex(int index);
	void setPortraitIndex(int index);
	void setGender(bool male);
	void setPrimaryAttributes();

	void clear();
};

#endif
