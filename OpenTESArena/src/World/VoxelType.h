#ifndef VOXEL_TYPE_H
#define VOXEL_TYPE_H

// A unique identifier for each type of voxel. 

// Voxel types are used instead of integer IDs, since it's easier to name specific 
// kinds of voxels. Depending on the climate and season, a certain voxel type might 
// actually be realized as one of many things, such as either dirt, wet dirt, or 
// snow. The intent with voxel types is to have independent generic voxels whose 
// actual appearance can be changed with the flip of a switch somewhere else in the 
// code. If there are certain climates or seasons that don't have a mapping for a 
// certain voxel type then just duplicate a related voxel type's mapping.

// There's an implicit mapping of voxel types to only wall and flat texture names, 
// since voxels shouldn't be using any other texture types than those.

// If a certain voxel type, like "bed", doesn't make sense for a location like a
// dungeon, then just use a "None" or "Dev" texture type or something to indicate 
// that the texture used in its place should just be blank, since it wouldn't ever be 
// used anyway. It's better to use a blank placeholder than to find something that's
// "close enough" in this case.

// The sides of pits and rivers aren't really their own voxel. I'm not sure how that
// will fit into the voxel generation. Maybe it could be a second-pass triangle
// generation algorithm that checks adjacencies between voxel faces. I'll need to
// look more into this.

enum class VoxelType
{
	// Empty.
	Air,

	// Ground.
	Ground1,
	Ground2,
	Ground3,
	Ground4,

	// Plain old wall and their associated half walls.
	Wall1,
	Wall2,
	Wall3,
	Wall4,
	HalfWall1,
	HalfWall2,
	HalfWall3,
	HalfWall4,
	// Four diagonals for each wall texture?

	// Bridges. Multiple bridge voxel types are necessary to act as stairs,
	// or perhaps slightly raised platforms for altars in dungeons. Let's say 
	// each one is a quarter the height of a voxel.
	Bridge1,
	Bridge2,
	Bridge3,
	Bridge4,

	// Beds (direction dependent, i.e., NorthSouth/EastWest).
	Bed1,
	Bed2,

	// Tables and shelves. There are multiple heights of shelves, like in the
	// Halls of Colossus.
	Table,
	Shelf1,
	Shelf2,
	Shelf3,

	// Specific buildings...
	// Tavern1, Mages1, palace gates, etc..

	// Liquid (probably either water or lava. No ice). There should be a voxel
	// underneath them, since they are just liquid by themselves.
	Liquid
};

#endif