#include <cassert>

#include "Food.h"

#include "ConsumableType.h"

Food::Food(const std::string &displayName, double weight)
{
	this->displayName = displayName;
	this->weight = weight;

	assert(this->displayName == displayName);
	assert(this->weight == weight);
	assert(this->weight >= 0.0);
}

Food::~Food()
{

}

double Food::getWeight() const
{
	return this->weight;
}

int Food::getGoldValue() const
{
	int baseValue = 0;
	return baseValue;
}

std::string Food::getDisplayName() const
{
	return this->displayName;
}

ConsumableType Food::getConsumableType() const
{
	return ConsumableType::Food;
}

std::string Food::typeToString() const
{
	return "Food";
}
