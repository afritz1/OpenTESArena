#ifndef ENTITY_TYPE_H
#define ENTITY_TYPE_H

// An entity type determines the behavior of an entity when time is ticking or when 
// they are interacted with.

// Non-players: an NPC or creature.
// Doodads: furniture, trees, street lights, junk, staff pieces, etc..
// Containers: treasure chests, piles, etc..
// Projectiles: flying objects or spells.
// Transitions: a sprite entrance to somewhere, like a den.

// Some entities can be activated; that is, friendly non-players can be talked to,
// transitions can be entered, and staff pieces can be picked up.

enum class EntityType
{
	NonPlayer,
	Doodad,
	Container,
	Projectile,
	Transition
};

#endif
