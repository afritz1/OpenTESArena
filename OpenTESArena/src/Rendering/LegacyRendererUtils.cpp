#include <algorithm>

#include "LegacyRendererUtils.h"
#include "RendererUtils.h"
#include "../Math/MathUtils.h"

#include "components/debug/Debug.h"

Double3 LegacyRendererUtils::getSkyGradientRowColor(double gradientPercent, const Double3 *skyColors, int skyColorCount)
{
	// Determine which sky color index the percent falls into, and how much of that
	// color to interpolate with the next one.
	const double realIndex = MathUtils::getRealIndex(skyColorCount, gradientPercent);
	const double percent = realIndex - std::floor(realIndex);
	const int index = static_cast<int>(realIndex);
	const int nextIndex = std::clamp(index + 1, 0, skyColorCount - 1);
	const Double3 &color = skyColors[index];
	const Double3 &nextColor = skyColors[nextIndex];
	return color.lerp(nextColor, percent);
}

Double3 LegacyRendererUtils::getThunderstormFlashColor(double flashPercent, const Double3 *colors, int colorCount)
{
	DebugAssert(colors != nullptr);
	DebugAssert(colorCount > 0);

	const double realIndex = MathUtils::getRealIndex(colorCount, flashPercent);
	const double percent = realIndex - std::floor(realIndex);
	const int index = static_cast<int>(realIndex);
	const int nextIndex = std::clamp(index + 1, 0, colorCount - 1);
	const Double3 &color = colors[index];
	const Double3 &nextColor = colors[nextIndex];
	return color.lerp(nextColor, percent);
}

