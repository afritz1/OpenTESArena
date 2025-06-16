#include "ArenaMeshUtils.h"
#include "../Math/Constants.h"
#include "../Voxels/VoxelFacing.h"

ArenaShapeInitCache::ArenaShapeInitCache()
{
	this->boxWidth = 0.0;
	this->boxHeight = 0.0;
	this->boxDepth = 0.0;
	this->boxYOffset = 0.0;
	this->boxYRotation = 0.0;

	this->positions.fill(0.0);
	this->normals.fill(0.0);
	this->texCoords.fill(0.0);

	this->indices0.fill(-1);
	this->indices1.fill(-1);
	this->indices2.fill(-1);
	this->indicesPtrs = { &this->indices0, &this->indices1, &this->indices2 };

	this->facings0.fill(static_cast<VoxelFacing3D>(-1));
	this->facings1.fill(static_cast<VoxelFacing3D>(-1));
	this->facings2.fill(static_cast<VoxelFacing3D>(-1));
	this->facingsPtrs = { &this->facings0, &this->facings1, &this->facings2 };

	this->positionsView.init(this->positions);
	this->normalsView.init(this->normals);
	this->texCoordsView.init(this->texCoords);

	this->indices0View.init(this->indices0);
	this->indices1View.init(this->indices1);
	this->indices2View.init(this->indices2);

	this->facings0View.init(this->facings0);
	this->facings1View.init(this->facings1);
	this->facings2View.init(this->facings2);
}

void ArenaShapeInitCache::initDefaultBoxValues()
{
	this->boxWidth = 1.0;
	this->boxHeight = 1.0;
	this->boxDepth = 1.0;
	this->boxYOffset = 0.0;
	this->boxYRotation = 0.0;
}

void ArenaShapeInitCache::initRaisedBoxValues(double height, double yOffset)
{
	this->boxWidth = 1.0;
	this->boxHeight = height;
	this->boxDepth = 1.0;
	this->boxYOffset = yOffset;
	this->boxYRotation = 0.0;
}

void ArenaShapeInitCache::initChasmBoxValues(bool isDryChasm)
{
	// Offset below the chasm floor so the collider isn't infinitely thin.
	// @todo: this doesn't seem right for wet chasms
	this->boxWidth = 1.0;
	this->boxHeight = 0.10;
	if (!isDryChasm)
	{
		this->boxHeight += 1.0 - ArenaChasmUtils::DEFAULT_HEIGHT;
	}

	this->boxDepth = 1.0;
	this->boxYOffset = -0.10;
	this->boxYRotation = 0.0;
}

void ArenaShapeInitCache::initDiagonalBoxValues(bool isRightDiag)
{
	constexpr Radians diagonalAngle = Constants::Pi / 4.0;
	constexpr double diagonalThickness = 0.050; // Arbitrary thin wall thickness
	static_assert(diagonalThickness > (Physics::BoxConvexRadius * 2.0));

	this->boxWidth = Constants::Sqrt2 - diagonalThickness; // Fit the edges of the voxel exactly
	this->boxHeight = 1.0;
	this->boxDepth = diagonalThickness;
	this->boxYOffset = 0.0;
	this->boxYRotation = isRightDiag ? -diagonalAngle : diagonalAngle;
}

void ArenaMeshUtils::writeWallRendererGeometryBuffers(BufferView<double> outPositions, BufferView<double> outNormals, BufferView<double> outTexCoords)
{
	constexpr ArenaVoxelType voxelType = ArenaVoxelType::Wall;

	// One quad per face (results in duplication; necessary for correct texture mapping).
	constexpr std::array<double, GetRendererVertexPositionComponentCount(voxelType)> positions =
	{
		// X=0
		0.0, 1.0, 0.0,
		0.0, 0.0, 0.0,
		0.0, 0.0, 1.0,
		0.0, 1.0, 1.0,
		// X=1
		1.0, 1.0, 1.0,
		1.0, 0.0, 1.0,
		1.0, 0.0, 0.0,
		1.0, 1.0, 0.0,
		// Y=0
		0.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		// Y=1
		0.0, 1.0, 1.0,
		1.0, 1.0, 1.0,
		1.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		// Z=0
		1.0, 1.0, 0.0,
		1.0, 0.0, 0.0,
		0.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		// Z=1
		0.0, 1.0, 1.0,
		0.0, 0.0, 1.0,
		1.0, 0.0, 1.0,
		1.0, 1.0, 1.0
	};

	constexpr std::array<double, GetRendererVertexNormalComponentCount(voxelType)> normals =
	{
		// X=0
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		// X=1
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		// Y=0
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0,
		// Y=1
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		// Z=0
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		// Z=1
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0
	};

	constexpr std::array<double, GetRendererVertexTexCoordComponentCount(voxelType)> texCoords =
	{
		// X=0
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// X=1
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// Y=0
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// Y=1
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// Z=0
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// Z=1
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0
	};

	std::copy(positions.begin(), positions.end(), outPositions.begin());
	std::copy(normals.begin(), normals.end(), outNormals.begin());
	std::copy(texCoords.begin(), texCoords.end(), outTexCoords.begin());
}

