#include <cassert>
#include <unordered_map>

#include "ArmorMaterial.h"
#include "ArmorMaterialType.h"

const std::unordered_map<ArmorMaterialType, std::string> ArmorMaterialDisplayNames =
{
	{ ArmorMaterialType::Chain, "Chain" },
	{ ArmorMaterialType::Leather, "Leather" },
	{ ArmorMaterialType::Plate, "Plate" }
};

std::string ArmorMaterial::typeToString(ArmorMaterialType materialType)
{
	auto displayName = ArmorMaterialDisplayNames.at(materialType);
	return displayName;
}
