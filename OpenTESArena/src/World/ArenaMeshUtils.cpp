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

	void WriteIndexBuffersSidesBottomTop(ArenaVoxelType voxelType, Span<int32_t> outSideIndices, Span<int32_t> outBottomIndices, Span<int32_t> outTopIndices)
	{
		const MeshLibrary &meshLibrary = MeshLibrary::getInstance();
		const MeshLibraryEntry *faceMeshEntries[] =
		{
			meshLibrary.getEntryWithTypeAndFacing(voxelType, VoxelFacing3D::PositiveX),
			meshLibrary.getEntryWithTypeAndFacing(voxelType, VoxelFacing3D::NegativeX),
			meshLibrary.getEntryWithTypeAndFacing(voxelType, VoxelFacing3D::PositiveY),
			meshLibrary.getEntryWithTypeAndFacing(voxelType, VoxelFacing3D::NegativeY),
			meshLibrary.getEntryWithTypeAndFacing(voxelType, VoxelFacing3D::PositiveZ),
			meshLibrary.getEntryWithTypeAndFacing(voxelType, VoxelFacing3D::NegativeZ)
		};

		auto writeEntryIndices = [](const MeshLibraryEntry *entry, Span<int32_t> outIndices, int &writeIndex)
		{
			if (entry == nullptr)
			{
				DebugLogWarning("Missing mesh library entry for sides/bottom/top indices.");
				return;
			}

			for (const int vertexIndex : entry->vertexIndices)
			{
				outIndices[writeIndex] = vertexIndex;
				writeIndex++;
			}
		};

		int sideIndicesWriteIndex = 0;
		writeEntryIndices(faceMeshEntries[0], outSideIndices, sideIndicesWriteIndex);
		writeEntryIndices(faceMeshEntries[1], outSideIndices, sideIndicesWriteIndex);
		writeEntryIndices(faceMeshEntries[4], outSideIndices, sideIndicesWriteIndex);
		writeEntryIndices(faceMeshEntries[5], outSideIndices, sideIndicesWriteIndex);

		int bottomIndicesWriteIndex = 0;
		writeEntryIndices(faceMeshEntries[3], outBottomIndices, bottomIndicesWriteIndex);

		int topIndicesWriteIndex = 0;
		writeEntryIndices(faceMeshEntries[2], outTopIndices, topIndicesWriteIndex);
	}

	void WriteIndexBuffersSides(ArenaVoxelType voxelType, Span<int32_t> outSideIndices)
	{
		const MeshLibrary &meshLibrary = MeshLibrary::getInstance();
		const MeshLibraryEntry *faceMeshEntries[] =
		{
			meshLibrary.getEntryWithTypeAndFacing(voxelType, VoxelFacing3D::PositiveX),
			meshLibrary.getEntryWithTypeAndFacing(voxelType, VoxelFacing3D::NegativeX),
			meshLibrary.getEntryWithTypeAndFacing(voxelType, VoxelFacing3D::PositiveZ),
			meshLibrary.getEntryWithTypeAndFacing(voxelType, VoxelFacing3D::NegativeZ)
		};

		auto writeEntryIndices = [](const MeshLibraryEntry *entry, Span<int32_t> outIndices, int &writeIndex)
		{
			if (entry == nullptr)
			{
				DebugLogWarning("Missing mesh library entry for side indices.");
				return;
			}

			for (const int vertexIndex : entry->vertexIndices)
			{
				outIndices[writeIndex] = vertexIndex;
				writeIndex++;
			}
		};

		int sideIndicesWriteIndex = 0;
		writeEntryIndices(faceMeshEntries[0], outSideIndices, sideIndicesWriteIndex);
		writeEntryIndices(faceMeshEntries[1], outSideIndices, sideIndicesWriteIndex);
		writeEntryIndices(faceMeshEntries[2], outSideIndices, sideIndicesWriteIndex);
		writeEntryIndices(faceMeshEntries[3], outSideIndices, sideIndicesWriteIndex);
	}

	void WriteIndexBuffer(const MeshLibraryEntry &entry, Span<int32_t> outIndices)
	{
		int writeIndex = 0;
		for (const int vertexIndex : entry.vertexIndices)
		{
			outIndices[writeIndex] = vertexIndex;
			writeIndex++;
		}
	}

	void WriteIndexBuffer(ArenaVoxelType voxelType, Span<int32_t> outIndices)
	{
		const MeshLibrary &meshLibrary = MeshLibrary::getInstance();
		Span<const MeshLibraryEntry> entries = meshLibrary.getEntriesOfType(voxelType);
		if (entries.getCount() != 1)
		{
			DebugLogErrorFormat("Expected only one mesh entry for voxel type %d.", static_cast<int>(voxelType));
			return;
		}

		WriteIndexBuffer(entries[0], outIndices);
	}

	void WriteFacingBuffersSidesBottomTop(ArenaVoxelType voxelType, Span<VoxelFacing3D> outSideFacings,
		Span<VoxelFacing3D> outBottomFacings, Span<VoxelFacing3D> outTopFacings)
	{
		const MeshLibrary &meshLibrary = MeshLibrary::getInstance();
		const MeshLibraryEntry *faceMeshEntries[] =
		{
			meshLibrary.getEntryWithTypeAndFacing(voxelType, VoxelFacing3D::PositiveX),
			meshLibrary.getEntryWithTypeAndFacing(voxelType, VoxelFacing3D::NegativeX),
			meshLibrary.getEntryWithTypeAndFacing(voxelType, VoxelFacing3D::PositiveY),
			meshLibrary.getEntryWithTypeAndFacing(voxelType, VoxelFacing3D::NegativeY),
			meshLibrary.getEntryWithTypeAndFacing(voxelType, VoxelFacing3D::PositiveZ),
			meshLibrary.getEntryWithTypeAndFacing(voxelType, VoxelFacing3D::NegativeZ)
		};

		auto writeEntryFacing = [](const MeshLibraryEntry *entry, Span<VoxelFacing3D> outFacings, int &writeIndex)
		{
			if (entry == nullptr)
			{
				DebugLogWarning("Missing mesh library entry for sides/bottom/top facings.");
				return;
			}

			if (!entry->facing.has_value())
			{
				return;
			}

			outFacings[writeIndex] = *entry->facing;
			writeIndex++;
		};

		int sideFacingsWriteIndex = 0;
		writeEntryFacing(faceMeshEntries[0], outSideFacings, sideFacingsWriteIndex);
		writeEntryFacing(faceMeshEntries[1], outSideFacings, sideFacingsWriteIndex);
		writeEntryFacing(faceMeshEntries[4], outSideFacings, sideFacingsWriteIndex);
		writeEntryFacing(faceMeshEntries[5], outSideFacings, sideFacingsWriteIndex);

		int bottomFacingsWriteIndex = 0;
		writeEntryFacing(faceMeshEntries[3], outBottomFacings, bottomFacingsWriteIndex);

		int topFacingsWriteIndex = 0;
		writeEntryFacing(faceMeshEntries[2], outTopFacings, topFacingsWriteIndex);
	}

	void WriteFacingBuffersSides(ArenaVoxelType voxelType, Span<VoxelFacing3D> outSideFacings)
	{
		const MeshLibrary &meshLibrary = MeshLibrary::getInstance();
		const MeshLibraryEntry *faceMeshEntries[] =
		{
			meshLibrary.getEntryWithTypeAndFacing(voxelType, VoxelFacing3D::PositiveX),
			meshLibrary.getEntryWithTypeAndFacing(voxelType, VoxelFacing3D::NegativeX),
			meshLibrary.getEntryWithTypeAndFacing(voxelType, VoxelFacing3D::PositiveZ),
			meshLibrary.getEntryWithTypeAndFacing(voxelType, VoxelFacing3D::NegativeZ)
		};

		auto writeEntryFacing = [](const MeshLibraryEntry *entry, Span<VoxelFacing3D> outFacings, int &writeIndex)
		{
			if (entry == nullptr)
			{
				DebugLogWarning("Missing mesh library entry for side facings.");
				return;
			}

			if (!entry->facing.has_value())
			{
				return;
			}

			outFacings[writeIndex] = *entry->facing;
			writeIndex++;
		};

		int sideFacingsWriteIndex = 0;
		writeEntryFacing(faceMeshEntries[0], outSideFacings, sideFacingsWriteIndex);
		writeEntryFacing(faceMeshEntries[1], outSideFacings, sideFacingsWriteIndex);
		writeEntryFacing(faceMeshEntries[2], outSideFacings, sideFacingsWriteIndex);
		writeEntryFacing(faceMeshEntries[3], outSideFacings, sideFacingsWriteIndex);
	}

	void WriteFacing(const MeshLibraryEntry &entry, Span<VoxelFacing3D> outFacings)
	{
		if (!entry.facing.has_value())
		{
			return;
		}

		outFacings[0] = *entry.facing;
	}

	void WriteFacing(ArenaVoxelType voxelType, Span<VoxelFacing3D> outFacings)
	{
		const MeshLibrary &meshLibrary = MeshLibrary::getInstance();
		Span<const MeshLibraryEntry> entries = meshLibrary.getEntriesOfType(voxelType);
		if (entries.getCount() != 1)
		{
			DebugLogErrorFormat("Expected only one mesh entry for voxel type %d.", static_cast<int>(voxelType));
			return;
		}

		WriteFacing(entries[0], outFacings);
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
	WriteIndexBuffersSidesBottomTop(ArenaVoxelType::Wall, outSideIndices, outBottomIndices, outTopIndices);
}

