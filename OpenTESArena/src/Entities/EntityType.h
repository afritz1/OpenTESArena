#ifndef ENTITY_TYPE_H
#define ENTITY_TYPE_H

// An entity type determines the behavior of an entity when time is ticking or when 
// they are interacted with.

// Containers are: treasure chests, piles, corpses, etc..
// Doodads are: furniture, trees, light posts, junk, staff pieces, etc..
// Doors are: swinging doors, raising portcullises; not voxels.
// Non-players are: anything with an AI (an NPC).
// Players are: the player only.
// Projectiles are: flying objects or spells that disappear on hit.
// Transitions are: a (sprite) entrance to somewhere, like a den; not voxels.

// Velocity:
// The player, non-players, and projectiles all have a 3D velocity, but only the 
// player and non-players also have a 3D direction they are facing.

// Activation:
// Most entities can be "activated"; that is, friendly non-players can be talked to,
// doors can be toggled, and transitions can be entered. Maybe also doodads, but 
// that seems like extra. I don't see there being any levers like in Daggerfall.

// Thieving:
// What about pickpocketing? It was hardly implemented in Arena. Let's leave it
// the same as it was, only with a chance of being caught now. This removes the
// need to have non-players also behave like containers. Thieving is just too big 
// of a feature to add onto the project.

// Lights:
// Doodads, the player, non-players, and projectiles can have a light. Maybe also
// containers, but that seems like extra. No "glowing treasure chests".

enum class EntityType
{
	Container,
	Doodad,	
	Door,
	NonPlayer,
	Player,
	Projectile,
	Transition
};

#endif
