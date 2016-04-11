#include <cassert>
#include <map>

#include "CharacterClass.h"

#include "CharacterClassCategory.h"
#include "../Items/ArmorMaterialType.h"
#include "../Items/ShieldType.h"
#include "../Items/WeaponType.h"

const auto CharacterClassNameDisplayNames = std::map<CharacterClassName, std::string>
{
	// Mages.
	{ CharacterClassName::BattleMage, "Battle Mage" },
	{ CharacterClassName::Healer, "Healer" },
	{ CharacterClassName::Mage, "Mage" },
	{ CharacterClassName::Nightblade, "Nightblade" },
	{ CharacterClassName::Sorcerer, "Sorcerer" },
	{ CharacterClassName::Spellsword, "Spellsword" },

	// Thieves.
	{ CharacterClassName::Acrobat, "Acrobat" },
	{ CharacterClassName::Assassin, "Assassin" },
	{ CharacterClassName::Bard, "Bard" },
	{ CharacterClassName::Burglar, "Burglar" },
	{ CharacterClassName::Rogue, "Rogue" },
	{ CharacterClassName::Thief, "Thief" },

	// Warriors.
	{ CharacterClassName::Archer, "Archer" },
	{ CharacterClassName::Barbarian, "Barbarian" },
	{ CharacterClassName::Knight, "Knight" },
	{ CharacterClassName::Monk, "Monk" },
	{ CharacterClassName::Ranger, "Ranger" },
	{ CharacterClassName::Warrior, "Warrior" }
};

const auto CharacterClassNameAllowedArmors = std::map<CharacterClassName, std::vector<ArmorMaterialType>>
{
	// Mages.
	{ CharacterClassName::BattleMage, { ArmorMaterialType::Leather } },
	{ CharacterClassName::Healer, { ArmorMaterialType::Leather, ArmorMaterialType::Chain } },
	{ CharacterClassName::Mage, { } },
	{ CharacterClassName::Nightblade, { ArmorMaterialType::Leather } },
	{ CharacterClassName::Sorcerer, { ArmorMaterialType::Leather, ArmorMaterialType::Chain } },
	{ CharacterClassName::Spellsword, { ArmorMaterialType::Leather, ArmorMaterialType::Chain } },

	// Thieves.
	{ CharacterClassName::Acrobat, { ArmorMaterialType::Leather } },
	{ CharacterClassName::Assassin, { ArmorMaterialType::Leather } },
	{ CharacterClassName::Bard, { ArmorMaterialType::Leather, ArmorMaterialType::Chain } },
	{ CharacterClassName::Burglar, { ArmorMaterialType::Leather } },
	{ CharacterClassName::Rogue, { ArmorMaterialType::Leather, ArmorMaterialType::Chain } },
	{ CharacterClassName::Thief, { ArmorMaterialType::Leather } },

	// Warriors.
	{ CharacterClassName::Archer, { ArmorMaterialType::Leather, ArmorMaterialType::Chain } },
	{ CharacterClassName::Barbarian, { ArmorMaterialType::Leather, ArmorMaterialType::Chain } },
	{ CharacterClassName::Knight, { ArmorMaterialType::Chain, ArmorMaterialType::Plate } },
	{ CharacterClassName::Monk, { } },
	{ CharacterClassName::Ranger, { ArmorMaterialType::Leather, ArmorMaterialType::Chain,
	ArmorMaterialType::Plate } },
	{ CharacterClassName::Warrior, { ArmorMaterialType::Leather, ArmorMaterialType::Chain,
	ArmorMaterialType::Plate } }
};

const auto CharacterClassNameAllowedShields = std::map<CharacterClassName, std::vector<ShieldType>>
{
	// Mages.
	{ CharacterClassName::BattleMage, { ShieldType::Buckler, ShieldType::Round } },
	{ CharacterClassName::Healer, { ShieldType::Buckler, ShieldType::Round } },
	{ CharacterClassName::Mage, { ShieldType::Buckler } },
	{ CharacterClassName::Nightblade, { ShieldType::Buckler } },
	{ CharacterClassName::Sorcerer, { } },
	{ CharacterClassName::Spellsword, { ShieldType::Buckler, ShieldType::Round, ShieldType::Kite } },

	// Thieves.
	{ CharacterClassName::Acrobat, { } },
	{ CharacterClassName::Assassin, { } },
	{ CharacterClassName::Bard, { ShieldType::Buckler, ShieldType::Round, ShieldType::Kite } },
	{ CharacterClassName::Burglar, { } },
	{ CharacterClassName::Rogue, { ShieldType::Buckler, ShieldType::Round, ShieldType::Kite } },
	{ CharacterClassName::Thief, { ShieldType::Buckler } },

	// Warriors.
	{ CharacterClassName::Archer, { } },
	{ CharacterClassName::Barbarian, { ShieldType::Buckler, ShieldType::Round, ShieldType::Kite,
	ShieldType::Tower } },
	{ CharacterClassName::Knight, { ShieldType::Buckler, ShieldType::Round, ShieldType::Kite,
	ShieldType::Tower } },
	{ CharacterClassName::Monk, { } },
	{ CharacterClassName::Ranger, { ShieldType::Buckler, ShieldType::Round, ShieldType::Kite } },
	{ CharacterClassName::Warrior, { ShieldType::Buckler, ShieldType::Round, ShieldType::Kite,
	ShieldType::Tower } }
};

