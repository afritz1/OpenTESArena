#include "WorldData.h"

WorldData::WorldData()
{

}

WorldData::~WorldData()
{

}

const std::string &WorldData::getMifName() const
{
	return this->mifName;
}

const std::vector<Double2> &WorldData::getStartPoints() const
{
	return this->startPoints;
}
