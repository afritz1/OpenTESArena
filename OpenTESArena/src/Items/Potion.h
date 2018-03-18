#ifndef CONSUMABLE_POTION_H
#define CONSUMABLE_POTION_H

#include <string>

#include "Consumable.h"

// Potions will be constructed by a given effect. Their name will be given by that
// effect, and all potions will have a weight of 0.5 kilograms.

// The effect will be in the Consumable class.

class Potion : public Consumable
{
public:
	Potion() = default;
	virtual ~Potion() = default;

	// All potions are the same weight.
	virtual double getWeight() const override;

	// The gold value is based on the effect.
	virtual int getGoldValue() const override;

	// The display name is based on the effect.
	virtual std::string getDisplayName() const override;

	virtual ConsumableType getConsumableType() const override;
	virtual std::string typeToString() const override;
};

#endif
