#ifndef GUILD_H
#define GUILD_H

#include <string>

enum class GuildName;

class Guild
{
private:
	GuildName guildName;
public:
	Guild(GuildName guildName);
	~Guild();

	GuildName getGuildName() const;
	std::string toString() const;
};

#endif
