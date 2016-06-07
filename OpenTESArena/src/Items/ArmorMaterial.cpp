#include <cassert>
#include <map>

#include "ArmorMaterial.h"

#include "ArmorMaterialType.h"

const auto ArmorMaterialDisplayNames = std::map<ArmorMaterialType, std::string>
{
	{ ArmorMaterialType::Chain, "Chain" },
	{ ArmorMaterialType::Leather, "Leather" },
	{ ArmorMaterialType::Plate, "Plate" }
};

ArmorMaterial::ArmorMaterial()
{

}

ArmorMaterial::~ArmorMaterial()
{

}

std::string ArmorMaterial::typeToString(ArmorMaterialType materialType)
{
	auto displayName = ArmorMaterialDisplayNames.at(materialType);
	return displayName;
}
