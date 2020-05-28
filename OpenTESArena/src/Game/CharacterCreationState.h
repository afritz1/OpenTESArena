#ifndef CHARACTER_CREATION_STATE_H
#define CHARACTER_CREATION_STATE_H

#include <array>
#include <string_view>

class CharacterCreationState
{
public:
	static constexpr int MAX_NAME_LENGTH = 25;
private:
	static constexpr int NO_INDEX = -1;

	std::array<char, MAX_NAME_LENGTH + 1> name;
	// @todo: attributes collection
	int classIndex;
	int raceIndex;
	int portraitIndex;
	bool male;
public:
	CharacterCreationState();

	const std::string_view getName() const;
	int getClassIndex() const;
	int getRaceIndex() const;
	int getPortraitIndex() const;
	bool isMale() const;

	void setName(const char *name);
	void setClassIndex(int index);
	void setRaceIndex(int index);
	void setPortraitIndex(int index);
	void setGender(bool male);

	void clear();
};

#endif
