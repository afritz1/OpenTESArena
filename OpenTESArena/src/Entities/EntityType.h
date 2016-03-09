#ifndef ENTITY_TYPE_H
#define ENTITY_TYPE_H

// Containers are: treasure chests, piles, corpses, etc..
// Doodads are: furniture, trees, light posts, etc..
// Doors are: swinging doors, portcullises, not voxels.
// Non-players are: anything with an AI.
// Players are: the player only.
// Spells are: any spell projectiles flying through the air.
// Transitions are: a (sprite) entrance to somewhere, like a den; not voxels.

enum class EntityType
{
	Container,
	Doodad,	
	Door,
	NonPlayer,
	Player,
	Spell,
	Transition
};

#endif