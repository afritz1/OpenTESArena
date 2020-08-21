#ifndef DYNAMIC_ENTITY_TYPE_H
#define DYNAMIC_ENTITY_TYPE_H

enum class DynamicEntityType
{
	Citizen, // Wanders around and can talk to player.
	Creature, // Enemy NPC, anywhere from a rat to the final boss.
	Projectile // Flying objects like arrows or spells.
};

#endif
