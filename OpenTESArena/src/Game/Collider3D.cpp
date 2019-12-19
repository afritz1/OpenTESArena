#include "Collider3D.h"
#include <typeinfo>
#include <algorithm>

Collider3D::ColliderHit::ColliderHit(const Collider3D *a, const Collider3D *b, const Double3 &pointOfImpactOnA, const Double3 &pointOfImpactOnB, const Double3 &normal) : 
	A(a), 
	B(b), 
	PointOfImpactOnA(pointOfImpactOnA), 
	PointOfImpactOnB(pointOfImpactOnB),
	Normal(normal)
{
}

Collider3D::Collider3D(const Matrix4d&transform)
{
	Transform = transform;
}

Collider3D::~Collider3D()
{
}

CapsuleCollider3D::CapsuleCollider3D(const Matrix4d&transform, double radius, double length) : Collider3D(transform)
{
	Radius = radius;
	Length = length;
}

BoxCollider3D::BoxCollider3D(const Matrix4d&transform, double width, double height, double depth) : Collider3D(transform)
{
	Width = width;
	Height = height;
	Depth = depth;
}

QuadCollider3D::QuadCollider3D(const Double3 &center, const Double3 &normal, double width, double height) : Collider3D(Matrix4d::identity())
{
	Point = center;
	Normal = normal;
	Width = width;
	Height = height;
}

AxisAlignedCylinderCollider3D::AxisAlignedCylinderCollider3D(const Double3 &center, double radius, double height) :
	Collider3D(Matrix4d::translation(center.x, center.y, center.z)),
	Radius(radius), Height(height)
{
}

bool CapsuleCollider3D::CheckCollision(const Collider3D &other, ColliderHit &hit)
{
	auto processHit = [this, &other](ColliderHit& hit) {
		Double3 pointOfImpactOnA = hit.A == &other ? hit.PointOfImpactOnB : hit.PointOfImpactOnA;
		Double3 pointOfImpactOnB = hit.B == &other ? hit.PointOfImpactOnB : hit.PointOfImpactOnA;
		Double3 normal = hit.Normal;
		hit = ColliderHit(
			static_cast<Collider3D*>(this),
			&other,
			pointOfImpactOnA,
			pointOfImpactOnB,
			normal);
	};
	if (dynamic_cast<const CapsuleCollider3D*>(&other) != nullptr)
	{
		bool collided = Collider3D::CheckCollisionCapsuleCapsule(*this, static_cast<const CapsuleCollider3D&>(other), hit);
		if (collided)
		{
			processHit(hit);
			return true;
		}
	}
	else if (dynamic_cast<const AxisAlignedCylinderCollider3D *>(&other) != nullptr)
	{
		bool collided = Collider3D::CheckCollisionCylinderCapsule(static_cast<const AxisAlignedCylinderCollider3D &>(other), *this, hit);
		if (collided)
		{
			processHit(hit);
			return true;
		}
	}
	else if (dynamic_cast<const BoxCollider3D*>(&other) != nullptr)
	{
		bool collided = Collider3D::CheckCollisionBoxCapsule(static_cast<const BoxCollider3D&>(other), *this, hit);
		if (collided)
		{
			processHit(hit);
			return true;
		}
	}
	else if (dynamic_cast<const QuadCollider3D*>(&other) != nullptr)
	{
		bool collided = Collider3D::CheckCollisionCapsuleQuad(*this, static_cast<const QuadCollider3D&>(other), hit);
		if (collided)
		{
			processHit(hit);
			return true;
		}
	}

	// No collision detected
	return false;
}

