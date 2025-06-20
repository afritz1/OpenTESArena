#include "ArenaMeshUtils.h"
#include "MeshLibrary.h"
#include "../Math/Constants.h"
#include "../Voxels/VoxelFacing.h"

namespace
{
	void WriteVertexBuffers(Span<const MeshLibraryEntry> meshEntries, Span<double> outPositions, Span<double> outNormals, Span<double> outTexCoords)
	{
		int destinationVertexIndex = 0;
		for (int i = 0; i < meshEntries.getCount(); i++)
		{
			const MeshLibraryEntry &meshEntry = meshEntries[i];
			for (const ObjVertex &sourceVertex : meshEntry.vertices)
			{
				const int outPositionsIndex = destinationVertexIndex * MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
				const int outNormalsIndex = destinationVertexIndex * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
				const int outTexCoordsIndex = destinationVertexIndex * MeshUtils::TEX_COORD_COMPONENTS_PER_VERTEX;
				outPositions[outPositionsIndex] = sourceVertex.positionX;
				outPositions[outPositionsIndex + 1] = sourceVertex.positionY;
				outPositions[outPositionsIndex + 2] = sourceVertex.positionZ;
				outNormals[outNormalsIndex] = sourceVertex.normalX;
				outNormals[outNormalsIndex + 1] = sourceVertex.normalY;
				outNormals[outNormalsIndex + 2] = sourceVertex.normalZ;
				outTexCoords[outTexCoordsIndex] = sourceVertex.texCoordU;
				outTexCoords[outTexCoordsIndex + 1] = sourceVertex.texCoordV;
				destinationVertexIndex++;
			}
		}
	}

	void WriteVertexBuffersRaised(double yOffset, double ySize, double vBottom, double vTop,
		Span<double> outPositions, Span<double> outNormals, Span<double> outTexCoords)
	{
		const MeshLibrary &meshLibrary = MeshLibrary::getInstance();
		Span<const MeshLibraryEntry> meshEntries = meshLibrary.getEntriesOfType(ArenaVoxelType::Raised);

		double yMax = std::numeric_limits<double>::lowest();
		double vMax = std::numeric_limits<double>::lowest();
		for (int i = 0; i < meshEntries.getCount(); i++)
		{
			const MeshLibraryEntry &meshEntry = meshEntries[i];
			for (const ObjVertex &sourceVertex : meshEntry.vertices)
			{
				yMax = std::max(yMax, sourceVertex.positionY);
				vMax = std::max(vMax, sourceVertex.texCoordV);
			}
		}

		int destinationVertexIndex = 0;
		for (int i = 0; i < meshEntries.getCount(); i++)
		{
			const MeshLibraryEntry &meshEntry = meshEntries[i];
			for (const ObjVertex &sourceVertex : meshEntry.vertices)
			{
				const int outPositionsIndex = destinationVertexIndex * MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
				const int outNormalsIndex = destinationVertexIndex * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
				const int outTexCoordsIndex = destinationVertexIndex * MeshUtils::TEX_COORD_COMPONENTS_PER_VERTEX;
				outPositions[outPositionsIndex] = sourceVertex.positionX;

				double outPositionY = yOffset;
				if (sourceVertex.positionY == yMax)
				{
					outPositionY += ySize;
				}

				outPositions[outPositionsIndex + 1] = outPositionY;
				outPositions[outPositionsIndex + 2] = sourceVertex.positionZ;
				outNormals[outNormalsIndex] = sourceVertex.normalX;
				outNormals[outNormalsIndex + 1] = sourceVertex.normalY;
				outNormals[outNormalsIndex + 2] = sourceVertex.normalZ;
				outTexCoords[outTexCoordsIndex] = sourceVertex.texCoordU;

				double outTexCoordV = vTop;
				if (sourceVertex.texCoordV == vMax)
				{
					outTexCoordV = vBottom;
				}

				outTexCoords[outTexCoordsIndex + 1] = outTexCoordV;
				destinationVertexIndex++;
			}
		}
	}

