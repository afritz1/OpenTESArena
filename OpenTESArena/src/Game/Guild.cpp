#include <cassert>
#include <map>

#include "Guild.h"

const auto GuildDisplayNames = std::map<GuildName, std::string>
{
	{ GuildName::Blades, "Blades" },
	{ GuildName::DarkBrotherhood, "Dark Brotherhood" },
	{ GuildName::MagesGuild, "Mage's Guild" },
	{ GuildName::Necromancers, "Necromancers" },
	{ GuildName::ThievesGuild, "Thieves Guild" },
	{ GuildName::Underking, "Underking" },
	{ GuildName::WharfRats, "Wharf Rats" }
};

Guild::Guild(GuildName guildName)
{
	this->guildName = guildName;
}

Guild::~Guild()
{

}

GuildName Guild::getGuildName() const
{
	return this->guildName;
}

std::string Guild::toString() const
{
	auto displayName = GuildDisplayNames.at(this->getGuildName());
	assert(displayName.size() > 0);
	return displayName;
}
