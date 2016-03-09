#ifndef CONSUMABLE_FOOD_H
#define CONSUMABLE_FOOD_H

#include <string>

#include "Consumable.h"
#include "FoodType.h"

class Food : public Consumable
{
private:
	FoodType foodType;
public:
	Food(FoodType foodType);
	virtual ~Food();

	virtual double getWeight() const override;
	virtual int getGoldValue() const override;
	virtual std::string getDisplayName() const override;

	const FoodType &getFoodType() const;
	virtual ConsumableType getConsumableType() const override;
	virtual std::string typeToString() const override;
};

#endif