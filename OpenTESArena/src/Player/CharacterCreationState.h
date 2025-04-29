#ifndef CHARACTER_CREATION_STATE_H
#define CHARACTER_CREATION_STATE_H

#include <array>
#include <optional>
#include <string_view>

#include "../Math/Random.h"
#include "../Stats/PrimaryAttribute.h"

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
	PrimaryAttributes attributes;
	std::array<int, 8> changedPoints;

public:
	CharacterCreationState();

	const std::string_view getName() const;
	int getClassDefID() const;
	int getRaceIndex() const;
	const PrimaryAttributes &getAttributes() const;
	int getPortraitIndex() const;
	bool isMale() const;
	const int* getChangedPoints() const { return changedPoints.data(); }
	int* getChangedPoints() { return changedPoints.data(); }

	void setName(const char *name);
	void setClassDefID(int id);
	void setRaceIndex(int index);
	void setPortraitIndex(int index);
	void setGender(bool male);
	void rollAttributes(Random &random);
	void clearChangedPoints() { changedPoints.fill(0); }

	void clear();
};

#endif