bool BoxCollider3D::CheckCollision(const Collider3D &other, ColliderHit &hit)
{
	auto processHit = [this, &other](ColliderHit &hit) {
		Double3 pointOfImpactOnA = hit.A == &other ? hit.PointOfImpactOnB : hit.PointOfImpactOnA;
		Double3 pointOfImpactOnB = hit.B == &other ? hit.PointOfImpactOnB : hit.PointOfImpactOnA;
		Double3 normal = hit.Normal;
		hit = ColliderHit(
			static_cast<Collider3D*>(this),
			&other,
			pointOfImpactOnA,
			pointOfImpactOnB,
			normal);
	};

	if (dynamic_cast<const CapsuleCollider3D*>(&other) != nullptr)
	{
		bool collided = Collider3D::CheckCollisionBoxCapsule(*this, static_cast<const CapsuleCollider3D&>(other), hit);
		if (collided)
		{
			processHit(hit);
			return true;
		}
	}
	else if (dynamic_cast<const BoxCollider3D*>(&other) != nullptr)
	{
		bool collided = Collider3D::CheckCollisionBoxBox(static_cast<const BoxCollider3D&>(other), *this, hit);
		if (collided)
		{
			processHit(hit);
			return true;
		}
	}
	else if (dynamic_cast<const QuadCollider3D*>(&other) != nullptr)
	{
		bool collided = Collider3D::CheckCollisionQuadBox(static_cast<const QuadCollider3D&>(other), *this, hit);
		if (collided)
		{
			processHit(hit);
			return true;
		}
	}

	// No collision detected
	return false;
}

bool QuadCollider3D::CheckCollision(const Collider3D &other, ColliderHit &hit)
{
	auto processHit = [this, &other](ColliderHit &hit) {
		Double3 pointOfImpactOnA = hit.A == &other ? hit.PointOfImpactOnB : hit.PointOfImpactOnA;
		Double3 pointOfImpactOnB = hit.B == &other ? hit.PointOfImpactOnB : hit.PointOfImpactOnA;
		Double3 normal = hit.Normal;
		hit = ColliderHit(
			static_cast<Collider3D*>(this),
			&other,
			pointOfImpactOnA,
			pointOfImpactOnB,
			normal);
	};

	if (dynamic_cast<const CapsuleCollider3D*>(&other) != nullptr)
	{
		bool collided = Collider3D::CheckCollisionCapsuleQuad(static_cast<const CapsuleCollider3D&>(other), *this, hit);
		if (collided)
		{
			processHit(hit);
			return true;
		}
	}
	else if (dynamic_cast<const BoxCollider3D*>(&other) != nullptr)
	{
		bool collided = Collider3D::CheckCollisionQuadBox(*this, static_cast<const BoxCollider3D&>(other), hit);
		if (collided)
		{
			processHit(hit);
			return true;
		}
	}

	// No collision detected
	return false;
}

bool AxisAlignedCylinderCollider3D::CheckCollision(const Collider3D &other, ColliderHit &hit)
{
	auto processHit = [this, &other](ColliderHit &hit) {
		Double3 pointOfImpactOnA = hit.A == &other ? hit.PointOfImpactOnB : hit.PointOfImpactOnA;
		Double3 pointOfImpactOnB = hit.B == &other ? hit.PointOfImpactOnB : hit.PointOfImpactOnA;
		Double3 normal = hit.Normal;
		hit = ColliderHit(
			static_cast<Collider3D *>(this),
			&other,
			pointOfImpactOnA,
			pointOfImpactOnB,
			normal);
	};

	if (dynamic_cast<const AxisAlignedCylinderCollider3D *>(&other) != nullptr)
	{
		bool collided = Collider3D::CheckCollisionCylinderCylinder(*this, static_cast<const AxisAlignedCylinderCollider3D &>(other), hit);
		if (collided)
		{
			processHit(hit);
			return true;
		}
	}
	else if (dynamic_cast<const CapsuleCollider3D *>(&other) != nullptr)
	{
		bool collided = Collider3D::CheckCollisionCylinderCapsule(*this, static_cast<const CapsuleCollider3D &>(other), hit);
		if (collided)
		{
			processHit(hit);
			return true;
		}
	}
	else if (dynamic_cast<const BoxCollider3D *>(&other) != nullptr)
	{
		bool collided = Collider3D::CheckCollisionBoxCylinder(static_cast<const BoxCollider3D &>(other), *this, hit);
		if (collided)
		{
			processHit(hit);
			return true;
		}
	}
	else if (dynamic_cast<const QuadCollider3D *>(&other) != nullptr)
	{
		bool collided = Collider3D::CheckCollisionCylinderQuad(*this, static_cast<const QuadCollider3D &>(other), hit);
		if (collided)
		{
			processHit(hit);
			return true;
		}
	}

	// No collision detected
	return false;
}

