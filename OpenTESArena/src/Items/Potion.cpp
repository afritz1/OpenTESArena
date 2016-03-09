#include <cassert>

#include "Potion.h"
#include "ConsumableType.h"

Potion::Potion()
{

}

Potion::~Potion()
{

}

double Potion::getWeight() const
{
	auto weight = 0.5;
	assert(weight >= 0.0);
	return weight;
}

int Potion::getGoldValue() const
{
	auto baseValue = 10;
	return baseValue;
}

std::string Potion::getDisplayName() const
{
	return this->typeToString() + " of Nothing";
}

ConsumableType Potion::getConsumableType() const
{
	return ConsumableType::Potion;
}

std::string Potion::typeToString() const
{
	return "Potion";
}
