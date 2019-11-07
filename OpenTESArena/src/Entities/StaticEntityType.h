#ifndef STATIC_ENTITY_TYPE_H
#define STATIC_ENTITY_TYPE_H

// NPC: non-player-character or creature (separate from dynamic NPC).
// Doodads: furniture, trees, street lights, junk, staff pieces, etc..
// Containers: treasure chests, piles, etc..
// Transitions: a sprite entrance to somewhere, like a den.

enum class StaticEntityType
{
	NPC,
	Doodad,
	Container,
	Transition
};

#endif
