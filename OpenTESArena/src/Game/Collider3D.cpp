#include "Collider3D.h"
#include <typeinfo>

Collider3D::Collider3D(const Matrix4<double>& transform)
{
	Transform = transform;
}

Collider3D::~Collider3D()
{
}

CapsuleCollider3D::CapsuleCollider3D(const Matrix4<double>& transform, const double &radius, const double &length) : Collider3D(transform)
{
	Radius = radius;
	Length = length;
}

BoxCollider3D::BoxCollider3D(const Matrix4<double> &transform, const double& width, const double& height, const double &depth) : Collider3D(transform)
{
	Width = width;
	Height = height;
	Depth = depth;
}

bool CapsuleCollider3D::CheckCollision(const Collider3D& other)
{
	if (typeid(other) == typeid(CapsuleCollider3D))
	{
		return Collider3D::CheckCollisionCapsuleCapsule(*this, reinterpret_cast<const CapsuleCollider3D&>(other));
	}
	else if (typeid(other) == typeid(BoxCollider3D))
	{
		return Collider3D::CheckCollisionBoxCapsule(reinterpret_cast<const BoxCollider3D&>(other), *this);
	}
	else
	{
		// Unrecognized collider type
		return false;
	}
}

bool BoxCollider3D::CheckCollision(const Collider3D& other)
{
	if (typeid(other) == typeid(CapsuleCollider3D))
	{
		return Collider3D::CheckCollisionBoxCapsule(*this, reinterpret_cast<const CapsuleCollider3D&>(other));
	}
	else if (typeid(other) == typeid(BoxCollider3D))
	{
		return Collider3D::CheckCollisionBoxBox(*this, reinterpret_cast<const BoxCollider3D&>(other));
	}
	else
	{
		// Unrecognized collider type
		return false;
	}
}

