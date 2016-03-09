#ifndef GUILD_H
#define GUILD_H

#include <string>

#include "GuildName.h"

class Guild
{
private:
	GuildName guildName;
public:
	Guild(GuildName guildName);
	~Guild();

	const GuildName &getGuildName() const;
	std::string toString() const;
};

#endif