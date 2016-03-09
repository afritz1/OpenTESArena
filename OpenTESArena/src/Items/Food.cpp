#include <cassert>
#include <map>

#include "Food.h"
#include "ConsumableType.h"

const auto FoodDisplayNames = std::map<FoodType, std::string>
{
	// Typical foods.
	{ FoodType::Apple, "Apple" },
	{ FoodType::ApplePie, "Apple Pie" },
	{ FoodType::Blueberry, "Blueberry" },
	{ FoodType::BlueberryPie, "Blueberry Pie" },
	{ FoodType::Bread, "Bread" },
	{ FoodType::Cabbage, "Cabbage" },
	{ FoodType::Carrot, "Carrot" },
	{ FoodType::Cheese, "Cheese" },
	{ FoodType::Fish, "Fish" },
	{ FoodType::Watermelon, "Watermelon" },

	// Creature meat.
	{ FoodType::GoblinMeat, "Goblin Meat" },
	{ FoodType::RatMeat, "Rat Meat" },
	{ FoodType::WolfMeat, "Wolf Meat" }
};

// These values are made up.
const auto FoodWeights = std::map<FoodType, double>
{
	// Typical foods.
	{ FoodType::Apple, 0.15 },
	{ FoodType::ApplePie, 1.0 },
	{ FoodType::Blueberry, 0.02 },
	{ FoodType::BlueberryPie, 1.0 },
	{ FoodType::Bread, 0.5 },
	{ FoodType::Cabbage, 0.25 },
	{ FoodType::Carrot, 0.10 },
	{ FoodType::Cheese, 0.25 },
	{ FoodType::Fish, 0.25 },
	{ FoodType::Watermelon, 1.0 },

	// Creature meat.
	{ FoodType::GoblinMeat, 0.50 },
	{ FoodType::RatMeat, 0.50 },
	{ FoodType::WolfMeat, 0.50 }
};

// These values are made up.
const auto FoodGoldValues = std::map<FoodType, int>
{
	// Typical foods.
	{ FoodType::Apple, 3 },
	{ FoodType::ApplePie, 25 },
	{ FoodType::Blueberry, 1 },
	{ FoodType::BlueberryPie, 25 },
	{ FoodType::Bread, 15 },
	{ FoodType::Cabbage, 5 },
	{ FoodType::Carrot, 2 },
	{ FoodType::Cheese, 8 },
	{ FoodType::Fish, 6 },
	{ FoodType::Watermelon, 10 },

	// Creature meat.
	{ FoodType::GoblinMeat, 4 },
	{ FoodType::RatMeat, 2 },
	{ FoodType::WolfMeat, 6 }
};

Food::Food(FoodType foodType)
{
	this->foodType = foodType;
}

Food::~Food()
{

}

double Food::getWeight() const
{
	auto weight = FoodWeights.at(this->getFoodType());
	assert(weight >= 0.0);
	return weight;
}

int Food::getGoldValue() const
{
	int baseValue = FoodGoldValues.at(this->getFoodType());
	return baseValue;
}

std::string Food::getDisplayName() const
{
	auto displayName = FoodDisplayNames.at(this->getFoodType());
	assert(displayName.size() > 0);
	return displayName;
}

const FoodType &Food::getFoodType() const
{
	return this->foodType;
}

ConsumableType Food::getConsumableType() const
{
	return ConsumableType::Food;
}

std::string Food::typeToString() const
{
	return "Food";
}