/*bool LegacyRendererUtils::findDiag1Intersection(const CoordInt2 &coord, const NewDouble2 &nearPoint,
	const NewDouble2 &farPoint, RayHit &hit)
{
	// Start, middle, and end points of the diagonal line segment relative to the grid.
	const NewInt2 absoluteVoxel = VoxelUtils::coordToNewVoxel(coord);
	NewDouble2 diagStart, diagMiddle, diagEnd;
	RendererUtils::getDiag1Points2D(absoluteVoxel.x, absoluteVoxel.y, &diagStart, &diagMiddle, &diagEnd);

	// Normals for the left and right faces of the wall, facing down-right and up-left
	// respectively (magic number is sqrt(2) / 2).
	const Double3 leftNormal(0.7071068, 0.0, -0.7071068);
	const Double3 rightNormal(-0.7071068, 0.0, 0.7071068);

	// An intersection occurs if the near point and far point are on different sides 
	// of the diagonal line, or if the near point lies on the diagonal line. No need
	// to normalize the (localPoint - diagMiddle) vector because it's just checking
	// if it's greater than zero.
	const NewDouble2 leftNormal2D(leftNormal.x, leftNormal.z);
	const bool nearOnLeft = leftNormal2D.dot(nearPoint - diagMiddle) >= 0.0;
	const bool farOnLeft = leftNormal2D.dot(farPoint - diagMiddle) >= 0.0;
	const bool intersectionOccurred = (nearOnLeft && !farOnLeft) || (!nearOnLeft && farOnLeft);

	// Only set the output data if an intersection occurred.
	if (intersectionOccurred)
	{
		// Change in X and change in Z of the incoming ray across the voxel.
		const SNDouble dx = farPoint.x - nearPoint.x;
		const WEDouble dz = farPoint.y - nearPoint.y;

		// The hit coordinate is a 0->1 value representing where the diagonal was hit.
		const double hitCoordinate = [&nearPoint, &diagStart, dx, dz]()
		{
			// Special cases: when the slope is horizontal or vertical. This method treats
			// the X axis as the vertical axis and the Z axis as the horizontal axis.
			const bool isHorizontal = std::abs(dx) < Constants::Epsilon;
			const bool isVertical = std::abs(dz) < Constants::Epsilon;

			if (isHorizontal)
			{
				// The X axis intercept is the intersection coordinate.
				return nearPoint.x - diagStart.x;
			}
			else if (isVertical)
			{
				// The Z axis intercept is the intersection coordinate.
				return nearPoint.y - diagStart.y;
			}
			else
			{
				// Slope of the diagonal line (trivial, x = z).
				constexpr double diagSlope = 1.0;

				// Vertical axis intercept of the diagonal line.
				const double diagXIntercept = diagStart.x - diagStart.y;

				// Slope of the incoming ray.
				const double raySlope = dx / dz;

				// Get the vertical axis intercept of the incoming ray.
				const double rayXIntercept = nearPoint.x - (raySlope * nearPoint.y);

				// General line intersection calculation.
				return ((rayXIntercept - diagXIntercept) / (diagSlope - raySlope)) - diagStart.y;
			}
		}();

		// Set the hit data.
		hit.u = std::clamp(hitCoordinate, 0.0, Constants::JustBelowOne);
		hit.point = diagStart + ((diagEnd - diagStart) * hitCoordinate);
		hit.innerZ = (hit.point - nearPoint).length();
		hit.normal = nearOnLeft ? leftNormal : rightNormal;

		return true;
	}
	else
	{
		// No intersection.
		return false;
	}
}

bool LegacyRendererUtils::findDiag2Intersection(const CoordInt2 &coord, const NewDouble2 &nearPoint,
	const NewDouble2 &farPoint, RayHit &hit)
{
	// Mostly a copy of findDiag1Intersection(), though with a couple different values
	// for the diagonal (end points, slope, etc.).

	// Start, middle, and end points of the diagonal line segment relative to the grid.
	const NewInt2 absoluteVoxel = VoxelUtils::coordToNewVoxel(coord);
	NewDouble2 diagStart, diagMiddle, diagEnd;
	RendererUtils::getDiag2Points2D(absoluteVoxel.x, absoluteVoxel.y, &diagStart, &diagMiddle, &diagEnd);

	// Normals for the left and right faces of the wall, facing down-left and up-right
	// respectively (magic number is sqrt(2) / 2).
	const Double3 leftNormal(0.7071068, 0.0, 0.7071068);
	const Double3 rightNormal(-0.7071068, 0.0, -0.7071068);

	// An intersection occurs if the near point and far point are on different sides 
	// of the diagonal line, or if the near point lies on the diagonal line. No need
	// to normalize the (localPoint - diagMiddle) vector because it's just checking
	// if it's greater than zero.
	const Double2 leftNormal2D(leftNormal.x, leftNormal.z);
	const bool nearOnLeft = leftNormal2D.dot(nearPoint - diagMiddle) >= 0.0;
	const bool farOnLeft = leftNormal2D.dot(farPoint - diagMiddle) >= 0.0;
	const bool intersectionOccurred = (nearOnLeft && !farOnLeft) || (!nearOnLeft && farOnLeft);

	// Only set the output data if an intersection occurred.
	if (intersectionOccurred)
	{
		// Change in X and change in Z of the incoming ray across the voxel.
		const SNDouble dx = farPoint.x - nearPoint.x;
		const WEDouble dz = farPoint.y - nearPoint.y;

		// The hit coordinate is a 0->1 value representing where the diagonal was hit.
		const double hitCoordinate = [&nearPoint, &diagStart, dx, dz]()
		{
			// Special cases: when the slope is horizontal or vertical. This method treats
			// the X axis as the vertical axis and the Z axis as the horizontal axis.
			const bool isHorizontal = std::abs(dx) < Constants::Epsilon;
			const bool isVertical = std::abs(dz) < Constants::Epsilon;

			if (isHorizontal)
			{
				// The X axis intercept is the compliment of the intersection coordinate.
				return Constants::JustBelowOne - (nearPoint.x - diagStart.x);
			}
			else if (isVertical)
			{
				// The Z axis intercept is the compliment of the intersection coordinate.
				return Constants::JustBelowOne - (nearPoint.y - diagStart.y);
			}
			else
			{
				// Slope of the diagonal line (trivial, x = -z).
				const double diagSlope = -1.0;

				// Vertical axis intercept of the diagonal line.
				const double diagXIntercept = diagStart.x + diagStart.y;

				// Slope of the incoming ray.
				const double raySlope = dx / dz;

				// Get the vertical axis intercept of the incoming ray.
				const double rayXIntercept = nearPoint.x - (raySlope * nearPoint.y);

				// General line intersection calculation.
				return ((rayXIntercept - diagXIntercept) / (diagSlope - raySlope)) - diagStart.y;
			}
		}();

		// Set the hit data.
		hit.u = std::clamp(Constants::JustBelowOne - hitCoordinate, 0.0, Constants::JustBelowOne);
		hit.point = diagStart + ((diagEnd - diagStart) * hitCoordinate);
		hit.innerZ = (hit.point - nearPoint).length();
		hit.normal = nearOnLeft ? leftNormal : rightNormal;

		return true;
	}
	else
	{
		// No intersection.
		return false;
	}
}

bool LegacyRendererUtils::findInitialEdgeIntersection(const CoordInt2 &coord, VoxelFacing2D edgeFacing,
	bool flipped, const NewDouble2 &nearPoint, const NewDouble2 &farPoint, const Camera &camera,
	const Ray &ray, RayHit &hit)
{
	// Reuse the chasm facing code to find which face is intersected.
	const NewDouble3 absoluteEye = VoxelUtils::coordToNewPoint(camera.eye);
	const NewDouble2 absoluteEye2D(absoluteEye.x, absoluteEye.z);
	const VoxelFacing2D farFacing = SoftwareRenderer::getInitialChasmFarFacing(coord, absoluteEye2D, ray);

	// If the edge facing and far facing match, there's an intersection.
	if (edgeFacing == farFacing)
	{
		hit.innerZ = (farPoint - nearPoint).length();
		hit.u = [flipped, &farPoint, farFacing]()
		{
			const double uVal = [&farPoint, farFacing]()
			{
				if (farFacing == VoxelFacing2D::PositiveX)
				{
					return Constants::JustBelowOne - (farPoint.y - std::floor(farPoint.y));
				}
				else if (farFacing == VoxelFacing2D::NegativeX)
				{
					return farPoint.y - std::floor(farPoint.y);
				}
				else if (farFacing == VoxelFacing2D::PositiveZ)
				{
					return farPoint.x - std::floor(farPoint.x);
				}
				else
				{
					return Constants::JustBelowOne - (farPoint.x - std::floor(farPoint.x));
				}
			}();

			// Account for the possibility of the texture being flipped horizontally.
			return std::clamp(!flipped ? uVal : (Constants::JustBelowOne - uVal),
				0.0, Constants::JustBelowOne);
		}();

		hit.point = farPoint;
		hit.normal = -VoxelUtils::getNormal(farFacing);
		return true;
	}
	else
	{
		// No intersection.
		return false;
	}
}

bool LegacyRendererUtils::findEdgeIntersection(const CoordInt2 &coord, VoxelFacing2D edgeFacing,
	bool flipped, VoxelFacing2D nearFacing, const NewDouble2 &nearPoint, const NewDouble2 &farPoint,
	double nearU, const Camera &camera, const Ray &ray, RayHit &hit)
{
	// If the edge facing and near facing match, the intersection is trivial.
	if (edgeFacing == nearFacing)
	{
		hit.innerZ = 0.0;
		hit.u = !flipped ? nearU : std::clamp(
			Constants::JustBelowOne - nearU, 0.0, Constants::JustBelowOne);
		hit.point = nearPoint;
		hit.normal = VoxelUtils::getNormal(nearFacing);
		return true;
	}
	else
	{
		// A search is needed to see whether an intersection occurred. Reuse the chasm
		// facing code to find what the far facing is.
		const VoxelFacing2D farFacing = SoftwareRenderer::getChasmFarFacing(coord, nearFacing, camera, ray);

		// If the edge facing and far facing match, there's an intersection.
		if (edgeFacing == farFacing)
		{
			hit.innerZ = (farPoint - nearPoint).length();
			hit.u = [flipped, &farPoint, farFacing]()
			{
				const double uVal = [&farPoint, farFacing]()
				{
					if (farFacing == VoxelFacing2D::PositiveX)
					{
						return Constants::JustBelowOne - (farPoint.y - std::floor(farPoint.y));
					}
					else if (farFacing == VoxelFacing2D::NegativeX)
					{
						return farPoint.y - std::floor(farPoint.y);
					}
					else if (farFacing == VoxelFacing2D::PositiveZ)
					{
						return farPoint.x - std::floor(farPoint.x);
					}
					else
					{
						return Constants::JustBelowOne - (farPoint.x - std::floor(farPoint.x));
					}
				}();

				// Account for the possibility of the texture being flipped horizontally.
				return std::clamp(!flipped ? uVal : (Constants::JustBelowOne - uVal),
					0.0, Constants::JustBelowOne);
			}();

			hit.point = farPoint;
			hit.normal = -VoxelUtils::getNormal(farFacing);
			return true;
		}
		else
		{
			// No intersection.
			return false;
		}
	}
}

bool LegacyRendererUtils::findInitialSwingingDoorIntersection(const CoordInt2 &coord, double percentOpen,
	const NewDouble2 &nearPoint, const NewDouble2 &farPoint, bool xAxis, const Camera &camera,
	const Ray &ray, RayHit &hit)
{
	const NewInt2 absoluteVoxel = VoxelUtils::coordToNewVoxel(coord);

	// Decide which corner the door's hinge will be in, and create the line segment
	// that will be rotated based on percent open.
	NewDouble2 interpStart;
	const NewDouble2 pivot = [&absoluteVoxel, xAxis, &interpStart]()
	{
		const NewInt2 corner = [&absoluteVoxel, xAxis, &interpStart]()
		{
			if (xAxis)
			{
				interpStart = CardinalDirection::South;
				return absoluteVoxel;
			}
			else
			{
				interpStart = CardinalDirection::West;
				return NewInt2(absoluteVoxel.x + 1, absoluteVoxel.y);
			}
		}();

		const NewDouble2 cornerReal(
			static_cast<SNDouble>(corner.x),
			static_cast<WEDouble>(corner.y));

		// Bias the pivot towards the voxel center slightly to avoid Z-fighting with adjacent walls.
		const NewDouble2 voxelCenter = VoxelUtils::getVoxelCenter(absoluteVoxel);
		const NewDouble2 bias = (voxelCenter - cornerReal) * Constants::Epsilon;
		return cornerReal + bias;
	}();

	// Use the left perpendicular vector of the door's closed position as the 
	// fully open position.
	const NewDouble2 interpEnd = interpStart.leftPerp();

	// Actual position of the door in its rotation, represented as a vector.
	const NewDouble2 doorVec = interpStart.lerp(interpEnd, 1.0 - percentOpen).normalized();

	// Use back-face culling with swinging doors so it's not obstructing the player's
	// view as much when it's opening.
	const NewDouble3 absoluteEye = VoxelUtils::coordToNewPoint(camera.eye);
	const NewDouble2 eye2D(absoluteEye.x, absoluteEye.z);
	const bool isFrontFace = (eye2D - pivot).normalized().dot(doorVec.leftPerp()) > 0.0;

	if (isFrontFace)
	{
		// Vector cross product in 2D, returns a scalar.
		auto cross = [](const NewDouble2 &a, const NewDouble2 &b)
		{
			return (a.x * b.y) - (b.x * a.y);
		};

		// Solve line segment intersection between the incoming ray and the door.
		const NewDouble2 p1 = pivot;
		const NewDouble2 v1 = doorVec;
		const NewDouble2 p2 = nearPoint;
		const NewDouble2 v2 = farPoint - nearPoint;

		// Percent from p1 to (p1 + v1).
		const double t = cross(p2 - p1, v2) / cross(v1, v2);

		// See if the two line segments intersect.
		if ((t >= 0.0) && (t < 1.0))
		{
			// Hit.
			hit.point = p1 + (v1 * t);
			hit.innerZ = (hit.point - nearPoint).length();
			hit.u = t;
			hit.normal = [&v1]()
			{
				const NewDouble2 norm2D = v1.rightPerp();
				return Double3(norm2D.x, 0.0, norm2D.y);
			}();

			return true;
		}
		else
		{
			// No hit.
			return false;
		}
	}
	else
	{
		// Cull back face.
		return false;
	}
}

bool LegacyRendererUtils::findInitialDoorIntersection(const CoordInt2 &coord, ArenaTypes::DoorType doorType,
	double percentOpen, const NewDouble2 &nearPoint, const NewDouble2 &farPoint, const Camera &camera,
	const Ray &ray, const ChunkManager &chunkManager, RayHit &hit)
{
	// Determine which axis the door should open/close for (either X or Z).
	const bool xAxis = [&coord, &chunkManager]()
	{
		// Check adjacent voxels on the X axis for air.
		auto voxelIsAir = [&chunkManager](const CoordInt2 &checkCoord)
		{
			const Chunk *chunk = chunkManager.tryGetChunk(checkCoord.chunk);
			if (chunk != nullptr)
			{
				const VoxelInt2 &voxel = checkCoord.voxel;
				const Chunk::VoxelID voxelID = chunk->getVoxel(voxel.x, 1, voxel.y);
				const VoxelDefinition &voxelDef = chunk->getVoxelDef(voxelID);
				return voxelDef.type == ArenaTypes::VoxelType::None;
			}
			else
			{
				// Anything outside the level is considered air.
				return true;
			}
		};

		// If the two nearest X voxels are empty, return true.
		const CoordInt2 higherCoord = ChunkUtils::recalculateCoord(
			coord.chunk, VoxelInt2(coord.voxel.x + 1, coord.voxel.y));
		const CoordInt2 lowerCoord = ChunkUtils::recalculateCoord(
			coord.chunk, VoxelInt2(coord.voxel.x - 1, coord.voxel.y));
		return voxelIsAir(higherCoord) && voxelIsAir(lowerCoord);
	}();

	// If the current intersection surface is along one of the voxel's edges, treat the door
	// like a wall by basing intersection calculations on the far facing.
	const bool useFarFacing = [doorType, percentOpen]()
	{
		const bool isClosed = percentOpen == 0.0;
		return isClosed ||
			(doorType == ArenaTypes::DoorType::Sliding) ||
			(doorType == ArenaTypes::DoorType::Raising) ||
			(doorType == ArenaTypes::DoorType::Splitting);
	}();

	if (useFarFacing)
	{
		// Treat the door like a wall. Reuse the chasm facing code to find which face is intersected.
		const NewDouble3 absoluteEye = VoxelUtils::coordToNewPoint(camera.eye);
		const NewDouble2 absoluteEye2D(absoluteEye.x, absoluteEye.z);
		const VoxelFacing2D farFacing = SoftwareRenderer::getInitialChasmFarFacing(coord, absoluteEye2D, ray);
		const VoxelFacing2D doorFacing = xAxis ? VoxelFacing2D::PositiveX : VoxelFacing2D::PositiveZ;

		if (doorFacing == farFacing)
		{
			// The ray intersected the target facing. See if the door itself was intersected
			// and write out hit data based on the door type.
			const double farU = [&farPoint, xAxis]()
			{
				const double uVal = [&farPoint, xAxis]()
				{
					if (xAxis)
					{
						return Constants::JustBelowOne - (farPoint.y - std::floor(farPoint.y));
					}
					else
					{
						return farPoint.x - std::floor(farPoint.x);
					}
				}();

				return std::clamp(uVal, 0.0, Constants::JustBelowOne);
			}();

			if (doorType == ArenaTypes::DoorType::Swinging)
			{
				// Treat like a wall.
				hit.innerZ = (farPoint - nearPoint).length();
				hit.u = farU;
				hit.point = farPoint;
				hit.normal = -VoxelUtils::getNormal(farFacing);
				return true;
			}
			else if (doorType == ArenaTypes::DoorType::Sliding)
			{
				// If far U coordinate is within percent closed, it's a hit. At 100% open,
				// a sliding door is still partially visible.
				const double minVisible = ArenaRenderUtils::DOOR_MIN_VISIBLE;
				const double visibleAmount = 1.0 - ((1.0 - minVisible) * percentOpen);
				if (visibleAmount > farU)
				{
					hit.innerZ = (farPoint - nearPoint).length();
					hit.u = std::clamp(farU + (1.0 - visibleAmount), 0.0, Constants::JustBelowOne);
					hit.point = farPoint;
					hit.normal = -VoxelUtils::getNormal(farFacing);
					return true;
				}
				else
				{
					// No hit.
					return false;
				}
			}
			else if (doorType == ArenaTypes::DoorType::Raising)
			{
				// Raising doors are always hit.
				hit.innerZ = (farPoint - nearPoint).length();
				hit.u = farU;
				hit.point = farPoint;
				hit.normal = -VoxelUtils::getNormal(farFacing);
				return true;
			}
			else if (doorType == ArenaTypes::DoorType::Splitting)
			{
				// If far U coordinate is within percent closed on left or right half, it's a hit.
				// At 100% open, a splitting door is still partially visible.
				const double minVisible = ArenaRenderUtils::DOOR_MIN_VISIBLE;
				const bool leftHalf = farU < 0.50;
				const bool rightHalf = farU > 0.50;
				double leftVisAmount, rightVisAmount;
				const bool success = [percentOpen, farU, minVisible, leftHalf, rightHalf,
					&leftVisAmount, &rightVisAmount]()
				{
					if (leftHalf)
					{
						// Left half.
						leftVisAmount = 0.50 - ((0.50 - minVisible) * percentOpen);
						return farU <= leftVisAmount;
					}
					else if (rightHalf)
					{
						// Right half.
						rightVisAmount = 0.50 + ((0.50 - minVisible) * percentOpen);
						return farU >= rightVisAmount;
					}
					else
					{
						// Midpoint (only when door is completely closed).
						return percentOpen == 0.0;
					}
				}();

				if (success)
				{
					// Hit.
					hit.innerZ = (farPoint - nearPoint).length();
					hit.u = [farU, leftHalf, rightHalf, leftVisAmount, rightVisAmount]()
					{
						const double u = [farU, leftHalf, rightHalf, leftVisAmount, rightVisAmount]()
						{
							if (leftHalf)
							{
								return (farU + 0.50) - leftVisAmount;
							}
							else if (rightHalf)
							{
								return (farU + 0.50) - rightVisAmount;
							}
							else
							{
								// Midpoint.
								return 0.50;
							}
						}();

						return std::clamp(u, 0.0, Constants::JustBelowOne);
					}();

					hit.point = farPoint;
					hit.normal = -VoxelUtils::getNormal(farFacing);

					return true;
				}
				else
				{
					// No hit.
					return false;
				}
			}
			else
			{
				// Invalid door type.
				return false;
			}
		}
		else
		{
			// No hit.
			return false;
		}
	}
	else if (doorType == ArenaTypes::DoorType::Swinging)
	{
		return SoftwareRenderer::findInitialSwingingDoorIntersection(coord, percentOpen,
			nearPoint, farPoint, xAxis, camera, ray, hit);
	}
	else
	{
		// Invalid door type.
		return false;
	}
}

bool LegacyRendererUtils::findSwingingDoorIntersection(const CoordInt2 &coord, double percentOpen,
	VoxelFacing2D nearFacing, const NewDouble2 &nearPoint, const NewDouble2 &farPoint,
	double nearU, RayHit &hit)
{
	const NewInt2 absoluteVoxel = VoxelUtils::coordToNewVoxel(coord);

	// Decide which corner the door's hinge will be in, and create the line segment
	// that will be rotated based on percent open.
	NewDouble2 interpStart;
	const NewDouble2 pivot = [&absoluteVoxel, nearFacing, &interpStart]()
	{
		const NewInt2 corner = [&absoluteVoxel, nearFacing, &interpStart]()
		{
			if (nearFacing == VoxelFacing2D::PositiveX)
			{
				interpStart = CardinalDirection::North;
				return NewInt2(absoluteVoxel.x + 1, absoluteVoxel.y + 1);
			}
			else if (nearFacing == VoxelFacing2D::NegativeX)
			{
				interpStart = CardinalDirection::South;
				return absoluteVoxel;
			}
			else if (nearFacing == VoxelFacing2D::PositiveZ)
			{
				interpStart = CardinalDirection::East;
				return NewInt2(absoluteVoxel.x, absoluteVoxel.y + 1);
			}
			else if (nearFacing == VoxelFacing2D::NegativeZ)
			{
				interpStart = CardinalDirection::West;
				return NewInt2(absoluteVoxel.x + 1, absoluteVoxel.y);
			}
			else
			{
				DebugUnhandledReturnMsg(NewInt2, std::to_string(static_cast<int>(nearFacing)));
			}
		}();

		const NewDouble2 cornerReal(
			static_cast<SNDouble>(corner.x),
			static_cast<WEDouble>(corner.y));

		// Bias the pivot towards the voxel center slightly to avoid Z-fighting with adjacent walls.
		const NewDouble2 voxelCenter = VoxelUtils::getVoxelCenter(absoluteVoxel);
		const NewDouble2 bias = (voxelCenter - cornerReal) * Constants::Epsilon;
		return cornerReal + bias;
	}();

	// Use the left perpendicular vector of the door's closed position as the 
	// fully open position.
	const NewDouble2 interpEnd = interpStart.leftPerp();

	// Actual position of the door in its rotation, represented as a vector.
	const NewDouble2 doorVec = interpStart.lerp(interpEnd, 1.0 - percentOpen).normalized();

	// Vector cross product in 2D, returns a scalar.
	auto cross = [](const NewDouble2 &a, const NewDouble2 &b)
	{
		return (a.x * b.y) - (b.x * a.y);
	};

	// Solve line segment intersection between the incoming ray and the door.
	const NewDouble2 p1 = pivot;
	const NewDouble2 v1 = doorVec;
	const NewDouble2 p2 = nearPoint;
	const NewDouble2 v2 = farPoint - nearPoint;

	// Percent from p1 to (p1 + v1).
	const double t = cross(p2 - p1, v2) / cross(v1, v2);

	// See if the two line segments intersect.
	if ((t >= 0.0) && (t < 1.0))
	{
		// Hit.
		hit.point = p1 + (v1 * t);
		hit.innerZ = (hit.point - nearPoint).length();
		hit.u = t;
		hit.normal = [&v1]()
		{
			const NewDouble2 norm2D = v1.rightPerp();
			return Double3(norm2D.x, 0.0, norm2D.y);
		}();

		return true;
	}
	else
	{
		// No hit.
		return false;
	}
}

bool LegacyRendererUtils::findDoorIntersection(const CoordInt2 &coord, ArenaTypes::DoorType doorType,
	double percentOpen, VoxelFacing2D nearFacing, const NewDouble2 &nearPoint, const NewDouble2 &farPoint,
	double nearU, RayHit &hit)
{
	// Check trivial case first: whether the door is closed.
	const bool isClosed = percentOpen == 0.0;

	if (isClosed)
	{
		// Treat like a wall.
		hit.innerZ = 0.0;
		hit.u = nearU;
		hit.point = nearPoint;
		hit.normal = VoxelUtils::getNormal(nearFacing);
		return true;
	}
	else if (doorType == ArenaTypes::DoorType::Swinging)
	{
		return SoftwareRenderer::findSwingingDoorIntersection(coord, percentOpen,
			nearFacing, nearPoint, farPoint, nearU, hit);
	}
	else if (doorType == ArenaTypes::DoorType::Sliding)
	{
		// If near U coordinate is within percent closed, it's a hit. At 100% open,
		// a sliding door is still partially visible.
		const double minVisible = ArenaRenderUtils::DOOR_MIN_VISIBLE;
		const double visibleAmount = 1.0 - ((1.0 - minVisible) * percentOpen);
		if (visibleAmount > nearU)
		{
			hit.innerZ = 0.0;
			hit.u = std::clamp(nearU + (1.0 - visibleAmount), 0.0, Constants::JustBelowOne);
			hit.point = nearPoint;
			hit.normal = VoxelUtils::getNormal(nearFacing);
			return true;
		}
		else
		{
			// No hit.
			return false;
		}
	}
	else if (doorType == ArenaTypes::DoorType::Raising)
	{
		// Raising doors are always hit.
		hit.innerZ = 0.0;
		hit.u = nearU;
		hit.point = nearPoint;
		hit.normal = VoxelUtils::getNormal(nearFacing);
		return true;
	}
	else if (doorType == ArenaTypes::DoorType::Splitting)
	{
		// If near U coordinate is within percent closed on left or right half, it's a hit.
		// At 100% open, a splitting door is still partially visible.
		const double minVisible = ArenaRenderUtils::DOOR_MIN_VISIBLE;
		const bool leftHalf = nearU < 0.50;
		const bool rightHalf = nearU > 0.50;
		double leftVisAmount, rightVisAmount;
		const bool success = [percentOpen, nearU, minVisible, leftHalf, rightHalf,
			&leftVisAmount, &rightVisAmount]()
		{
			if (leftHalf)
			{
				// Left half.
				leftVisAmount = 0.50 - ((0.50 - minVisible) * percentOpen);
				return nearU <= leftVisAmount;
			}
			else if (rightHalf)
			{
				// Right half.
				rightVisAmount = 0.50 + ((0.50 - minVisible) * percentOpen);
				return nearU >= rightVisAmount;
			}
			else
			{
				// Midpoint (only when door is completely closed).
				return percentOpen == 0.0;
			}
		}();

		if (success)
		{
			// Hit.
			hit.innerZ = 0.0;
			hit.u = [nearU, leftHalf, rightHalf, leftVisAmount, rightVisAmount]()
			{
				const double u = [nearU, leftHalf, rightHalf, leftVisAmount, rightVisAmount]()
				{
					if (leftHalf)
					{
						return (nearU + 0.50) - leftVisAmount;
					}
					else if (rightHalf)
					{
						return (nearU + 0.50) - rightVisAmount;
					}
					else
					{
						// Midpoint.
						return 0.50;
					}
				}();

				return std::clamp(u, 0.0, Constants::JustBelowOne);
			}();

			hit.point = nearPoint;
			hit.normal = VoxelUtils::getNormal(nearFacing);

			return true;
		}
		else
		{
			// No hit.
			return false;
		}
	}
	else
	{
		// Invalid door type.
		return false;
	}
}*/

