#include <cassert>
#include <unordered_map>

#include "ArmorMaterial.h"
#include "ArmorMaterialType.h"

namespace std
{
	// Hash specialization, required until GCC 6.1.
	template <>
	struct hash<ArmorMaterialType>
	{
		size_t operator()(const ArmorMaterialType &x) const
		{
			return static_cast<size_t>(x);
		}
	};
}

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
