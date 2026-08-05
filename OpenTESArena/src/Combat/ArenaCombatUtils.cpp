#include <algorithm>

#include "ArenaCombatUtils.h"
#include "../Math/Random.h"
#include "../Stats/ArenaStatUtils.h"
#include "../Stats/CharacterRaceLibrary.h"

bool ArenaCombatUtils::isMeleeHitSuccessful(int attackerLevel, int attackerRaceID, int attackerHitBonus, int attackerLuckBonus,
	int defenderLevel, int defenderClassID, int defenderDefenseBonus, int defenderLuckBonus, ArenaRandom &random)
{
	int racialChanceBonus = 0;
	
	const bool attackerHasRacialBonus = (attackerRaceID == 1) || (attackerRaceID == 3) || (attackerRaceID == 5);
	if (attackerHasRacialBonus)
	{
		racialChanceBonus = ArenaStatUtils::scale100To256(attackerLevel);
	}

	const int defenderArmorSlotIndex = 0; // @todo armor slots
	const int defenderArmorClassValue = 0; // @todo look up in defender's armor	
	int chance1 = 128 + ArenaStatUtils::scale100To256((attackerLevel - defenderLevel) * 5) + (attackerLuckBonus - defenderLuckBonus) + racialChanceBonus + (attackerHitBonus - defenderDefenseBonus) - defenderArmorClassValue;

	const bool defenderHasClassBonus = (defenderClassID == 9) || (defenderClassID == 12);
	if (defenderHasClassBonus)
	{
		chance1 = (chance1 * 8 / 256) * (defenderLevel + 1);
	}

	const int chance2 = ArenaStatUtils::scale100To256(attackerLevel) + 51;
	return random.next(256) < std::max(chance1, chance2);
}
