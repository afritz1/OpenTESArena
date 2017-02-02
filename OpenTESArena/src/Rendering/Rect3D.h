#ifndef RECT_3D_H
#define RECT_3D_H

#include <vector>

#include "../Math/Vector3.h"

// Intended for use with rendering. I thought of this optimization since all surfaces 
// in Arena can be represented as rectangles (no need for triangles).

// There's no need for explicit UV coordinates; they can be inferred relative to points
// and used as constants. In this design, it would go counter-clockwise from the top-left 
// point:
// - p1: (u=0, v=0)
// - p2: (u=0, v=1)
// - p3: (u=1, v=1)
// - p4: (u=1, v=0)
//
// p1 +--------o p4 (inferred)
//    |        |
//    |        |
//    |        |
// p2 +--------+ p3
//
// I think using rectangles is a good idea. Everything in the game (sprites, walls, 
// textures, even a skybox eventually) will be rectangular. Even calculating the
// tangent frame at an intersection will become trivial. 

// I can't think of any reason not to make this change. The flexibility of triangles 
// is not required in the ray tracer.

class Rect3D
{
private:
	Float3 p1, p2, p3;
	 
	// Gets the axis-aligned bounding box for the rectangle.
	std::pair<Float3, Float3> getAABB() const;
public:
	Rect3D(const Float3 &p1, const Float3 &p2, const Float3 &p3);
	~Rect3D();

	// Creates a rectangle using a couple vectors with a width and height. The point is 
	// assumed to be at the center of the bottom edge of the rectangle (intended for 
	// use with sprite positions).
	static Rect3D fromFrame(const Float3 &point, const Float3 &right, 
		const Float3 &up, float width, float height);

	const Float3 &getP1() const;
	const Float3 &getP2() const;
	const Float3 &getP3() const;
	Float3 getP4() const;
	Float3 getNormal() const;

	// Returns a vector of voxel coordinates for all voxels that the rectangle touches,
	// with the option to only get voxels within the world bounds.
	// - As a possible optimization to this method, to avoid heap usage, simply return
	//   the min and max XYZ values, and the caller can loop over those themselves.
	//   Maybe return a bounding box struct?
	std::vector<Int3> getTouchedVoxels(int worldWidth, int worldHeight, int worldDepth) const;
	std::vector<Int3> getTouchedVoxels() const;
};

#endif
