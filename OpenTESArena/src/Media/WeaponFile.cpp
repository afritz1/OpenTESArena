#include <unordered_map>

#include "TextureFile.h"
#include "TextureName.h"
#include "WeaponFile.h"
#include "../Items/WeaponType.h"

namespace std
{
	// Hash specializations, since GCC doesn't support enum classes used as keys
	// in unordered_maps before C++14.
	template <>
	struct hash<WeaponType>
	{
		size_t operator()(const WeaponType &x) const
		{
			return static_cast<size_t>(x);
		}
	};
}

namespace
{
	const std::unordered_map<WeaponType, TextureName> WeaponTextureNames =
	{
		{ WeaponType::BattleAxe, TextureName::AxeAnimation },
		{ WeaponType::Broadsword, TextureName::SwordAnimation },
		{ WeaponType::Claymore, TextureName::SwordAnimation },
		{ WeaponType::Dagger, TextureName::SwordAnimation },
		{ WeaponType::DaiKatana, TextureName::SwordAnimation },
		{ WeaponType::Fists, TextureName::FistsAnimation },
		{ WeaponType::Flail, TextureName::FlailAnimation },
		{ WeaponType::Katana, TextureName::SwordAnimation },
		{ WeaponType::LongBow, TextureName::ArrowsAnimation },
		{ WeaponType::Longsword, TextureName::SwordAnimation },
		{ WeaponType::Mace, TextureName::MaceAnimation },
		{ WeaponType::Saber, TextureName::SwordAnimation },
		{ WeaponType::ShortBow, TextureName::ArrowsAnimation },
		{ WeaponType::Shortsword, TextureName::SwordAnimation },
		{ WeaponType::Staff, TextureName::StaffAnimation },
		{ WeaponType::Tanto, TextureName::SwordAnimation },
		{ WeaponType::Wakizashi, TextureName::SwordAnimation },
		{ WeaponType::WarAxe, TextureName::AxeAnimation },
		{ WeaponType::Warhammer, TextureName::HammerAnimation }
	};
}

const std::string &WeaponFile::fromName(WeaponType weaponType)
{
	const TextureName textureName = WeaponTextureNames.at(weaponType);
	const std::string &filename = TextureFile::fromName(textureName);
	return filename;
}