void ArenaMeshUtils::writeWallRendererIndexBuffers(BufferView<int32_t> outSideIndices, BufferView<int32_t> outBottomIndices, BufferView<int32_t> outTopIndices)
{
	constexpr ArenaVoxelType voxelType = ArenaVoxelType::Wall;

	constexpr std::array<int32_t, GetIndexBufferIndexCount(voxelType, 0)> sideIndices =
	{
		// X=0
		0, 1, 2,
		2, 3, 0,
		// X=1
		4, 5, 6,
		6, 7, 4,
		// Z=0
		16, 17, 18,
		18, 19, 16,
		// Z=1
		20, 21, 22,
		22, 23, 20
	};

	constexpr std::array<int32_t, GetIndexBufferIndexCount(voxelType, 1)> bottomIndices =
	{
		// Y=0
		8, 9, 10,
		10, 11, 8
	};

	constexpr std::array<int32_t, GetIndexBufferIndexCount(voxelType, 2)> topIndices =
	{
		// Y=1
		12, 13, 14,
		14, 15, 12
	};

	std::copy(sideIndices.begin(), sideIndices.end(), outSideIndices.begin());
	std::copy(bottomIndices.begin(), bottomIndices.end(), outBottomIndices.begin());
	std::copy(topIndices.begin(), topIndices.end(), outTopIndices.begin());
}

void ArenaMeshUtils::writeWallFacingBuffers(BufferView<VoxelFacing3D> outSideFacings, BufferView<VoxelFacing3D> outBottomFacings, BufferView<VoxelFacing3D> outTopFacings)
{
	constexpr ArenaVoxelType voxelType = ArenaVoxelType::Wall;
	constexpr std::array<VoxelFacing3D, GetFacingBufferFaceCount(voxelType, 0)> sideFacings =
	{
		VoxelFacing3D::PositiveX,
		VoxelFacing3D::NegativeX,
		VoxelFacing3D::PositiveZ,
		VoxelFacing3D::NegativeZ
	};

	constexpr std::array<VoxelFacing3D, GetFacingBufferFaceCount(voxelType, 1)> bottomFacings =
	{
		VoxelFacing3D::NegativeY
	};

	constexpr std::array<VoxelFacing3D, GetFacingBufferFaceCount(voxelType, 2)> topFacings =
	{
		VoxelFacing3D::PositiveY
	};

	std::copy(sideFacings.begin(), sideFacings.end(), outSideFacings.begin());
	std::copy(bottomFacings.begin(), bottomFacings.end(), outBottomFacings.begin());
	std::copy(topFacings.begin(), topFacings.end(), outTopFacings.begin());
}

void ArenaMeshUtils::writeFloorRendererGeometryBuffers(BufferView<double> outPositions, BufferView<double> outNormals,
	BufferView<double> outTexCoords)
{
	constexpr ArenaVoxelType voxelType = ArenaVoxelType::Floor;
	constexpr std::array<double, GetRendererVertexPositionComponentCount(voxelType)> positions =
	{
		// Y=1
		0.0, 1.0, 1.0,
		1.0, 1.0, 1.0,
		1.0, 1.0, 0.0,
		0.0, 1.0, 0.0
	};

	constexpr std::array<double, GetRendererVertexNormalComponentCount(voxelType)> normals =
	{
		// Y=1
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0
	};

	constexpr std::array<double, GetRendererVertexTexCoordComponentCount(voxelType)> texCoords =
	{
		// Y=1
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		0.0, 0.0
	};

	std::copy(positions.begin(), positions.end(), outPositions.begin());
	std::copy(normals.begin(), normals.end(), outNormals.begin());
	std::copy(texCoords.begin(), texCoords.end(), outTexCoords.begin());
}