	void WriteVertexBuffersEdge(VoxelFacing2D facing, double yOffset, bool flipped, Span<double> outPositions,
		Span<double> outNormals, Span<double> outTexCoords)
	{
		const VoxelFacing3D facing3D = VoxelUtils::convertFaceTo3D(facing);
		const MeshLibrary &meshLibrary = MeshLibrary::getInstance();
		const MeshLibraryEntry *meshEntry = meshLibrary.getEntryWithTypeAndFacing(ArenaVoxelType::Edge, facing3D);
		if (meshEntry == nullptr)
		{
			DebugLogErrorFormat("Couldn't get mesh entry for edge voxel with facing %d.", static_cast<int>(facing3D));
			return;
		}

		double yMax = std::numeric_limits<double>::lowest();
		for (const ObjVertex &sourceVertex : meshEntry->vertices)
		{
			yMax = std::max(yMax, sourceVertex.positionY);
		}

		int destinationVertexIndex = 0;
		for (const ObjVertex &sourceVertex : meshEntry->vertices)
		{
			const int outPositionsIndex = destinationVertexIndex * MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
			const int outNormalsIndex = destinationVertexIndex * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
			const int outTexCoordsIndex = destinationVertexIndex * MeshUtils::TEX_COORD_COMPONENTS_PER_VERTEX;
			outPositions[outPositionsIndex] = sourceVertex.positionX;

			double outPositionY = yOffset;
			if (sourceVertex.positionY == yMax)
			{
				outPositionY += 1.0;
			}

			outPositions[outPositionsIndex + 1] = outPositionY;

			outPositions[outPositionsIndex + 2] = sourceVertex.positionZ;
			outNormals[outNormalsIndex] = sourceVertex.normalX;
			outNormals[outNormalsIndex + 1] = sourceVertex.normalY;
			outNormals[outNormalsIndex + 2] = sourceVertex.normalZ;

			double outTexCoordU = sourceVertex.texCoordU;
			if (flipped)
			{
				outTexCoordU = std::clamp(1.0 - outTexCoordU, 0.0, 1.0);
			}

			outTexCoords[outTexCoordsIndex] = outTexCoordU;
			outTexCoords[outTexCoordsIndex + 1] = sourceVertex.texCoordV;
			destinationVertexIndex++;
		}
	}

	void WriteVertexBuffers(ArenaVoxelType voxelType, Span<double> outPositions, Span<double> outNormals, Span<double> outTexCoords)
	{
		const MeshLibrary &meshLibrary = MeshLibrary::getInstance();
		Span<const MeshLibraryEntry> meshEntries = meshLibrary.getEntriesOfType(voxelType);
		WriteVertexBuffers(meshEntries, outPositions, outNormals, outTexCoords);
	}
}

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

void ArenaMeshUtils::writeWallRendererGeometryBuffers(Span<double> outPositions, Span<double> outNormals, Span<double> outTexCoords)
{
	WriteVertexBuffers(ArenaVoxelType::Wall, outPositions, outNormals, outTexCoords);
}

void ArenaMeshUtils::writeWallRendererIndexBuffers(Span<int32_t> outSideIndices, Span<int32_t> outBottomIndices, Span<int32_t> outTopIndices)
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

void ArenaMeshUtils::writeWallFacingBuffers(Span<VoxelFacing3D> outSideFacings, Span<VoxelFacing3D> outBottomFacings, Span<VoxelFacing3D> outTopFacings)
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

void ArenaMeshUtils::writeFloorRendererGeometryBuffers(Span<double> outPositions, Span<double> outNormals,
	Span<double> outTexCoords)
{
	WriteVertexBuffers(ArenaVoxelType::Floor, outPositions, outNormals, outTexCoords);
}

void ArenaMeshUtils::writeFloorRendererIndexBuffers(Span<int32_t> outIndices)
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

void ArenaMeshUtils::writeFloorFacingBuffers(Span<VoxelFacing3D> outFacings)
{
	constexpr ArenaVoxelType voxelType = ArenaVoxelType::Floor;
	constexpr std::array<VoxelFacing3D, GetFacingBufferFaceCount(voxelType, 0)> facings =
	{
		VoxelFacing3D::PositiveY
	};

	std::copy(facings.begin(), facings.end(), outFacings.begin());
}

