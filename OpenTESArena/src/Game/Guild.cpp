#include "Guild.h"

Guild::Guild(const std::string &guildName)
	: guildName(guildName)
{
	
}

Guild::~Guild()
{

}

const std::string &Guild::getGuildName() const
{
	return this->guildName;
}
