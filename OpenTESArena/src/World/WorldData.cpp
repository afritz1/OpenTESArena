#include "WorldData.h"

WorldData::WorldData()
{

}

WorldData::~WorldData()
{

}

const std::vector<NewDouble2> &WorldData::getStartPoints() const
{
	return this->startPoints;
}