void ArenaMeshUtils::writeFloorRendererIndexBuffers(BufferView<int32_t> outIndices)
{
	constexpr ArenaVoxelType voxelType = ArenaVoxelType::Floor;

	constexpr std::array<int32_t, GetIndexBufferIndexCount(voxelType, 0)> indices =
	{
		// Y=1
		0, 1, 2,
		2, 3, 0
	};

	std::copy(indices.begin(), indices.end(), outIndices.begin());
}

void ArenaMeshUtils::writeFloorFacingBuffers(BufferView<VoxelFacing3D> outFacings)
{
	constexpr ArenaVoxelType voxelType = ArenaVoxelType::Floor;
	constexpr std::array<VoxelFacing3D, GetFacingBufferFaceCount(voxelType, 0)> facings =
	{
		VoxelFacing3D::PositiveY
	};

	std::copy(facings.begin(), facings.end(), outFacings.begin());
}

void ArenaMeshUtils::writeCeilingRendererGeometryBuffers(BufferView<double> outPositions, BufferView<double> outNormals,
	BufferView<double> outTexCoords)
{
	constexpr ArenaVoxelType voxelType = ArenaVoxelType::Ceiling;
	constexpr std::array<double, GetRendererVertexPositionComponentCount(voxelType)> positions =
	{
		// Y=0
		0.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 1.0,
		0.0, 0.0, 1.0
	};

	constexpr std::array<double, GetRendererVertexNormalComponentCount(voxelType)> normals =
	{
		// Y=0
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0
	};

	constexpr std::array<double, GetRendererVertexTexCoordComponentCount(voxelType)> texCoords =
	{
		// Y=0
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0
	};

	std::copy(positions.begin(), positions.end(), outPositions.begin());
	std::copy(normals.begin(), normals.end(), outNormals.begin());
	std::copy(texCoords.begin(), texCoords.end(), outTexCoords.begin());
}

void ArenaMeshUtils::writeCeilingRendererIndexBuffers(BufferView<int32_t> outIndices)
{
	constexpr ArenaVoxelType voxelType = ArenaVoxelType::Ceiling;

	constexpr std::array<int32_t, GetIndexBufferIndexCount(voxelType, 0)> indices =
	{
		// Y=0
		0, 1, 2,
		2, 3, 0
	};

	std::copy(indices.begin(), indices.end(), outIndices.begin());
}

void ArenaMeshUtils::writeCeilingFacingBuffers(BufferView<VoxelFacing3D> outFacings)
{
	constexpr ArenaVoxelType voxelType = ArenaVoxelType::Ceiling;
	constexpr std::array<VoxelFacing3D, GetFacingBufferFaceCount(voxelType, 0)> facings =
	{
		VoxelFacing3D::NegativeY
	};

	std::copy(facings.begin(), facings.end(), outFacings.begin());
}