double distanceBetweenLineSegments(const Double3 &p0, const Double3 &u, const Double3 &q0, const Double3 &v, Double3 &Ps, Double3 &Qt)
{
	// These values are needed for the calculation of values s and t
	auto p0q0 = p0 - q0;
	auto _a = u.dot(u);
	auto _b = u.dot(v);
	auto _c = v.dot(v);
	auto _d = u.dot(p0q0);
	auto _e = v.dot(p0q0);

	auto be = _b * _e;
	auto cd = _c * _d;
	auto ac = _a * _c;
	auto ae = _a * _e;
	auto bd = _b * _d;
	auto bb = _b * _b;

	// Calculate s and t. These are the points along u and v from p0 and q0 respectively that are the closest to each other. 
	// The values are limited to the interval [0, 1] because outside of that range is along the line that the segment exists
	// on, but outside the bounds of the segment
	auto s = std::clamp((be - cd) / (ac - bb), 0.0, 1.0);
	auto t = std::clamp((ae - bd) / (ac - bb), 0.0, 1.0);

	// Calculate Psc and Qtc. These are the points on their respective segments that are closest to each other.
	Ps = p0 + (u * s);
	Qt = p0 + (v * t);

	// If the distance between the closest points on each segment is less than or equal to the sum of the radii of the capsules, then the capsules are colliding
	return (Ps - Qt).length();
}

double distanceBetweenLineSegmentAndPlane(const Double3 &point, const Double3 &normal, const Double3 &p0, const Double3 &u, Double3 &nearestPointInPlane)
{
	// Get the distance between the endpoints of the segment and the plane
	auto p1 = p0 + u;
	auto a = (p0 - point).dot(normal);
	auto b = (p1 - point).dot(normal);

	// If endpoints are on opposite sides of the plane, then the segment crosses the plane
	if (0 > a * b)
	{
		double t = abs(a) / (abs(a) + abs(b));
		nearestPointInPlane = p0 + (u * t);
		return 0;
	}
	else if(a < b) // a and b are on the same side, but a is closer
	{
		nearestPointInPlane = p0 - (normal * a);
		return a;
	}
	else if(a > b) // p0 and b are on the same side, but b is closer
	{ 
		nearestPointInPlane = p1 - (normal * b);
		return b;
	}
	else // a == b, unlikely
	{
		nearestPointInPlane = (p0 + (u / 2)) - (normal * a);
		return a;
	}
}

double distanceBetweenLineSegmentAndPoint(const Double3& p0, const Double3& u, const Double3& q, Double3& Ps)
{
	Double3 p0q = q - p0;
	double s = (u.cross(p0q) / u.lengthSquared()).length();

	Ps = p0 + (u * s);
	return (Ps - q).length();
}

bool Collider3D::CheckCollisionCapsuleCapsule(const CapsuleCollider3D &a, const CapsuleCollider3D &b, ColliderHit &hit)
{
	// p0 and q0 are the lowest point on the line segment that forms the core of their respective capsules
	auto p0 = (a.Transform * Double4(0, -a.Length / 2, 0, 1)).toXYZ();
	auto q0 = (b.Transform * Double4(0, -b.Length / 2, 0, 1)).toXYZ();
	
	// u and v are the direction and length of the core of their respective capsules
	auto u = (a.Transform * Double4(0, a.Length, 0, 0)).toXYZ();
	auto v = (b.Transform * Double4(0, b.Length, 0, 0)).toXYZ();

	Double3 Ps, Qt;
	auto dist = distanceBetweenLineSegments(p0, u, q0, v, Ps, Qt);
	
	return dist <= (a.Radius + b.Radius);
}

