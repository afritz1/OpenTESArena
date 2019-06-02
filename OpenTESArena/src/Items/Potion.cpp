#include "ConsumableType.h"
#include "Potion.h"

double Potion::getWeight() const
{
	double weight = 0.5;
	return weight;
}

int Potion::getGoldValue() const
{
	int baseValue = 0;
	return baseValue;
}

std::string Potion::getDisplayName() const
{
	return this->typeToString();
}

ConsumableType Potion::getConsumableType() const
{
	return ConsumableType::Potion;
}

std::string Potion::typeToString() const
{
	return "Potion";
}