double dot3(Double4 a, Double4 b)
{
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

double clamp(double x, double min, double max)
{
	return x > max ? max : (x < min) ? min : x;
}

double distanceBetweenLineSegments(const Double4 &p0, const Double4 &u, const Double4 &q0, const Double4 &v)
{
	// These values are needed for the calculation of values s and t
	auto p0q0 = p0 - q0;
	auto _a = dot3(u, u);
	auto _b = dot3(u, v);
	auto _c = dot3(v, v);
	auto _d = dot3(u, p0q0);
	auto _e = dot3(v, p0q0);

	auto be = _b * _e;
	auto cd = _c * _d;
	auto ac = _a * _c;
	auto ae = _a * _e;
	auto bd = _b * _d;
	auto bb = _b * _b;

	// Calculate s and t. These are the points along u and v from p0 and q0 respectively that are the closest to each other. 
	// The values are limited to the interval [0, 1] because outside of that range is along the line that the segment exists
	// on, but outside the bounds of the segment
	auto s = clamp((be - cd) / (ac - bb), 0, 1);
	auto t = clamp((ae - bd) / (ac - bb), 0, 1);

	// Calculate Psc and Qtc. These are the points on their respective segments that are closest to each other.
	auto Psc = p0 + (u * s);
	auto Qtc = p0 + (v * t);

	// If the distance between the closest points on each segment is less than or equal to the sum of the radii of the capsules, then the capsules are colliding
	return (Psc - Qtc).length;
}

double distanceBetweenLineSegmentAndPlane(const Double4& point, const Double4& normal, const Double4& p0, const Double4& u)
{
	// Get the distance between the endpoints of the segment and the plane
	auto a = dot3(p0 - point, normal);
	auto b = dot3(p0 + u - point, normal);

	// If endpoints are on opposite sides of the plane, then the segment crosses the plane
	if (0 > a* b)
		return 0;

	// Return the absolute value of the distance
	return abs((a < b) ? a : b);
}

Double4 NearestPointOnPlane(const Double4& point, const Double4& normal, const Double4& p0, const Double4& u)
{
	auto a = dot3(p0 - point, normal);
	auto b = dot3(p0 + u - point, normal);

	if (a * b < 0)
	{
		auto s = a / (a + b);
		return p0 + (u * s);
	}

	if (abs(a) < abs(b))
	{
		return (normal * -a) + p0;
	}
	else
	{
		return (normal * -b) + p0 + u;
	}
}

Double4 IntersectLineSegmentAndPlane(const Double4& point, const Double4& normal, const Double4& p0, const Double4& u)
{
	auto a = dot3(p0 - point, normal);
	auto b = dot3(p0 + u - point, normal);

	auto s = a / (a + b);
	return p0 + (u * s);
}

bool Collider3D::CheckCollisionCapsuleCapsule(const CapsuleCollider3D& a, const CapsuleCollider3D& b)
{
	// p0 and q0 are the lowest point on the line segment that forms the core of their respective capsules
	auto p0 = a.Transform * Double4(0, -a.Length / 2, 0, 1);
	auto q0 = b.Transform * Double4(0, -b.Length / 2, 0, 1);
	
	// u and v are the direction and length of the core of their respective capsules
	auto u = (a.Transform * Double4(0, a.Length, 0, 0));
	auto v = b.Transform * Double4(0, b.Length, 0, 0);

	auto dist = distanceBetweenLineSegments(p0, u, q0, v);
	
	return dist <= (a.Radius + b.Radius);
}

bool Collider3D::CheckCollisionBoxCapsule(const BoxCollider3D& a, const CapsuleCollider3D& b)
{
	auto a0 = a.Transform * Double4(0, 0, 0, 1);

	double minXa = a0.x - (a.Width / 2);
	double maxXa = a0.x + (a.Width / 2);
	double minYa = a0.y - (a.Height / 2);
	double maxYa = a0.y + (a.Height / 2);
	double minZa = a0.z - (a.Depth / 2);
	double maxZa = a0.z + (a.Depth / 2);

	auto b0 = b.Transform * Double4(0, -b.Length / 2, 0, 1);
	auto v = b.Transform * Double4(0, b.Length, 0, 0);
	auto b1 = b0 + v;

	// Check if the capsule is separated by the box by an axis-normal plane
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

	// Check if one of the endpoints is inside the box
	if (b0.x >= minXa && b0.x <= maxXa &&
		b0.y >= minYa && b0.y <= maxYa &&
		b0.z >= minZa && b0.z <= maxZa)
		return true;
	if (b1.x >= minXa && b1.x <= maxXa &&
		b1.y >= minYa && b1.y <= maxYa &&
		b1.z >= minZa && b1.z <= maxZa)
		return true;

	// If we made it this far, then the collision detection is going to be a bit trickier

	// Get the distance to each edge. This will return true if colliding with a vertex too
	auto _a = distanceBetweenLineSegments(Double4(minXa, minYa, minZa, 0), Double4(a.Width, 0, 0, 0), b0, v);
	auto _b = distanceBetweenLineSegments(Double4(minXa, maxYa, minZa, 0), Double4(a.Width, 0, 0, 0), b0, v);
	auto _c = distanceBetweenLineSegments(Double4(minXa, minYa, maxZa, 0), Double4(a.Width, 0, 0, 0), b0, v);
	auto _d = distanceBetweenLineSegments(Double4(minXa, maxYa, maxZa, 0), Double4(a.Width, 0, 0, 0), b0, v);
	auto _e = distanceBetweenLineSegments(Double4(minXa, minYa, minZa, 0), Double4(0,a.Height, 0, 0), b0, v);
	auto _f = distanceBetweenLineSegments(Double4(maxXa, minYa, minZa, 0), Double4(0, a.Height, 0, 0), b0, v);
	auto _g = distanceBetweenLineSegments(Double4(minXa, minYa, maxZa, 0), Double4(0, a.Height, 0, 0), b0, v);
	auto _h = distanceBetweenLineSegments(Double4(maxXa, minYa, maxZa, 0), Double4(0, a.Height, 0, 0), b0, v);
	auto _i = distanceBetweenLineSegments(Double4(minXa, minYa, minZa, 0), Double4(0, 0, a.Depth, 0), b0, v);
	auto _j = distanceBetweenLineSegments(Double4(maxXa, minYa, minZa, 0), Double4(0, 0, a.Depth, 0), b0, v);
	auto _k = distanceBetweenLineSegments(Double4(minXa, maxYa, minZa, 0), Double4(0, 0, a.Depth, 0), b0, v);
	auto _l = distanceBetweenLineSegments(Double4(maxXa, maxYa, minZa, 0), Double4(0, 0, a.Depth, 0), b0, v);

	if (_a <= b.Radius || _b <= b.Radius || _c <= b.Radius || _d <= b.Radius ||
		_e <= b.Radius || _f <= b.Radius || _g <= b.Radius || _h <= b.Radius ||
		_i <= b.Radius || _j <= b.Radius || _k <= b.Radius || _l <= b.Radius)
		return true;

	// Still need to check if the capsule collides with a face
	return false;
}

bool Collider3D::CheckCollisionBoxBox(const BoxCollider3D& a, const BoxCollider3D& b)
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