template <int TextureWidth, int TextureHeight>
uint8_t LegacyRendererUtils::sampleFogMatrixTexture(const ArenaRenderUtils::FogMatrix &fogMatrix, double u, double v)
{
	static_assert((TextureWidth * TextureHeight) == std::tuple_size_v<std::remove_reference_t<decltype(fogMatrix)>>);
	constexpr double textureWidthReal = static_cast<double>(TextureWidth);
	constexpr double textureHeightReal = static_cast<double>(TextureHeight);
	const double texelWidth = 1.0 / textureWidthReal;
	const double texelHeight = 1.0 / textureHeightReal;
	const double halfTexelWidth = texelWidth * 0.50;
	const double halfTexelHeight = texelHeight * 0.50;

	// Neighboring percents that might land in an adjacent texel.
	const double uLow = std::max(u - halfTexelWidth, 0.0);
	const double uHigh = std::min(u + halfTexelWidth, Constants::JustBelowOne);
	const double vLow = std::max(v - halfTexelHeight, 0.0);
	const double vHigh = std::min(v + halfTexelHeight, Constants::JustBelowOne);

	const double uLowWidth = uLow * textureWidthReal;
	const double vLowHeight = vLow * textureHeightReal;
	const double uLowPercent = 1.0 - (uLowWidth - std::floor(uLowWidth));
	const double uHighPercent = 1.0 - uLowPercent;
	const double vLowPercent = 1.0 - (vLowHeight - std::floor(vLowHeight));
	const double vHighPercent = 1.0 - vLowPercent;
	const double tlPercent = uLowPercent * vLowPercent;
	const double trPercent = uHighPercent * vLowPercent;
	const double blPercent = uLowPercent * vHighPercent;
	const double brPercent = uHighPercent * vHighPercent;
	const int textureXL = std::clamp(static_cast<int>(uLow * textureWidthReal), 0, TextureWidth - 1);
	const int textureXR = std::clamp(static_cast<int>(uHigh * textureWidthReal), 0, TextureWidth - 1);
	const int textureYT = std::clamp(static_cast<int>(vLow * textureHeightReal), 0, TextureHeight - 1);
	const int textureYB = std::clamp(static_cast<int>(vHigh * textureHeightReal), 0, TextureHeight - 1);
	const int textureIndexTL = textureXL + (textureYT * TextureWidth);
	const int textureIndexTR = textureXR + (textureYT * TextureWidth);
	const int textureIndexBL = textureXL + (textureYB * TextureWidth);
	const int textureIndexBR = textureXR + (textureYB * TextureWidth);

	const uint8_t texelTL = fogMatrix[textureIndexTL];
	const uint8_t texelTR = fogMatrix[textureIndexTR];
	const uint8_t texelBL = fogMatrix[textureIndexBL];
	const uint8_t texelBR = fogMatrix[textureIndexBR];

	constexpr int percentMultiplier = 100;
	constexpr double percentMultiplierReal = static_cast<double>(percentMultiplier);
	const uint16_t tlPercentInteger = static_cast<uint16_t>(tlPercent * percentMultiplierReal);
	const uint16_t trPercentInteger = static_cast<uint16_t>(trPercent * percentMultiplierReal);
	const uint16_t blPercentInteger = static_cast<uint16_t>(blPercent * percentMultiplierReal);
	const uint16_t brPercentInteger = static_cast<uint16_t>(brPercent * percentMultiplierReal);

	const uint16_t texelTLScaled = texelTL * tlPercentInteger;
	const uint16_t texelTRScaled = texelTR * trPercentInteger;
	const uint16_t texelBLScaled = texelBL * blPercentInteger;
	const uint16_t texelBRScaled = texelBR * brPercentInteger;

	const uint16_t texelSumScaled = texelTLScaled + texelTRScaled + texelBLScaled + texelBRScaled;
	return static_cast<uint8_t>(texelSumScaled / percentMultiplier);
}

