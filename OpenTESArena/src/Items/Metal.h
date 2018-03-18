#ifndef METAL_H
#define METAL_H

#include <string>

enum class MetalType;

class Metal
{
private:
	MetalType metalType;
public:
	Metal(MetalType metalType);

	MetalType getMetalType() const;

	// The modifier is the same but with different signs for armor and damage.
	// If higher (positive) armor rating was better, that would work out nicely.
	int getRatingModifier() const;

	// Stronger metals have higher condition multipliers.
	int getConditionMultiplier() const;

	// Some metals are heavier than others.
	double getWeightMultiplier() const;

	std::string toString() const;
};

#endif
