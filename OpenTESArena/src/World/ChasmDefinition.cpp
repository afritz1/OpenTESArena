#include "ArenaChasmUtils.h"
#include "ChasmDefinition.h"

ChasmDefinition::ChasmDefinition()
{
	this->allowsSwimming = false;
	this->isDamaging = false;
}

void ChasmDefinition::initClassic(ArenaTypes::ChasmType chasmType)
{
	this->allowsSwimming = ArenaChasmUtils::allowsSwimming(chasmType);
	this->isDamaging = ArenaChasmUtils::isDamaging(chasmType);
}