bool Collider3D::CheckCollisionCylinderCylinder(const AxisAlignedCylinderCollider3D &a, const AxisAlignedCylinderCollider3D &b, ColliderHit &hit)
{
	Double3 aPos = (a.Transform * Double4(0, 0, 0, 1)).toXYZ();
	Double3 bPos = (a.Transform * Double4(0, 0, 0, 1)).toXYZ();

	// If there doesn't exist an XZ plane that both cylinders exist in, then they can't be colliding
	if (aPos.y > bPos.y + b.Height)
		return false;
	if (bPos.y > aPos.y + a.Height)
		return false;
	
	// Get the plane of collision
	double top = std::min(aPos.y + a.Height, bPos.y + b.Height);
	double bottom = std::max(aPos.y, bPos.y);
	double collisionPlane = (top + bottom) / 2;
	
	// Get the distance between the points of collision
	aPos = Double3(aPos.x, collisionPlane, aPos.z);
	bPos = Double3(bPos.x, collisionPlane, bPos.z);
	Double3 b2a = (aPos - bPos);

	const double distance = b2a.length();
	if (distance > a.Radius + b.Radius)
		return false;

	// We have a collision. Calculate the hit info and return true
	Double3 normal = b2a.normalized();
	Double3 pointOnA = aPos - (normal * a.Radius);
	Double3 pointOnB = bPos + (normal * b.Radius);
	hit = ColliderHit(static_cast<const Collider3D *>(&a), static_cast<const Collider3D *>(&b), pointOnA, pointOnB, normal);
	return true;
}

