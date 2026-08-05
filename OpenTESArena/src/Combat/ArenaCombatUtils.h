#pragma once

struct ArenaRandom;

namespace ArenaCombatUtils
{
	bool isMeleeHitSuccessful(int attackerLevel, int attackerRaceID, int attackerHitBonus, int attackerLuckBonus,
		int defenderLevel, int defenderClassID, int defenderDefenseBonus, int defenderLuckBonus, ArenaRandom &random);
}