/*void LegacyRendererUtils::sampleChasmTexture(const ChasmTexture &texture, double screenXPercent, double screenYPercent,
	double *r, double *g, double *b)
{
	const double textureWidthReal = static_cast<double>(texture.width);
	const double textureHeightReal = static_cast<double>(texture.height);

	// @todo: this is just the first implementation of chasm texturing. There is apparently no
	// perfect solution, so there will probably be graphics options to tweak how exactly this
	// sampling is done (stretch, tile, etc.).
	const int textureX = static_cast<int>(screenXPercent * textureWidthReal);
	const int textureY = static_cast<int>((screenYPercent * 2.0) * textureHeightReal) % texture.height;
	const int textureIndex = textureX + (textureY * texture.width);

	const ChasmTexel &texel = texture.texels[textureIndex];
	*r = texel.r;
	*g = texel.g;
	*b = texel.b;
}*/

/*bool LegacyRendererUtils::tryGetEntitySelectionData(const Double2 &uv, const TextureAssetReference &textureAssetRef,
	bool flipped, bool reflective, bool pixelPerfect, const Palette &palette, bool *outIsSelected)
{
	// Branch depending on whether the selection request needs to include texture data.
	if (pixelPerfect)
	{
		// Get the texture list from the texture group at the given animation state and angle.
		const FlatTexture &texture = this->entityTextures.getTexture(textureAssetRef, flipped, reflective);

		// Convert texture coordinates to a texture index. Don't need to clamp; just return
		// failure if it's out-of-bounds.
		const int textureX = static_cast<int>(uv.x * static_cast<double>(texture.width));
		const int textureY = static_cast<int>(uv.y * static_cast<double>(texture.height));

		if ((textureX < 0) || (textureX >= texture.width) ||
			(textureY < 0) || (textureY >= texture.height))
		{
			// Outside the texture.
			return false;
		}

		const int textureIndex = textureX + (textureY * texture.width);

		// Check if the texel is non-transparent.
		const FlatTexel &texel = texture.texels[textureIndex];
		const Color &texelColor = palette[texel.value];
		*outIsSelected = texelColor.a > 0;
		return true;
	}
	else
	{
		// If not pixel perfect, the entity's projected rectangle is hit if the texture coordinates
		// are valid.
		const bool withinEntity = (uv.x >= 0.0) && (uv.x <= 1.0) && (uv.y >= 0.0) && (uv.y <= 1.0);
		*outIsSelected = withinEntity;
		return true;
	}
}*/

Double3 LegacyRendererUtils::screenPointToRay(double xPercent, double yPercent, const Double3 &cameraDirection,
	double fovY, double aspect)
{
	// The basic components are the forward, up, and right vectors.
	const Double3 up = Double3::UnitY;
	const Double3 right = cameraDirection.cross(up).normalized();
	const Double3 forward = up.cross(right).normalized();

	// Building blocks of the ray direction. Up is reversed because y=0 is at the top
	// of the screen.
	const double rightPercent = ((xPercent * 2.0) - 1.0) * aspect;

	// Subtract y-shear from the Y percent because Y coordinates on-screen are reversed.
	const Radians yAngleRadians = cameraDirection.getYAngleRadians();
	const double zoom = MathUtils::verticalFovToZoom(fovY);
	const double yShear = RendererUtils::getYShear(yAngleRadians, zoom);
	const double upPercent = (((yPercent - yShear) * 2.0) - 1.0) / ArenaRenderUtils::TALL_PIXEL_RATIO;

	// Combine the various components to get the final vector
	const Double3 forwardComponent = forward * zoom;
	const Double3 rightComponent = right * rightPercent;
	const Double3 upComponent = up * upPercent;
	return (forwardComponent + rightComponent - upComponent).normalized();
}
