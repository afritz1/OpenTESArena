#ifndef KEY_VALUE_FILE_H
#define KEY_VALUE_FILE_H

#include <string>
#include <string_view>
#include <vector>

// A key-value map reads in a key-value pair file that uses the "key = value" syntax.
// Pairs are associated with a section and can be listed in the file in any order.
// Comments can be anywhere in a line.

class KeyValueFile
{
public:
	class Section
	{
	private:
		std::vector<std::pair<std::string, std::string>> pairs;

		bool tryGetValue(const std::string &key, std::string_view &value) const;
	public:
		int getPairCount() const;
		const std::pair<std::string, std::string> &getPair(int index) const;

		bool tryGetBoolean(const std::string &key, bool &value) const;
		bool tryGetInteger(const std::string &key, int &value) const;
		bool tryGetDouble(const std::string &key, double &value) const;
		bool tryGetString(const std::string &key, std::string_view &value) const;

		void add(std::string &&key, std::string &&value);
		void clear();
	};
private:
	std::vector<std::pair<std::string, Section>> sections;
public:
	// These are public so other code can use them (i.e., for options writing).
	static const char COMMENT;
	static const char PAIR_SEPARATOR;
	static const char SECTION_FRONT;
	static const char SECTION_BACK;

	bool init(const char *filename);

	int getSectionCount() const;
	const std::string &getSectionName(int index) const;
	const Section &getSection(int index) const;
	const Section *getSectionByName(const std::string &name) const;
};

#endif