void ArenaMeshUtils::writeWallFacingBuffers(Span<VoxelFacing3D> outSideFacings, Span<VoxelFacing3D> outBottomFacings, Span<VoxelFacing3D> outTopFacings)
{
	WriteFacingBuffersSidesBottomTop(ArenaVoxelType::Wall, outSideFacings, outBottomFacings, outTopFacings);
}

void ArenaMeshUtils::writeFloorRendererGeometryBuffers(Span<double> outPositions, Span<double> outNormals,
	Span<double> outTexCoords)
{
	WriteVertexBuffers(ArenaVoxelType::Floor, outPositions, outNormals, outTexCoords);
}

void ArenaMeshUtils::writeFloorRendererIndexBuffers(Span<int32_t> outIndices)
{
	WriteIndexBuffer(ArenaVoxelType::Floor, outIndices);
}

void ArenaMeshUtils::writeFloorFacingBuffers(Span<VoxelFacing3D> outFacings)
{
	WriteFacing(ArenaVoxelType::Floor, outFacings);
}

void ArenaMeshUtils::writeCeilingRendererGeometryBuffers(Span<double> outPositions, Span<double> outNormals, Span<double> outTexCoords)
{
	WriteVertexBuffers(ArenaVoxelType::Ceiling, outPositions, outNormals, outTexCoords);
}