void ArenaMeshUtils::writeRaisedRendererGeometryBuffers(double yOffset, double ySize, double vBottom, double vTop,
	BufferView<double> outPositions, BufferView<double> outNormals, BufferView<double> outTexCoords)
{
	constexpr ArenaVoxelType voxelType = ArenaVoxelType::Raised;
	const double yBottom = yOffset;
	const double yTop = yOffset + ySize;

	// One quad per face (results in duplication; necessary for correct texture mapping).
	const std::array<double, GetRendererVertexPositionComponentCount(voxelType)> positions =
	{
		// X=0
		0.0, yTop, 0.0,
		0.0, yBottom, 0.0,
		0.0, yBottom, 1.0,
		0.0, yTop, 1.0,
		// X=1
		1.0, yTop, 1.0,
		1.0, yBottom, 1.0,
		1.0, yBottom, 0.0,
		1.0, yTop, 0.0,
		// Y=0
		0.0, yBottom, 0.0,
		1.0, yBottom, 0.0,
		1.0, yBottom, 1.0,
		0.0, yBottom, 1.0,
		// Y=1
		0.0, yTop, 1.0,
		1.0, yTop, 1.0,
		1.0, yTop, 0.0,
		0.0, yTop, 0.0,
		// Z=0
		1.0, yTop, 0.0,
		1.0, yBottom, 0.0,
		0.0, yBottom, 0.0,
		0.0, yTop, 0.0,
		// Z=1
		0.0, yTop, 1.0,
		0.0, yBottom, 1.0,
		1.0, yBottom, 1.0,
		1.0, yTop, 1.0
	};

	constexpr std::array<double, GetRendererVertexNormalComponentCount(voxelType)> normals =
	{
		// X=0
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		// X=1
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		// Y=0
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0,
		// Y=1
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		// Z=0
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		// Z=1
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0
	};

	const std::array<double, GetRendererVertexTexCoordComponentCount(voxelType)> texCoords =
	{
		// X=0
		0.0, vTop,
		0.0, vBottom,
		1.0, vBottom,
		1.0, vTop,
		// X=1
		0.0, vTop,
		0.0, vBottom,
		1.0, vBottom,
		1.0, vTop,
		// Y=0
		1.0, 1.0,
		0.0, 1.0,
		0.0, 0.0,
		1.0, 0.0,
		// Y=1
		1.0, 1.0,
		0.0, 1.0,
		0.0, 0.0,
		1.0, 0.0,
		// Z=0
		0.0, vTop,
		0.0, vBottom,
		1.0, vBottom,
		1.0, vTop,
		// Z=1
		0.0, vTop,
		0.0, vBottom,
		1.0, vBottom,
		1.0, vTop
	};

	std::copy(positions.begin(), positions.end(), outPositions.begin());
	std::copy(normals.begin(), normals.end(), outNormals.begin());
	std::copy(texCoords.begin(), texCoords.end(), outTexCoords.begin());
}

void ArenaMeshUtils::writeRaisedRendererIndexBuffers(BufferView<int32_t> outSideIndices, BufferView<int32_t> outBottomIndices, BufferView<int32_t> outTopIndices)
{
	constexpr ArenaVoxelType voxelType = ArenaVoxelType::Raised;

	constexpr std::array<int32_t, GetIndexBufferIndexCount(voxelType, 0)> sideIndices =
	{
		// X=0
		0, 1, 2,
		2, 3, 0,
		// X=1
		4, 5, 6,
		6, 7, 4,
		// Z=0
		16, 17, 18,
		18, 19, 16,
		// Z=1
		20, 21, 22,
		22, 23, 20
	};

	constexpr std::array<int32_t, GetIndexBufferIndexCount(voxelType, 1)> bottomIndices =
	{
		// Y=0
		8, 9, 10,
		10, 11, 8
	};

	constexpr std::array<int32_t, GetIndexBufferIndexCount(voxelType, 2)> topIndices =
	{
		// Y=1
		12, 13, 14,
		14, 15, 12
	};

	std::copy(sideIndices.begin(), sideIndices.end(), outSideIndices.begin());
	std::copy(bottomIndices.begin(), bottomIndices.end(), outBottomIndices.begin());
	std::copy(topIndices.begin(), topIndices.end(), outTopIndices.begin());
}

