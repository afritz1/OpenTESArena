#ifndef ARMOR_MATERIAL_TYPE_H
#define ARMOR_MATERIAL_TYPE_H

// Both leather and chain are concrete types, but plate is abstract because it 
// requires a metal type pairing.

enum class ArmorMaterialType
{
	Leather,
	Chain,
	Plate
};

#endif