bool Collider3D::CheckCollisionBoxCapsule(const BoxCollider3D &a, const CapsuleCollider3D &b, ColliderHit &hit)
{
	auto a0 = (a.Transform * Double4(0, 0, 0, 1)).toXYZ();

	double minXa = a0.x - (a.Width / 2);
	double maxXa = a0.x + (a.Width / 2);
	double minYa = a0.y - (a.Height / 2);
	double maxYa = a0.y + (a.Height / 2);
	double minZa = a0.z - (a.Depth / 2);
	double maxZa = a0.z + (a.Depth / 2);

	auto b0 = (b.Transform * Double4(0, -b.Length / 2, 0, 1)).toXYZ();
	auto v = (b.Transform * Double4(0, b.Length, 0, 0)).toXYZ();
	auto b1 = b0 + v;

	// Check if the capsule is separated from the box by an axis-normal plane
	if (b0.x - maxXa > b.Radius && b1.x - maxXa > b.Radius)
		return false;
	if (b0.y - maxYa > b.Radius && b1.y - maxYa > b.Radius)
		return false;
	if (b0.z - maxZa > b.Radius && b1.z - maxZa > b.Radius)
		return false;
	if (minXa - b0.x > b.Radius && minXa - b1.x > b.Radius)
		return false;
	if (minYa - b0.y > b.Radius && minYa - b1.y > b.Radius)
		return false;
	if (minZa - b0.z > b.Radius && minYa - b1.z > b.Radius)
		return false;

	// If we made it this far, then the collision detection is going to be a bit trickier
	// We want to check for every possible collision between the capsule and the faces, edges, and vertices of the box. If no possible collisions are found, then we return false
	// If we find at least 1 possible collision, we sort by the shortest collision distance and return that one
	std::vector<ColliderHit> possibleCollisions;

	// Check if the capsule is colliding with one of the faces
	auto processFaceCollision = [&possibleCollisions, &a, &b, &b0, &v, &minXa, &maxXa, &minYa, &maxYa, &minZa, &maxZa](const Double3 &point, const Double3 &normal) {
		Double3 projection;
		auto distance = distanceBetweenLineSegmentAndPlane(point, normal, b0, v, projection);
		Double3 nearestPoint = projection + (normal * (distance - b.Radius));

		if(distance <= b.Radius &&
			(projection.x == std::clamp(projection.x, minXa, maxXa)) &&
			(projection.y == std::clamp(projection.y, minYa, maxYa)) && 
			(projection.z == std::clamp(projection.z, minZa, maxZa)))
			possibleCollisions.push_back(ColliderHit(static_cast<const Collider3D*>(&a), static_cast<const Collider3D*>(&b), projection, nearestPoint, normal));
	};

	processFaceCollision(Double3(maxXa, a0.y, a0.z), Double3::UnitX);
	processFaceCollision(Double3(minXa, a0.y, a0.z), -Double3::UnitX);
	processFaceCollision(Double3(a0.x, maxYa, a0.z), Double3::UnitY);
	processFaceCollision(Double3(a0.x, minYa, a0.z), -Double3::UnitY);
	processFaceCollision(Double3(a0.x, a0.y, maxZa), Double3::UnitZ);
	processFaceCollision(Double3(a0.x, a0.y, minZa), -Double3::UnitZ);

	if (possibleCollisions.size() > 0)
	{
		hit = possibleCollisions[0];
		return true;
	}

	// Get the distance to each vertex
	auto processVertexCollision = [&possibleCollisions, &a, &b, &b0, &v](const Double3 &vertex) {
		Double3 Ps;
		double distance = distanceBetweenLineSegmentAndPoint(b0, v, vertex, Ps);
		
		if (distance <= b.Radius)
		{
			Double3 pointOnCapsuleSurface = Ps + ((vertex - Ps) * (b.Radius / distance));
			Double3 normal = (Ps - vertex).normalized();
			possibleCollisions.push_back(ColliderHit(static_cast<const Collider3D*>(&a), static_cast<const Collider3D*>(&b), vertex, pointOnCapsuleSurface, normal));
		}
	};

	processVertexCollision(Double3(minXa, minYa, minZa));
	processVertexCollision(Double3(maxXa, minYa, minZa));
	processVertexCollision(Double3(minXa, maxYa, minZa));
	processVertexCollision(Double3(maxXa, maxYa, minZa));
	processVertexCollision(Double3(minXa, minYa, maxZa));
	processVertexCollision(Double3(maxXa, minYa, maxZa));
	processVertexCollision(Double3(minXa, maxYa, maxZa));
	processVertexCollision(Double3(maxXa, maxYa, maxZa));

	if (possibleCollisions.size() > 0)
	{
		hit = possibleCollisions[0];
		return true;
	}

	// Get the distance to each edge.
	auto processEdgeCollision = [&possibleCollisions, &a, &b, &b0, &v](const Double3 &edgeStart, const Double3 &edgeEnd) {

		Double3 Ps, Qt;
		double distance = distanceBetweenLineSegments(edgeStart, edgeEnd, b0, v, Ps, Qt);

		if (distance <= b.Radius) {
			Double3 normal = (Qt - Ps).normalized();
			Double3 pointOnCapsuleSurface = Qt - (normal * b.Radius);
			possibleCollisions.push_back(ColliderHit(static_cast<const Collider3D*>(&a), static_cast<const Collider3D*>(&b), Ps, pointOnCapsuleSurface, normal));
		}
	};

	processEdgeCollision(Double3(minXa, minYa, minZa), Double3(a.Width, 0, 0));
	processEdgeCollision(Double3(minXa, maxYa, minZa), Double3(a.Width, 0, 0));
	processEdgeCollision(Double3(minXa, minYa, maxZa), Double3(a.Width, 0, 0));
	processEdgeCollision(Double3(minXa, maxYa, maxZa), Double3(a.Width, 0, 0));
	processEdgeCollision(Double3(minXa, minYa, minZa), Double3(0, a.Height, 0));
	processEdgeCollision(Double3(maxXa, minYa, minZa), Double3(0, a.Height, 0));
	processEdgeCollision(Double3(minXa, minYa, maxZa), Double3(0, a.Height, 0));
	processEdgeCollision(Double3(maxXa, minYa, maxZa), Double3(0, a.Height, 0));
	processEdgeCollision(Double3(minXa, minYa, minZa), Double3(0, 0, a.Depth));
	processEdgeCollision(Double3(maxXa, minYa, minZa), Double3(0, 0, a.Depth));
	processEdgeCollision(Double3(minXa, maxYa, minZa), Double3(0, 0, a.Depth));
	processEdgeCollision(Double3(maxXa, maxYa, minZa), Double3(0, 0, a.Depth));

	if (possibleCollisions.size() > 0)
	{
		hit = possibleCollisions[0];
		return true;
	}

	// If we got this far, we exhausted all options and there cannot possibly be a collision
	return false;
}