void ArenaMeshUtils::writeDiagonalRendererGeometryBuffers(bool type1, BufferView<double> outPositions,
	BufferView<double> outNormals, BufferView<double> outTexCoords)
{
	constexpr ArenaVoxelType voxelType = ArenaVoxelType::Diagonal;
	constexpr int positionComponentCount = GetRendererVertexPositionComponentCount(voxelType);
	constexpr std::array<double, positionComponentCount> type1Positions =
	{
		// Front
		0.0, 1.0, 0.0,
		0.0, 0.0, 0.0,
		1.0, 0.0, 1.0,
		1.0, 1.0, 1.0,

		// Back
		1.0, 1.0, 1.0,
		1.0, 0.0, 1.0,
		0.0, 0.0, 0.0,
		0.0, 1.0, 0.0
	};

	constexpr std::array<double, positionComponentCount> type2Positions =
	{
		// Front
		1.0, 1.0, 0.0,
		1.0, 0.0, 0.0,
		0.0, 0.0, 1.0,
		0.0, 1.0, 1.0,

		// Back
		0.0, 1.0, 1.0,
		0.0, 0.0, 1.0,
		1.0, 0.0, 0.0,
		1.0, 1.0, 0.0
	};

	constexpr int normalComponentsCount = GetRendererVertexNormalComponentCount(voxelType);
	constexpr double halfSqrt2 = Constants::HalfSqrt2;
	constexpr std::array<double, normalComponentsCount> type1Normals =
	{
		// Front
		-halfSqrt2, 0.0, halfSqrt2,
		-halfSqrt2, 0.0, halfSqrt2,
		-halfSqrt2, 0.0, halfSqrt2,
		-halfSqrt2, 0.0, halfSqrt2,

		// Back
		halfSqrt2, 0.0, -halfSqrt2,
		halfSqrt2, 0.0, -halfSqrt2,
		halfSqrt2, 0.0, -halfSqrt2,
		halfSqrt2, 0.0, -halfSqrt2
	};

	constexpr std::array<double, normalComponentsCount> type2Normals =
	{
		// Front
		-halfSqrt2, 0.0, -halfSqrt2,
		-halfSqrt2, 0.0, -halfSqrt2,
		-halfSqrt2, 0.0, -halfSqrt2,
		-halfSqrt2, 0.0, -halfSqrt2,

		// Back
		halfSqrt2, 0.0, halfSqrt2,
		halfSqrt2, 0.0, halfSqrt2,
		halfSqrt2, 0.0, halfSqrt2,
		halfSqrt2, 0.0, halfSqrt2
	};

	constexpr std::array<double, GetRendererVertexTexCoordComponentCount(voxelType)> texCoords =
	{
		// Front
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,

		// Back
		1.0, 0.0,
		1.0, 1.0,
		0.0, 1.0,
		0.0, 0.0
	};

	const auto &positions = type1 ? type1Positions : type2Positions;
	const auto &normals = type1 ? type1Normals : type2Normals;

	std::copy(positions.begin(), positions.end(), outPositions.begin());
	std::copy(normals.begin(), normals.end(), outNormals.begin());
	std::copy(texCoords.begin(), texCoords.end(), outTexCoords.begin());
}

void ArenaMeshUtils::writeDiagonalRendererIndexBuffers(BufferView<int32_t> outIndices)
{
	constexpr ArenaVoxelType voxelType = ArenaVoxelType::Diagonal;

	constexpr std::array<int32_t, GetIndexBufferIndexCount(voxelType, 0)> indices =
	{
		// Front
		0, 1, 2,
		2, 3, 0,

		// Back
		4, 5, 6,
		6, 7, 4
	};

	std::copy(indices.begin(), indices.end(), outIndices.begin());
}

void ArenaMeshUtils::writeTransparentWallRendererGeometryBuffers(BufferView<double> outPositions,
	BufferView<double> outNormals, BufferView<double> outTexCoords)
{
	constexpr ArenaVoxelType voxelType = ArenaVoxelType::TransparentWall;

	// One quad per face (results in duplication; necessary for correct texture mapping).
	constexpr std::array<double, GetRendererVertexPositionComponentCount(voxelType)> positions =
	{
		// X=0
		0.0, 1.0, 0.0,
		0.0, 0.0, 0.0,
		0.0, 0.0, 1.0,
		0.0, 1.0, 1.0,
		// X=1
		1.0, 1.0, 1.0,
		1.0, 0.0, 1.0,
		1.0, 0.0, 0.0,
		1.0, 1.0, 0.0,
		// Z=0
		1.0, 1.0, 0.0,
		1.0, 0.0, 0.0,
		0.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		// Z=1
		0.0, 1.0, 1.0,
		0.0, 0.0, 1.0,
		1.0, 0.0, 1.0,
		1.0, 1.0, 1.0
	};

	constexpr std::array<double, GetRendererVertexNormalComponentCount(voxelType)> normals =
	{
		// X=0
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		// X=1
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		// Z=0
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		// Z=1
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0
	};

	constexpr std::array<double, GetRendererVertexTexCoordComponentCount(voxelType)> texCoords =
	{
		// X=0
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// X=1
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// Z=0
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// Z=1
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0
	};

	std::copy(positions.begin(), positions.end(), outPositions.begin());
	std::copy(normals.begin(), normals.end(), outNormals.begin());
	std::copy(texCoords.begin(), texCoords.end(), outTexCoords.begin());
}