void ArenaMeshUtils::writeCeilingRendererGeometryBuffers(Span<double> outPositions, Span<double> outNormals, Span<double> outTexCoords)
{
	WriteVertexBuffers(ArenaVoxelType::Ceiling, outPositions, outNormals, outTexCoords);
}

void ArenaMeshUtils::writeCeilingRendererIndexBuffers(Span<int32_t> outIndices)
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

void ArenaMeshUtils::writeCeilingFacingBuffers(Span<VoxelFacing3D> outFacings)
{
	constexpr ArenaVoxelType voxelType = ArenaVoxelType::Ceiling;
	constexpr std::array<VoxelFacing3D, GetFacingBufferFaceCount(voxelType, 0)> facings =
	{
		VoxelFacing3D::NegativeY
	};

	std::copy(facings.begin(), facings.end(), outFacings.begin());
}

void ArenaMeshUtils::writeRaisedRendererGeometryBuffers(double yOffset, double ySize, double vBottom, double vTop,
	Span<double> outPositions, Span<double> outNormals, Span<double> outTexCoords)
{
	WriteVertexBuffersRaised(yOffset, ySize, vBottom, vTop, outPositions, outNormals, outTexCoords);
}

void ArenaMeshUtils::writeRaisedRendererIndexBuffers(Span<int32_t> outSideIndices, Span<int32_t> outBottomIndices, Span<int32_t> outTopIndices)
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

void ArenaMeshUtils::writeDiagonalRendererGeometryBuffers(bool type1, Span<double> outPositions, Span<double> outNormals, Span<double> outTexCoords)
{
	const MeshLibrary &meshLibrary = MeshLibrary::getInstance();
	Span<const MeshLibraryEntry> meshEntries = meshLibrary.getEntriesOfType(ArenaVoxelType::Diagonal);
	if (meshEntries.getCount() != 2)
	{
		DebugLogErrorFormat("Expected two diagonal meshes to pick from (have %d).", meshEntries.getCount());
		return;
	}

	const int sliceIndex = type1 ? 0 : 1;
	Span<const MeshLibraryEntry> meshEntriesSlice = meshEntries.slice(sliceIndex, 1);
	WriteVertexBuffers(meshEntriesSlice, outPositions, outNormals, outTexCoords);
}

void ArenaMeshUtils::writeDiagonalRendererIndexBuffers(Span<int32_t> outIndices)
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

void ArenaMeshUtils::writeTransparentWallRendererGeometryBuffers(Span<double> outPositions, Span<double> outNormals, Span<double> outTexCoords)
{
	WriteVertexBuffers(ArenaVoxelType::TransparentWall, outPositions, outNormals, outTexCoords);
}

void ArenaMeshUtils::writeTransparentWallRendererIndexBuffers(Span<int32_t> outIndices)
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
	Span<double> outPositions, Span<double> outNormals, Span<double> outTexCoords)
{
	WriteVertexBuffersEdge(facing, yOffset, flipped, outPositions, outNormals, outTexCoords);
}

void ArenaMeshUtils::writeEdgeRendererIndexBuffers(Span<int32_t> outIndices)
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

void ArenaMeshUtils::writeChasmRendererGeometryBuffers(Span<double> outPositions, Span<double> outNormals, Span<double> outTexCoords)
{
	WriteVertexBuffers(ArenaVoxelType::Chasm, outPositions, outNormals, outTexCoords);
}

void ArenaMeshUtils::writeChasmFloorRendererIndexBuffers(Span<int32_t> outIndices)
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

void ArenaMeshUtils::writeDoorRendererGeometryBuffers(Span<double> outPositions, Span<double> outNormals,
	Span<double> outTexCoords)
{
	WriteVertexBuffers(ArenaVoxelType::Door, outPositions, outNormals, outTexCoords);
}

void ArenaMeshUtils::writeDoorRendererIndexBuffers(Span<int32_t> outIndices)
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
