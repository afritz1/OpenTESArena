#ifndef CHARACTER_CLASS_PARSER_H
#define CHARACTER_CLASS_PARSER_H

#include <memory>
#include <string>
#include <vector>

// This class will need to switch from parsing "classes.txt" to the original
// Arena data files.

class CharacterClass;

class CharacterClassParser
{
private:
	static const std::string PATH;
	static const std::string FILENAME;

	CharacterClassParser() = delete;
	CharacterClassParser(const CharacterClassParser&) = delete;
	~CharacterClassParser() = delete;
public:
	static std::vector<std::unique_ptr<CharacterClass>> parse();
};

#endif