const auto CharacterClassNameAllowedWeapons = std::map<CharacterClassName, std::vector<WeaponType>>
{
	// Mages.
	{ CharacterClassName::BattleMage, { WeaponType::BattleAxe, WeaponType::Broadsword,
	WeaponType::Claymore, WeaponType::Dagger, WeaponType::DaiKatana, WeaponType::Fists,
	WeaponType::Flail, WeaponType::Katana, WeaponType::LongBow, WeaponType::Longsword,
	WeaponType::Mace, WeaponType::Saber, WeaponType::ShortBow, WeaponType::Shortsword,
	WeaponType::Staff, WeaponType::Tanto, WeaponType::Wakizashi, WeaponType::WarAxe,
	WeaponType::Warhammer } },
	{ CharacterClassName::Healer, { WeaponType::Dagger, WeaponType::Staff, WeaponType::Mace,
	WeaponType::Flail } },
	{ CharacterClassName::Mage, { WeaponType::Dagger, WeaponType::Staff } },
	{ CharacterClassName::Nightblade, { WeaponType::Dagger, WeaponType::Staff, WeaponType::ShortBow,
	WeaponType::Shortsword, WeaponType::Saber } },
	{ CharacterClassName::Sorcerer, { WeaponType::BattleAxe, WeaponType::Broadsword,
	WeaponType::Claymore, WeaponType::Dagger, WeaponType::DaiKatana, WeaponType::Fists,
	WeaponType::Flail, WeaponType::Katana, WeaponType::LongBow, WeaponType::Longsword,
	WeaponType::Mace, WeaponType::Saber, WeaponType::ShortBow, WeaponType::Shortsword,
	WeaponType::Staff, WeaponType::Tanto, WeaponType::Wakizashi, WeaponType::WarAxe,
	WeaponType::Warhammer } },
	{ CharacterClassName::Spellsword, { WeaponType::BattleAxe, WeaponType::Broadsword,
	WeaponType::Claymore, WeaponType::Dagger, WeaponType::DaiKatana, WeaponType::Fists,
	WeaponType::Flail, WeaponType::Katana, WeaponType::LongBow, WeaponType::Longsword,
	WeaponType::Mace, WeaponType::Saber, WeaponType::ShortBow, WeaponType::Shortsword,
	WeaponType::Staff, WeaponType::Tanto, WeaponType::Wakizashi, WeaponType::WarAxe,
	WeaponType::Warhammer } },

	// Thieves.
	{ CharacterClassName::Acrobat, { WeaponType::Dagger, WeaponType::Shortsword, WeaponType::Broadsword,
	WeaponType::Tanto, WeaponType::ShortBow } },
	{ CharacterClassName::Assassin, { WeaponType::BattleAxe, WeaponType::Broadsword,
	WeaponType::Claymore, WeaponType::Dagger, WeaponType::DaiKatana, WeaponType::Fists,
	WeaponType::Flail, WeaponType::Katana, WeaponType::LongBow, WeaponType::Longsword,
	WeaponType::Mace, WeaponType::Saber, WeaponType::ShortBow, WeaponType::Shortsword,
	WeaponType::Staff, WeaponType::Tanto, WeaponType::Wakizashi, WeaponType::WarAxe,
	WeaponType::Warhammer } },
	{ CharacterClassName::Bard, { WeaponType::Dagger, WeaponType::Shortsword, WeaponType::Broadsword,
	WeaponType::Saber, WeaponType::Mace, WeaponType::WarAxe, WeaponType::ShortBow } },
	{ CharacterClassName::Burglar, { WeaponType::Dagger, WeaponType::Shortsword, WeaponType::Tanto,
	WeaponType::ShortBow } },
	{ CharacterClassName::Rogue, { WeaponType::BattleAxe, WeaponType::Broadsword,
	WeaponType::Claymore, WeaponType::Dagger, WeaponType::DaiKatana, WeaponType::Fists,
	WeaponType::Flail, WeaponType::Katana, WeaponType::LongBow, WeaponType::Longsword,
	WeaponType::Mace, WeaponType::Saber, WeaponType::ShortBow, WeaponType::Shortsword,
	WeaponType::Staff, WeaponType::Tanto, WeaponType::Wakizashi, WeaponType::WarAxe,
	WeaponType::Warhammer } },
	{ CharacterClassName::Thief, { WeaponType::Dagger, WeaponType::Shortsword, WeaponType::Broadsword,
	WeaponType::Saber, WeaponType::WarAxe, WeaponType::ShortBow } },

	// Warriors.
	{ CharacterClassName::Archer, { WeaponType::BattleAxe, WeaponType::Broadsword,
	WeaponType::Claymore, WeaponType::Dagger, WeaponType::DaiKatana, WeaponType::Fists,
	WeaponType::Flail, WeaponType::Katana, WeaponType::LongBow, WeaponType::Longsword,
	WeaponType::Mace, WeaponType::Saber, WeaponType::ShortBow, WeaponType::Shortsword,
	WeaponType::Staff, WeaponType::Tanto, WeaponType::Wakizashi, WeaponType::WarAxe,
	WeaponType::Warhammer } },
	{ CharacterClassName::Barbarian, { WeaponType::BattleAxe, WeaponType::Broadsword,
	WeaponType::Claymore, WeaponType::Dagger, WeaponType::DaiKatana, WeaponType::Fists,
	WeaponType::Flail, WeaponType::Katana, WeaponType::LongBow, WeaponType::Longsword,
	WeaponType::Mace, WeaponType::Saber, WeaponType::ShortBow, WeaponType::Shortsword,
	WeaponType::Staff, WeaponType::Tanto, WeaponType::Wakizashi, WeaponType::WarAxe,
	WeaponType::Warhammer } },
	{ CharacterClassName::Knight, { WeaponType::BattleAxe, WeaponType::Broadsword,
	WeaponType::Claymore, WeaponType::Dagger, WeaponType::DaiKatana, WeaponType::Fists,
	WeaponType::Flail, WeaponType::Katana, WeaponType::LongBow, WeaponType::Longsword,
	WeaponType::Mace, WeaponType::Saber, WeaponType::ShortBow, WeaponType::Shortsword,
	WeaponType::Staff, WeaponType::Tanto, WeaponType::Wakizashi, WeaponType::WarAxe,
	WeaponType::Warhammer } },
	{ CharacterClassName::Monk, { WeaponType::BattleAxe, WeaponType::Broadsword,
	WeaponType::Claymore, WeaponType::Dagger, WeaponType::DaiKatana, WeaponType::Fists,
	WeaponType::Flail, WeaponType::Katana, WeaponType::LongBow, WeaponType::Longsword,
	WeaponType::Mace, WeaponType::Saber, WeaponType::ShortBow, WeaponType::Shortsword,
	WeaponType::Staff, WeaponType::Tanto, WeaponType::Wakizashi, WeaponType::WarAxe,
	WeaponType::Warhammer } },
	{ CharacterClassName::Ranger, { WeaponType::BattleAxe, WeaponType::Broadsword,
	WeaponType::Claymore, WeaponType::Dagger, WeaponType::DaiKatana, WeaponType::Fists,
	WeaponType::Flail, WeaponType::Katana, WeaponType::LongBow, WeaponType::Longsword,
	WeaponType::Mace, WeaponType::Saber, WeaponType::ShortBow, WeaponType::Shortsword,
	WeaponType::Staff, WeaponType::Tanto, WeaponType::Wakizashi, WeaponType::WarAxe,
	WeaponType::Warhammer } },
	{ CharacterClassName::Warrior, { WeaponType::BattleAxe, WeaponType::Broadsword,
	WeaponType::Claymore, WeaponType::Dagger, WeaponType::DaiKatana, WeaponType::Fists,
	WeaponType::Flail, WeaponType::Katana, WeaponType::LongBow, WeaponType::Longsword,
	WeaponType::Mace, WeaponType::Saber, WeaponType::ShortBow, WeaponType::Shortsword,
	WeaponType::Staff, WeaponType::Tanto, WeaponType::Wakizashi, WeaponType::WarAxe,
	WeaponType::Warhammer } }
};

CharacterClass::CharacterClass(CharacterClassName className)
{
	this->className = className;
}

CharacterClass::~CharacterClass()
{

}

const CharacterClassName &CharacterClass::getClassName() const
{
	return this->className;
}

CharacterClassCategoryName CharacterClass::getClassCategoryName() const
{
	auto category = CharacterClassCategory(className);
	auto categoryName = category.getCategoryName();
	return categoryName;
}

std::string CharacterClass::toString() const
{
	auto displayName = CharacterClassNameDisplayNames.at(this->getClassName());
	assert(displayName.size() > 0);
	return displayName;
}

std::vector<ArmorMaterialType> CharacterClass::getAllowedArmors() const
{
	auto materialTypes = CharacterClassNameAllowedArmors.at(this->getClassName());
	return materialTypes;
}

std::vector<ShieldType> CharacterClass::getAllowedShields() const
{
	auto shieldTypes = CharacterClassNameAllowedShields.at(this->getClassName());
	return shieldTypes;
}

std::vector<WeaponType> CharacterClass::getAllowedWeapons() const
{
	auto weaponTypes = CharacterClassNameAllowedWeapons.at(this->getClassName());
	return weaponTypes;
}
