#ifndef METALLIC_H
#define METALLIC_H

#include <memory>

// Inherit this class if the item type has any kind of metal associated with it.

class Metal;

enum class MetalType;

class Metallic
{
private:
	std::unique_ptr<Metal> metal;
public:
	Metallic(MetalType metalType);
	~Metallic();

	const Metal &getMetal() const;
};

#endif
