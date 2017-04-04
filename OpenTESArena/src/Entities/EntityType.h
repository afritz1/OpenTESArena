#ifndef ENTITY_TYPE_H
#define ENTITY_TYPE_H

// An entity type determines the behavior of an entity when time is ticking or when 
// they are interacted with.

// Containers are: treasure chests, piles, corpses, etc..
// Doodads are: furniture, trees, light posts, sign posts, junk, staff pieces, etc..
// Doors are: swinging doors, raising portcullises.
// Non-players are: anything with an AI (an NPC or creature).
// Projectiles are: flying objects or spells.
// Transitions are: a sprite entrance to somewhere, like a den.

// Activation:
// Some entities can be "activated"; that is, friendly non-players can be talked to,
// doors can be toggled, transitions can be entered, and staff pieces can be picked up.

// Lights:
// Doodads, the player, non-players, and projectiles can have a light.

enum class EntityType
{
	Container,
	Doodad,	
	Door,
	NonPlayer,
	Projectile,
	Transition
};

#endif