void ArenaMeshUtils::writeCeilingRendererIndexBuffers(Span<int32_t> outIndices)
{
	WriteIndexBuffer(ArenaVoxelType::Ceiling, outIndices);
}

void ArenaMeshUtils::writeCeilingFacingBuffers(Span<VoxelFacing3D> outFacings)
{
	WriteFacing(ArenaVoxelType::Ceiling, outFacings);
}

void ArenaMeshUtils::writeRaisedRendererGeometryBuffers(double yOffset, double ySize, double vBottom, double vTop,
	Span<double> outPositions, Span<double> outNormals, Span<double> outTexCoords)
{
	WriteVertexBuffersRaised(yOffset, ySize, vBottom, vTop, outPositions, outNormals, outTexCoords);
}

void ArenaMeshUtils::writeRaisedRendererIndexBuffers(Span<int32_t> outSideIndices, Span<int32_t> outBottomIndices, Span<int32_t> outTopIndices)
{
	WriteIndexBuffersSidesBottomTop(ArenaVoxelType::Raised, outSideIndices, outBottomIndices, outTopIndices);
}

void ArenaMeshUtils::writeDiagonalRendererGeometryBuffers(bool type1, Span<double> outPositions, Span<double> outNormals, Span<double> outTexCoords)
{
	const MeshLibrary &meshLibrary = MeshLibrary::getInstance();
	Span<const MeshLibraryEntry> meshEntries = meshLibrary.getEntriesOfType(ArenaVoxelType::Diagonal);
	if (meshEntries.getCount() != 2)
	{
		DebugLogErrorFormat("Expected two diagonal mesh entries to pick from (have %d).", meshEntries.getCount());
		return;
	}

	const int sliceIndex = type1 ? 0 : 1;
	Span<const MeshLibraryEntry> meshEntriesSlice = meshEntries.slice(sliceIndex, 1);
	WriteVertexBuffers(meshEntriesSlice, outPositions, outNormals, outTexCoords);
}