void ArenaMeshUtils::writeTransparentWallRendererIndexBuffers(BufferView<int32_t> outIndices)
{
	constexpr ArenaVoxelType voxelType = ArenaVoxelType::TransparentWall;

	constexpr std::array<int32_t, GetIndexBufferIndexCount(voxelType, 0)> indices =
	{
		// X=0
		0, 1, 2,
		2, 3, 0,
		// X=1
		4, 5, 6,
		6, 7, 4,
		// Z=0
		8, 9, 10,
		10, 11, 8,
		// Z=1
		12, 13, 14,
		14, 15, 12
	};

	std::copy(indices.begin(), indices.end(), outIndices.begin());
}

void ArenaMeshUtils::writeEdgeRendererGeometryBuffers(VoxelFacing2D facing, double yOffset, bool flipped,
	BufferView<double> outPositions, BufferView<double> outNormals, BufferView<double> outTexCoords)
{
	constexpr ArenaVoxelType voxelType = ArenaVoxelType::Edge;

	// Bias the geometry towards the center of the voxel to avoid Z-fighting.
	constexpr double xBiasMin = Constants::Epsilon;
	constexpr double xBiasMax = 1.0 - Constants::Epsilon;
	const double yBottom = yOffset;
	const double yTop = yOffset + 1.0;
	constexpr double zBiasMin = xBiasMin;
	constexpr double zBiasMax = xBiasMax;

	constexpr int positionComponentCount = GetRendererVertexPositionComponentCount(voxelType);
	const std::array<double, positionComponentCount> nearXPositions =
	{
		// X=0 Front
		xBiasMin, yTop, 0.0,
		xBiasMin, yBottom, 0.0,
		xBiasMin, yBottom, 1.0,
		xBiasMin, yTop, 1.0,

		// X=0 Back
		xBiasMin, yTop, 1.0,
		xBiasMin, yBottom, 1.0,
		xBiasMin, yBottom, 0.0,
		xBiasMin, yTop, 0.0
	};

	const std::array<double, positionComponentCount> farXPositions =
	{
		// X=1 Front
		xBiasMax, yTop, 1.0,
		xBiasMax, yBottom, 1.0,
		xBiasMax, yBottom, 0.0,
		xBiasMax, yTop, 0.0,

		// X=1 Back
		xBiasMax, yTop, 0.0,
		xBiasMax, yBottom, 0.0,
		xBiasMax, yBottom, 1.0,
		xBiasMax, yTop, 1.0
	};

	const std::array<double, positionComponentCount> nearZPositions =
	{
		// Z=0 Front
		1.0, yTop, zBiasMin,
		1.0, yBottom, zBiasMin,
		0.0, yBottom, zBiasMin,
		0.0, yTop, zBiasMin,

		// Z=0 Back
		0.0, yTop, zBiasMin,
		0.0, yBottom, zBiasMin,
		1.0, yBottom, zBiasMin,
		1.0, yTop, zBiasMin
	};

	const std::array<double, positionComponentCount> farZPositions =
	{
		// Z=1 Front
		0.0, yTop, zBiasMax,
		0.0, yBottom, zBiasMax,
		1.0, yBottom, zBiasMax,
		1.0, yTop, zBiasMax,

		// Z=1 Back
		1.0, yTop, zBiasMax,
		1.0, yBottom, zBiasMax,
		0.0, yBottom, zBiasMax,
		0.0, yTop, zBiasMax
	};

	constexpr int normalComponentCount = GetRendererVertexNormalComponentCount(voxelType);
	constexpr std::array<double, normalComponentCount> nearXNormals =
	{
		// X=0 Front
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,

		// X=0 Back
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0
	};

	constexpr std::array<double, normalComponentCount> farXNormals =
	{
		// X=1 Front
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,

		// X=1 Back
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0
	};

	constexpr std::array<double, normalComponentCount> nearZNormals =
	{
		// Z=0 Front
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,

		// Z=0 Back
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0
	};

	constexpr std::array<double, normalComponentCount> farZNormals =
	{
		// Z=1 Front
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,

		// Z=1 Back
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0
	};

	const std::array<double, positionComponentCount> *positions = nullptr;
	const std::array<double, normalComponentCount> *normals = nullptr;
	switch (facing)
	{
	case VoxelFacing2D::PositiveX:
		positions = &farXPositions;
		normals = &farXNormals;
		break;
	case VoxelFacing2D::NegativeX:
		positions = &nearXPositions;
		normals = &nearXNormals;
		break;
	case VoxelFacing2D::PositiveZ:
		positions = &farZPositions;
		normals = &farZNormals;
		break;
	case VoxelFacing2D::NegativeZ:
		positions = &nearZPositions;
		normals = &nearZNormals;
		break;
	default:
		DebugNotImplementedMsg(std::to_string(static_cast<int>(facing)));
	}

	constexpr int texCoordCount = GetRendererVertexTexCoordComponentCount(voxelType);
	constexpr std::array<double, texCoordCount> unflippedTexCoords =
	{
		// Front
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,

		// Back
		1.0, 0.0,
		1.0, 1.0,
		0.0, 1.0,
		0.0, 0.0
	};

	constexpr std::array<double, texCoordCount> flippedTexCoords =
	{
		// Front
		1.0, 0.0,
		1.0, 1.0,
		0.0, 1.0,
		0.0, 0.0,

		// Back
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0
	};

	const std::array<double, texCoordCount> &texCoords = flipped ? flippedTexCoords : unflippedTexCoords;

	std::copy(positions->begin(), positions->end(), outPositions.begin());
	std::copy(normals->begin(), normals->end(), outNormals.begin());
	std::copy(texCoords.begin(), texCoords.end(), outTexCoords.begin());
}

