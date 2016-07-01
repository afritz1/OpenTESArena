#ifndef PORTRAIT_FILE_H
#define PORTRAIT_FILE_H

#include <string>
#include <vector>

// This static class gets the filenames for character portraits.

// The character backgrounds are available now. Once the original faces can be 
// used also, then this class should be redesigned to better fit that.

enum class CharacterGenderName;
enum class CharacterRaceName;

class PortraitFile
{
private:
	static const std::string PATH;

	PortraitFile() = delete;
	PortraitFile(const PortraitFile&) = delete;
	~PortraitFile() = delete;
public:
	// Returns a set of filenames which point to a series of portraits.
	static std::vector<std::string> getGroup(CharacterGenderName gender, 
		CharacterRaceName race, bool isMagic);
};

#endif