void ArenaMeshUtils::writeDiagonalRendererIndexBuffers(Span<int32_t> outIndices)
{
	const MeshLibrary &meshLibrary = MeshLibrary::getInstance();
	Span<const MeshLibraryEntry> meshEntries = meshLibrary.getEntriesOfType(ArenaVoxelType::Diagonal);
	if (meshEntries.getCount() == 0)
	{
		DebugLogError("Missing diagonal mesh entries.");
		return;
	}

	WriteIndexBuffer(meshEntries[0], outIndices);
}

void ArenaMeshUtils::writeTransparentWallRendererGeometryBuffers(Span<double> outPositions, Span<double> outNormals, Span<double> outTexCoords)
{
	WriteVertexBuffers(ArenaVoxelType::TransparentWall, outPositions, outNormals, outTexCoords);
}

void ArenaMeshUtils::writeTransparentWallRendererIndexBuffers(Span<int32_t> outIndices)
{
	WriteIndexBuffersSides(ArenaVoxelType::TransparentWall, outIndices);
}

void ArenaMeshUtils::writeEdgeRendererGeometryBuffers(VoxelFacing2D facing, double yOffset, bool flipped,
	Span<double> outPositions, Span<double> outNormals, Span<double> outTexCoords)
{
	WriteVertexBuffersEdge(facing, yOffset, flipped, outPositions, outNormals, outTexCoords);
}

void ArenaMeshUtils::writeEdgeRendererIndexBuffers(Span<int32_t> outIndices)
{
	const MeshLibrary &meshLibrary = MeshLibrary::getInstance();
	Span<const MeshLibraryEntry> meshEntries = meshLibrary.getEntriesOfType(ArenaVoxelType::Edge);
	if (meshEntries.getCount() == 0)
	{
		DebugLogError("Missing edge mesh entries.");
		return;
	}

	WriteIndexBuffer(meshEntries[0], outIndices);
}

void ArenaMeshUtils::writeChasmRendererGeometryBuffers(Span<double> outPositions, Span<double> outNormals, Span<double> outTexCoords)
{
	WriteVertexBuffers(ArenaVoxelType::Chasm, outPositions, outNormals, outTexCoords);
}

void ArenaMeshUtils::writeChasmFloorRendererIndexBuffers(Span<int32_t> outIndices)
{
	const MeshLibrary &meshLibrary = MeshLibrary::getInstance();
	const MeshLibraryEntry *bottomEntry = meshLibrary.getEntryWithTypeAndFacing(ArenaVoxelType::Chasm, VoxelFacing3D::NegativeY);
	if (bottomEntry == nullptr)
	{
		DebugLogError("Missing chasm floor mesh entry.");
		return;
	}

	WriteIndexBuffer(*bottomEntry, outIndices);
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
	WriteIndexBuffer(ArenaVoxelType::Door, outIndices);
}
