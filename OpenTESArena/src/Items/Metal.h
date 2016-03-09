#ifndef METAL_H
#define METAL_H

#include <string>

#include "MetalType.h"

class Metal
{
private:
	MetalType metalType;
public:
	Metal(MetalType metalType);
	~Metal();

	const MetalType &getMetalType() const;
	std::string toString() const;

	// The modifier is the same but with different signs for armor and damage.
	// If higher (positive) armor rating was better, that would work out nicely.
	int getRatingModifier() const;

	// Stronger metals have higher condition multipliers.
	int getConditionMultiplier() const;

	// Some metals are heavier than others.
	double getWeightMultiplier() const;
};

#endif