bool Collider3D::CheckCollisionBoxCylinder(const BoxCollider3D &a, const AxisAlignedCylinderCollider3D &b, ColliderHit &hit)
{
	auto handleCollision = [&a, &b, &hit](const Double3 &pointOnA, const Double3 &pointOnB, const Double3 &normal)
	{
		hit = ColliderHit(static_cast<const Collider3D *>(&a), static_cast<const Collider3D *>(&b), pointOnA, pointOnB, normal);
	};

	auto a0 = a.Transform * Double4(0, 0, 0, 1);
	double minXa = a0.x - (a.Width / 2);
	double maxXa = a0.x + (a.Width / 2);
	double minYa = a0.y - (a.Height / 2);
	double maxYa = a0.y + (a.Height / 2);
	double minZa = a0.z - (a.Depth / 2);
	double maxZa = a0.z + (a.Depth / 2);

	Double3 bPos = (b.Transform * Double4(0, 0, 0, 1)).toXYZ();
	Double3 bTop = Double3(bPos.x, bPos.y + b.Height, bPos.z);

	// Check if there's an XZ plane that intersects both
	if (bPos.y > maxYa || bTop.y < minYa)
		return false;

	// Check if the cylinder is too far out of bounds to be colliding with an edge or a face
	if (bPos.x > maxXa + b.Radius || bPos.x < minXa - b.Radius)
		return false;
	if (bPos.z > maxZa + b.Radius || bPos.z < minZa - b.Radius)
		return false;

	if (bPos.x < maxXa && bPos.x > minXa &&
		bPos.z < maxZa && bPos.z > minZa)
	{
		// Note: This will cause the cylinder to fall through the box the minute the top of the cylinder is below the top of the box. This isn't strictly a bad thing, but if you reverse these conditions
		// then the cylinder can pop up through the box on collision, the way players do in the real game
		if (bTop.y < maxYa) // The cylinder is colliding with the top of the box
		{
			auto pointOnB = bPos;
			auto pointOnA = Double3(bPos.x, maxYa, bPos.z);
			auto normal = -Double3::UnitY;
			handleCollision(pointOnA, pointOnB, normal);
			return true;
		}
		
		// The top of the cylinder is colliding with the bottom of the box
		auto pointOnB = bTop;
		auto pointOnA = Double3(bTop.x, minYa, bTop.z);
		auto normal = Double3::UnitY;
		handleCollision(pointOnA, pointOnB, normal);
		return true;
	}
	else
	{
		// We're possible colliding with a side or an edge of the box. Find the plane of collision
		double abovePlane = std::min(bTop.y, maxYa);
		double belowPlane = std::max(bPos.y, minYa);
		double collisionPlane = (abovePlane + belowPlane) / 2;

		if (bPos.x > minXa && bPos.x < maxXa)
		{
			if (bPos.z > maxZa)
			{
				if (bPos.z - b.Radius < maxZa)
				{
					// collision
					auto pointOnA = Double3(bPos.x, collisionPlane, maxZa);
					auto pointOnB = Double3(bPos.x, collisionPlane, bPos.z - b.Radius);
					auto normal = -Double3::UnitZ;
					handleCollision(pointOnA, pointOnB, normal);
				}
			}
			else // bPos.z < minZa
			{
				if (bPos.z + b.Radius < minZa)
				{
					// collision
					auto pointOnA = Double3(bPos.x, collisionPlane, minZa);
					auto pointOnB = Double3(bPos.x, collisionPlane, bPos.z + b.Radius);
					auto normal = Double3::UnitZ;
					handleCollision(pointOnA, pointOnB, normal);
				}
			}
		}
		else if (bPos.z > minZa && bPos.z < maxZa)
		{
			if (bPos.x > maxXa)
			{
				if (bPos.x - b.Radius < maxXa)
				{
					// collision
					auto pointOnA = Double3(maxXa, collisionPlane, bPos.z);
					auto pointOnB = Double3(bPos.x - b.Radius, collisionPlane, bPos.z);
					auto normal = -Double3::UnitZ;
					handleCollision(pointOnA, pointOnB, normal);
				}
			}
			else // bPos.x < minXa
			{
				if (bPos.x + b.Radius < minXa)
				{
					// collision
					auto pointOnA = Double3(minXa, collisionPlane, bPos.z);
					auto pointOnB = Double3(bPos.x + b.Radius, collisionPlane, bPos.z);
					auto normal = Double3::UnitZ;
					handleCollision(pointOnA, pointOnB, normal);
				}
			}
		}
		else
		{
			auto bPosC = Double3(bPos.x, collisionPlane, bPos.z);

			auto checkEdgeCollision = [&a, &b, &hit, &bPosC, &handleCollision](const Double3 &corner) -> bool
			{
				auto b2c = corner - bPosC;
				auto radSqr = b.Radius * b.Radius;
				if (b2c.lengthSquared() < radSqr)
				{
					auto normal = (bPosC - corner).normalized();
					auto pointOnA = corner;
					auto pointOnB = bPosC + (normal * b.Radius);
					handleCollision(pointOnA, pointOnB, normal);
					return true;
				}
				return false;
			};

			// Not colliding with any of the faces. Time to check the edges
			if(checkEdgeCollision(Double3(minXa, collisionPlane, minZa)))
				return true;
			if (checkEdgeCollision(Double3(maxXa, collisionPlane, minZa)))
				return true;
			if (checkEdgeCollision(Double3(minXa, collisionPlane, maxZa)))
				return true;
			if (checkEdgeCollision(Double3(maxXa, collisionPlane, maxZa)))
				return true;
		}
	}

	return false;
}

