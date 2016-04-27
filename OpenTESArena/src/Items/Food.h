#ifndef CONSUMABLE_FOOD_H
#define CONSUMABLE_FOOD_H

#include <string>

#include "Consumable.h"

// I removed the FoodType enumeration because it wasn't necessary. Foods all have the 
// same behavior; an effect upon consumption.

// The effect will be in the Consumable class.

class Food : public Consumable
{
private:
	std::string displayName;
	double weight;
public:
	Food(const std::string &displayName, double weight);
	virtual ~Food();

	virtual double getWeight() const override;

	// Gold value should be based on weight and effects.
	virtual int getGoldValue() const override;

	virtual std::string getDisplayName() const override;

	virtual ConsumableType getConsumableType() const override;
	virtual std::string typeToString() const override;
};

#endif
