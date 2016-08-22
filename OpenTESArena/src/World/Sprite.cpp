#include "Sprite.h"

#include "../Entities/Directable.h"
#include "../Math/Rect3D.h"
#include "../Utilities/Debug.h"

Sprite::Sprite(const Float3d &point, const Float3d &direction,
	double width, double height)
	: point(point), direction(direction)
{
	this->width = width;
	this->height = height;
}

Sprite::~Sprite()
{

}

std::vector<Int3> Sprite::getTouchedVoxels() const
{
	// No need for any reference to the world; just do float and integer math to
	// obtain a list of independent integer coordinates.

	const Float3d up = Directable::getGlobalUp();
	const Float3d right = this->direction.cross(up).normalized();

	const Float3f pointF(
		static_cast<float>(this->point.getX()),
		static_cast<float>(this->point.getY()),
		static_cast<float>(this->point.getZ()));
	const Float3f upF(
		static_cast<float>(up.getX()),
		static_cast<float>(up.getY()),
		static_cast<float>(up.getZ()));
	const Float3f rightF(
		static_cast<float>(right.getX()),
		static_cast<float>(right.getY()),
		static_cast<float>(right.getZ()));
	const float widthF = static_cast<float>(this->width);
	const float heightF = static_cast<float>(this->height);

	Rect3D rect = Rect3D::fromFrame(pointF, rightF, upF, widthF, heightF);

	// For a naive method, just get the extent of the sprite's rectangle and make an 
	// axis-aligned bounding box for it, then calculate the coordinates of all touched 
	// voxels (corner to corner, along the edges...).

	// For the more accurate method, this algorithm can assume that the sprite's normal
	// is always perpendicular to the global up, therefore allowing its geometry to be 
	// treated like a 2D line in the XZ plane. Maybe do either ray casting or Bresenham's 
	// from the top-down view using p2 to p3 on the rectangle, and copy the resulting
	// coordinates for each level of Y from the bottom up?

	Debug::crash("Sprite", "getTouchedVoxels() not implemented.");

	return std::vector<Int3>();
}
