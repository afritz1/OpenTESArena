#ifndef GUILD_H
#define GUILD_H

#include <string>

// A guild is mostly just a name used in various conversations in the game, but
// the Mage's Guild is an actual guild the player can go visit.

// Guilds will be read from the Arena executable eventually.

class Guild
{
private:
	std::string guildName;
public:
	Guild(const std::string &guildName);
	~Guild();

	const std::string &getGuildName() const;
};

#endif