bool Collider3D::CheckCollisionBoxBox(const BoxCollider3D &a, const BoxCollider3D &b, ColliderHit &hit)
{
	auto a0 = a.Transform * Double4(0, 0, 0, 1);
	auto b0 = b.Transform * Double4(0, 0, 0, 1);

	double minXa = a0.x - (a.Width / 2);
	double maxXa = a0.x + (a.Width / 2);
	double minYa = a0.y - (a.Height / 2);
	double maxYa = a0.y + (a.Height / 2);
	double minZa = a0.z - (a.Depth / 2);
	double maxZa = a0.z + (a.Depth / 2);

	double minXb = b0.x - (b.Width / 2);
	double maxXb = b0.x + (b.Width / 2);
	double minYb = b0.y - (b.Height / 2);
	double maxYb = b0.y + (b.Height / 2);
	double minZb = b0.z - (b.Depth / 2);
	double maxZb = b0.z + (b.Depth / 2);

	if ((minXa > maxXb) || (minXb > maxXa))
		return false;

	if ((minYa > maxYb) || (minYb > maxYa))
		return false;

	if ((minZa > maxZb) || (minZb > maxZa))
		return false;

	return true;
}

bool Collider3D::CheckCollisionCapsuleQuad(const CapsuleCollider3D &a, const QuadCollider3D &b, ColliderHit &hit)
{
	// @todo: Implement this function
	return false;
}

bool Collider3D::CheckCollisionQuadBox(const QuadCollider3D &a, const BoxCollider3D &b, ColliderHit &hit)
{
	// @todo: Implement this function
	return false;
}

bool Collider3D::CheckCollisionCylinderCapsule(const AxisAlignedCylinderCollider3D &A, const CapsuleCollider3D &B, ColliderHit &hit)
{
	// @todo: Implement this function
	return false;
}

bool Collider3D::CheckCollisionCylinderQuad(const AxisAlignedCylinderCollider3D &A, const QuadCollider3D &B, ColliderHit &hit)
{
	// @todo: Implement this function
	return false;
}