void ArenaMeshUtils::writeEdgeRendererIndexBuffers(BufferView<int32_t> outIndices)
{
	constexpr ArenaVoxelType voxelType = ArenaVoxelType::Edge;

	constexpr std::array<int32_t, GetIndexBufferIndexCount(voxelType, 0)> indices =
	{
		// Front
		0, 1, 2,
		2, 3, 0,

		// Back
		4, 5, 6,
		6, 7, 4
	};

	std::copy(indices.begin(), indices.end(), outIndices.begin());
}

void ArenaMeshUtils::writeChasmRendererGeometryBuffers(ArenaChasmType chasmType,
	BufferView<double> outPositions, BufferView<double> outNormals, BufferView<double> outTexCoords)
{
	constexpr ArenaVoxelType voxelType = ArenaVoxelType::Chasm;
	const double yBottom = 0.0;
	const double yTop = 1.0;

	const std::array<double, GetRendererVertexPositionComponentCount(voxelType)> positions =
	{
		// Y=0 (guaranteed to exist)
		0.0, yBottom, 1.0,
		1.0, yBottom, 1.0,
		1.0, yBottom, 0.0,
		0.0, yBottom, 0.0,

		// X=0
		0.0, yTop, 1.0,
		0.0, yBottom, 1.0,
		0.0, yBottom, 0.0,
		0.0, yTop, 0.0,
		// X=1
		1.0, yTop, 0.0,
		1.0, yBottom, 0.0,
		1.0, yBottom, 1.0,
		1.0, yTop, 1.0,
		// Z=0
		0.0, yTop, 0.0,
		0.0, yBottom, 0.0,
		1.0, yBottom, 0.0,
		1.0, yTop, 0.0,
		// Z=1
		1.0, yTop, 1.0,
		1.0, yBottom, 1.0,
		0.0, yBottom, 1.0,
		0.0, yTop, 1.0
	};

	constexpr std::array<double, GetRendererVertexNormalComponentCount(voxelType)> normals =
	{
		// Y=0 (guaranteed to exist)
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,

		// X=0
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		// X=1
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		// Z=0
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		// Z=1
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0
	};

	constexpr std::array<double, GetRendererVertexTexCoordComponentCount(voxelType)> texCoords =
	{
		// Y=0 (guaranteed to exist)
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,

		// X=0
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// X=1
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// Z=0
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// Z=1
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0
	};

	std::copy(positions.begin(), positions.end(), outPositions.begin());
	std::copy(normals.begin(), normals.end(), outNormals.begin());
	std::copy(texCoords.begin(), texCoords.end(), outTexCoords.begin());
}

