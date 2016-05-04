#ifndef CHARACTER_CLASS_PARSER_H
#define CHARACTER_CLASS_PARSER_H

#include <memory>
#include <vector>

class CharacterClass;

class CharacterClassParser
{
private:
	static const std::string PATH;
	static const std::string FILENAME;

	CharacterClassParser() = delete;
	CharacterClassParser(const CharacterClassParser&) = delete;
	~CharacterClassParser() = delete;

	// Splits a string by spaces. Intended for whitespace separated lists.
	static std::vector<std::string> split(const std::string &line);
public:
	static std::vector<std::unique_ptr<CharacterClass>> parse();
};

#endif