void ArenaMeshUtils::writeChasmFloorRendererIndexBuffers(BufferView<int32_t> outIndices)
{
	constexpr ArenaVoxelType voxelType = ArenaVoxelType::Chasm;

	constexpr std::array<int32_t, GetIndexBufferIndexCount(voxelType, 0)> indices =
	{
		// Y=0
		0, 1, 2,
		2, 3, 0
	};

	std::copy(indices.begin(), indices.end(), outIndices.begin());
}

void ArenaMeshUtils::writeChasmWallRendererIndexBuffers(ArenaChasmWallIndexBuffer *outNorthIndices, ArenaChasmWallIndexBuffer *outEastIndices,
	ArenaChasmWallIndexBuffer *outSouthIndices, ArenaChasmWallIndexBuffer *outWestIndices)
{
	if (outNorthIndices != nullptr) // X=0
	{
		(*outNorthIndices)[0] = 4;
		(*outNorthIndices)[1] = 5;
		(*outNorthIndices)[2] = 6;
		(*outNorthIndices)[3] = 6;
		(*outNorthIndices)[4] = 7;
		(*outNorthIndices)[5] = 4;
	}

	if (outEastIndices != nullptr) // Z=0
	{
		(*outEastIndices)[0] = 12;
		(*outEastIndices)[1] = 13;
		(*outEastIndices)[2] = 14;
		(*outEastIndices)[3] = 14;
		(*outEastIndices)[4] = 15;
		(*outEastIndices)[5] = 12;
	}

	if (outSouthIndices != nullptr) // X=1
	{
		(*outSouthIndices)[0] = 8;
		(*outSouthIndices)[1] = 9;
		(*outSouthIndices)[2] = 10;
		(*outSouthIndices)[3] = 10;
		(*outSouthIndices)[4] = 11;
		(*outSouthIndices)[5] = 8;
	}

	if (outWestIndices != nullptr) // Z=1
	{
		(*outWestIndices)[0] = 16;
		(*outWestIndices)[1] = 17;
		(*outWestIndices)[2] = 18;
		(*outWestIndices)[3] = 18;
		(*outWestIndices)[4] = 19;
		(*outWestIndices)[5] = 16;
	}
}

void ArenaMeshUtils::writeDoorRendererGeometryBuffers(BufferView<double> outPositions, BufferView<double> outNormals,
	BufferView<double> outTexCoords)
{
	constexpr ArenaVoxelType voxelType = ArenaVoxelType::Door;

	// @todo: this will probably have double the positions for splitting doors.

	// One quad that gets translated/rotated per face.
	constexpr std::array<double, GetRendererVertexPositionComponentCount(voxelType)> positions =
	{
		// X=0
		0.0, 1.0, 0.0,
		0.0, 0.0, 0.0,
		0.0, 0.0, 1.0,
		0.0, 1.0, 1.0
	};

	constexpr std::array<double, GetRendererVertexNormalComponentCount(voxelType)> normals =
	{
		// X=0
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0
	};

	constexpr std::array<double, GetRendererVertexTexCoordComponentCount(voxelType)> texCoords =
	{
		// X=0
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0
	};

	std::copy(positions.begin(), positions.end(), outPositions.begin());
	std::copy(normals.begin(), normals.end(), outNormals.begin());
	std::copy(texCoords.begin(), texCoords.end(), outTexCoords.begin());
}

void ArenaMeshUtils::writeDoorRendererIndexBuffers(BufferView<int32_t> outIndices)
{
	constexpr ArenaVoxelType voxelType = ArenaVoxelType::Door;

	constexpr std::array<int32_t, GetIndexBufferIndexCount(voxelType, 0)> indices =
	{
		// X=0
		0, 1, 2,
		2, 3, 0
	};

	std::copy(indices.begin(), indices.end(), outIndices.begin());
}
