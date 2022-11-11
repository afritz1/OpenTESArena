#include <array>
#include <numeric>
#include <optional>

#include "SceneGraph.h"
#include "../ArenaRenderUtils.h"
#include "../RenderCamera.h"
#include "../Renderer.h"
#include "../RendererSystem3D.h"
#include "../../Assets/MIFUtils.h"
#include "../../Entities/EntityManager.h"
#include "../../Entities/EntityVisibilityState.h"
#include "../../Math/Constants.h"
#include "../../Math/Matrix4.h"
#include "../../Media/TextureManager.h"
#include "../../World/ArenaMeshUtils.h"
#include "../../World/ChunkManager.h"
#include "../../World/LevelInstance.h"
#include "../../World/MapDefinition.h"
#include "../../World/MapType.h"
#include "../../World/VoxelFacing2D.h"

#include "components/debug/Debug.h"

namespace sgTexture
{
	// Indices for looking up VoxelDefinition textures based on which index buffer is being used.
	int GetVoxelOpaqueTextureAssetIndex(ArenaTypes::VoxelType voxelType, int indexBufferIndex)
	{
		switch (voxelType)
		{
		case ArenaTypes::VoxelType::Wall:
		case ArenaTypes::VoxelType::Floor:
		case ArenaTypes::VoxelType::Ceiling:
		case ArenaTypes::VoxelType::Diagonal:
			return indexBufferIndex;
		case ArenaTypes::VoxelType::Raised:
			if (indexBufferIndex == 0)
			{
				return 1;
			}
			else if (indexBufferIndex == 1)
			{
				return 2;
			}
			else
			{
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)) + " " + std::to_string(indexBufferIndex));
			}
		case ArenaTypes::VoxelType::Chasm:
			if (indexBufferIndex == 0)
			{
				return 0;
			}
			else
			{
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)) + " " + std::to_string(indexBufferIndex));
			}
		case ArenaTypes::VoxelType::TransparentWall:
		case ArenaTypes::VoxelType::Edge:
		case ArenaTypes::VoxelType::Door:
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)) + " " + std::to_string(indexBufferIndex));
		default:
			DebugNotImplementedMsg(std::to_string(static_cast<int>(voxelType)));
		}
	}

	int GetVoxelAlphaTestedTextureAssetIndex(ArenaTypes::VoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaTypes::VoxelType::Wall:
		case ArenaTypes::VoxelType::Floor:
		case ArenaTypes::VoxelType::Ceiling:
		case ArenaTypes::VoxelType::Diagonal:
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)));
		case ArenaTypes::VoxelType::Raised:
		case ArenaTypes::VoxelType::TransparentWall:
		case ArenaTypes::VoxelType::Edge:
		case ArenaTypes::VoxelType::Door:
			return 0;
		case ArenaTypes::VoxelType::Chasm:
			return 1;
		default:
			DebugNotImplementedMsg(std::to_string(static_cast<int>(voxelType)));
		}
	}

	// Loads the given voxel definition's textures into the voxel textures list if they haven't been loaded yet.
	void LoadVoxelDefTextures(const VoxelTextureDefinition &voxelTextureDef, std::vector<SceneGraph::LoadedVoxelTexture> &voxelTextures,
		TextureManager &textureManager, Renderer &renderer)
	{
		for (int i = 0; i < voxelTextureDef.textureCount; i++)
		{
			const TextureAsset &textureAsset = voxelTextureDef.getTextureAsset(i);
			const auto cacheIter = std::find_if(voxelTextures.begin(), voxelTextures.end(),
				[&textureAsset](const SceneGraph::LoadedVoxelTexture &loadedTexture)
			{
				return loadedTexture.textureAsset == textureAsset;
			});

			if (cacheIter == voxelTextures.end())
			{
				const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAsset);
				if (!textureBuilderID.has_value())
				{
					DebugLogWarning("Couldn't load voxel texture \"" + textureAsset.filename + "\".");
					continue;
				}

				const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
				ObjectTextureID voxelTextureID;
				if (!renderer.tryCreateObjectTexture(textureBuilder, &voxelTextureID))
				{
					DebugLogWarning("Couldn't create voxel texture \"" + textureAsset.filename + "\".");
					continue;
				}

				ScopedObjectTextureRef voxelTextureRef(voxelTextureID, renderer);
				SceneGraph::LoadedVoxelTexture newTexture;
				newTexture.init(textureAsset, std::move(voxelTextureRef));
				voxelTextures.emplace_back(std::move(newTexture));
			}
		}
	}

	bool LoadedChasmFloorComparer(const SceneGraph::LoadedChasmFloorTextureList &textureList, const ChasmDefinition &chasmDef)
	{
		if (textureList.animType != chasmDef.animType)
		{
			return false;
		}

		if (textureList.animType == ChasmDefinition::AnimationType::SolidColor)
		{
			return textureList.paletteIndex == chasmDef.solidColor.paletteIndex;
		}
		else if (textureList.animType == ChasmDefinition::AnimationType::Animated)
		{
			const int textureAssetCount = static_cast<int>(textureList.textureAssets.size());
			const ChasmDefinition::Animated &chasmDefAnimated = chasmDef.animated;

			if (textureAssetCount != chasmDefAnimated.textureAssets.getCount())
			{
				return false;
			}

			for (int i = 0; i < textureAssetCount; i++)
			{
				if (textureList.textureAssets[i] != chasmDefAnimated.textureAssets.get(i))
				{
					return false;
				}
			}

			return true;
		}
		else
		{
			DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(textureList.animType)));
		}
	}

	void LoadChasmDefTextures(VoxelChunk::ChasmDefID chasmDefID, const VoxelChunk &chunk, std::vector<SceneGraph::LoadedChasmFloorTextureList> &chasmTextureLists,
		std::vector<SceneGraph::LoadedChasmTextureKey> &chasmTextureKeys, TextureManager &textureManager, Renderer &renderer)
	{
		const ChunkInt2 chunkPos = chunk.getPosition();
		const ChasmDefinition &chasmDef = chunk.getChasmDef(chasmDefID);

		// Check if this chasm already has a mapping.
		const auto keyIter = std::find_if(chasmTextureKeys.begin(), chasmTextureKeys.end(),
			[chasmDefID, &chunkPos](const SceneGraph::LoadedChasmTextureKey &loadedKey)
		{
			return (loadedKey.chasmDefID == chasmDefID) && (loadedKey.chunkPos == chunkPos);
		});

		if (keyIter != chasmTextureKeys.end())
		{
			DebugLog("Chasm texture key already loaded for chasm def ID \"" + std::to_string(chasmDefID) + "\" in chunk (" + chunkPos.toString() + ").");
			return;
		}

		// Check if any loaded chasms reference the same assets.
		const auto chasmIter = std::find_if(chasmTextureLists.begin(), chasmTextureLists.end(),
			[&chasmDef](const SceneGraph::LoadedChasmFloorTextureList &textureList)
		{
			return LoadedChasmFloorComparer(textureList, chasmDef);
		});

		SceneGraph::LoadedChasmTextureKey key;
		if (chasmIter != chasmTextureLists.end())
		{
			// Add a key referencing that same mapping for this chasmID + chunkPos.
			DebugLog("Chasm texture(s) already loaded for chasm def ID \"" + std::to_string(chasmDefID) + "\" in chunk (" + chunkPos.toString() + ").");

			const int loadedChasmFloorListIndex = static_cast<int>(std::distance(chasmTextureLists.begin(), chasmIter));
			const int loadedChasmWallIndex = loadedChasmFloorListIndex; // @todo
			DebugLogError("Not implemented: loadedChasmWallIndex");
			key.init(chunkPos, chasmDefID, loadedChasmFloorListIndex, loadedChasmWallIndex);
		}
		else
		{
			// Load the required textures and add a key for them.
			SceneGraph::LoadedChasmFloorTextureList newTextureList;
			if (chasmDef.animType == ChasmDefinition::AnimationType::SolidColor)
			{
				// Dry chasms are a single color, no texture asset.
				ObjectTextureID dryChasmTextureID;
				if (!renderer.tryCreateObjectTexture(1, 1, false, &dryChasmTextureID))
				{
					DebugLogWarning("Couldn't create dry chasm texture.");
					return;
				}

				ScopedObjectTextureRef dryChasmTextureRef(dryChasmTextureID, renderer);
				LockedTexture lockedTexture = renderer.lockObjectTexture(dryChasmTextureID);
				if (!lockedTexture.isValid())
				{
					DebugLogWarning("Couldn't lock dry chasm texture for writing.");
					return;
				}

				const uint8_t paletteIndex = chasmDef.solidColor.paletteIndex;

				DebugAssert(!lockedTexture.isTrueColor);
				uint8_t *texels = static_cast<uint8_t*>(lockedTexture.texels);
				*texels = paletteIndex;
				renderer.unlockObjectTexture(dryChasmTextureID);

				newTextureList.initColor(paletteIndex, std::move(dryChasmTextureRef));
				chasmTextureLists.emplace_back(std::move(newTextureList));
			}
			else if (chasmDef.animType == ChasmDefinition::AnimationType::Animated)
			{
				std::vector<TextureAsset> newTextureAssets;
				std::vector<ScopedObjectTextureRef> newObjectTextureRefs;

				const Buffer<TextureAsset> &textureAssets = chasmDef.animated.textureAssets;
				for (int i = 0; i < textureAssets.getCount(); i++)
				{
					const TextureAsset &textureAsset = textureAssets.get(i);
					const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAsset);
					if (!textureBuilderID.has_value())
					{
						DebugLogWarning("Couldn't load chasm texture \"" + textureAsset.filename + "\".");
						continue;
					}

					const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
					ObjectTextureID chasmTextureID;
					if (!renderer.tryCreateObjectTexture(textureBuilder, &chasmTextureID))
					{
						DebugLogWarning("Couldn't create chasm texture \"" + textureAsset.filename + "\".");
						continue;
					}

					ScopedObjectTextureRef chasmTextureRef(chasmTextureID, renderer);
					newTextureAssets.emplace_back(textureAsset);
					newObjectTextureRefs.emplace_back(std::move(chasmTextureRef));
				}

				newTextureList.initTextured(std::move(newTextureAssets), std::move(newObjectTextureRefs));
				chasmTextureLists.emplace_back(std::move(newTextureList));
			}
			else
			{
				DebugNotImplementedMsg(std::to_string(static_cast<int>(chasmDef.animType)));
			}

			const int chasmFloorListIndex = static_cast<int>(chasmTextureLists.size()) - 1;
			const int chasmWallIndex = chasmFloorListIndex; // @todo
			key.init(chunkPos, chasmDefID, chasmFloorListIndex, chasmWallIndex);
		}

		chasmTextureKeys.emplace_back(std::move(key));
	}
}

void SceneGraph::LoadedVoxelTexture::init(const TextureAsset &textureAsset,
	ScopedObjectTextureRef &&objectTextureRef)
{
	this->textureAsset = textureAsset;
	this->objectTextureRef = std::move(objectTextureRef);
}

SceneGraph::LoadedChasmFloorTextureList::LoadedChasmFloorTextureList()
{
	this->animType = static_cast<ChasmDefinition::AnimationType>(-1);
	this->paletteIndex = 0;
}

void SceneGraph::LoadedChasmFloorTextureList::initColor(uint8_t paletteIndex,
	ScopedObjectTextureRef &&objectTextureRef)
{
	this->animType = ChasmDefinition::AnimationType::SolidColor;
	this->paletteIndex = paletteIndex;
	this->objectTextureRefs.emplace_back(std::move(objectTextureRef));
}

void SceneGraph::LoadedChasmFloorTextureList::initTextured(std::vector<TextureAsset> &&textureAssets,
	std::vector<ScopedObjectTextureRef> &&objectTextureRefs)
{
	this->animType = ChasmDefinition::AnimationType::Animated;
	this->textureAssets = std::move(textureAssets);
	this->objectTextureRefs = std::move(objectTextureRefs);
}

int SceneGraph::LoadedChasmFloorTextureList::getTextureIndex(double chasmAnimPercent) const
{
	const int textureCount = static_cast<int>(this->objectTextureRefs.size());
	DebugAssert(textureCount >= 1);

	if (this->animType == ChasmDefinition::AnimationType::SolidColor)
	{
		return 0;
	}
	else if (this->animType == ChasmDefinition::AnimationType::Animated)
	{
		const int index = std::clamp(static_cast<int>(static_cast<double>(textureCount) * chasmAnimPercent), 0, textureCount - 1);
		return index;
	}
	else
	{
		DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(this->animType)));
	}
}

void SceneGraph::LoadedChasmTextureKey::init(const ChunkInt2 &chunkPos, VoxelChunk::ChasmDefID chasmDefID,
	int chasmFloorListIndex, int chasmWallIndex)
{
	DebugAssert(chasmFloorListIndex >= 0);
	DebugAssert(chasmWallIndex >= 0);
	this->chunkPos = chunkPos;
	this->chasmDefID = chasmDefID;
	this->chasmFloorListIndex = chasmFloorListIndex;
	this->chasmWallIndex = chasmWallIndex;
}

void SceneGraph::init(RendererSystem3D &rendererSystem)
{
	// Populate chasm wall index buffers.
	ArenaMeshUtils::ChasmWallIndexBuffer northIndices, eastIndices, southIndices, westIndices;
	ArenaMeshUtils::WriteChasmWallMeshIndexBuffers(&northIndices, &eastIndices, &southIndices, &westIndices);
	constexpr int indicesPerFace = static_cast<int>(northIndices.size());

	this->chasmWallIndexBufferIDs.fill(-1);

	for (int i = 0; i < static_cast<int>(this->chasmWallIndexBufferIDs.size()); i++)
	{
		const bool hasNorth = (i & 0x1) != 0;
		const bool hasEast = (i & 0x2) != 0;
		const bool hasSouth = (i & 0x4) != 0;
		const bool hasWest = (i & 0x8) != 0;

		auto countFace = [](bool face)
		{
			return face ? 1 : 0;
		};

		const int faceCount = countFace(hasNorth) + countFace(hasEast) + countFace(hasSouth) + countFace(hasWest);
		if (faceCount == 0)
		{
			continue;
		}

		const int indexCount = faceCount * indicesPerFace;

		IndexBufferID &indexBufferID = this->chasmWallIndexBufferIDs[i];
		if (!rendererSystem.tryCreateIndexBuffer(indexCount, &indexBufferID))
		{
			DebugLogError("Couldn't create chasm wall index buffer (" + std::to_string(i) + ").");
			continue;
		}

		std::array<int32_t, indicesPerFace * 4> totalIndicesBuffer;
		int writingIndex = 0;
		auto tryWriteIndices = [indicesPerFace, &totalIndicesBuffer, &writingIndex](bool hasFace,
			const ArenaMeshUtils::ChasmWallIndexBuffer &faceIndices)
		{
			if (hasFace)
			{
				std::copy(faceIndices.begin(), faceIndices.end(), totalIndicesBuffer.begin() + writingIndex);
				writingIndex += indicesPerFace;
			}
		};

		tryWriteIndices(hasNorth, northIndices);
		tryWriteIndices(hasEast, eastIndices);
		tryWriteIndices(hasSouth, southIndices);
		tryWriteIndices(hasWest, westIndices);

		rendererSystem.populateIndexBuffer(indexBufferID, BufferView<const int32_t>(totalIndicesBuffer.data(), writingIndex));
	}
}

void SceneGraph::shutdown(RendererSystem3D &rendererSystem)
{
	for (IndexBufferID &indexBufferID : this->chasmWallIndexBufferIDs)
	{
		if (indexBufferID >= 0)
		{
			rendererSystem.freeIndexBuffer(indexBufferID);
			indexBufferID = -1;
		}
	}
}

ObjectTextureID SceneGraph::getVoxelTextureID(const TextureAsset &textureAsset) const
{
	const auto iter = std::find_if(this->voxelTextures.begin(), this->voxelTextures.end(),
		[&textureAsset](const LoadedVoxelTexture &loadedTexture)
	{
		return loadedTexture.textureAsset == textureAsset;
	});

	DebugAssertMsg(iter != this->voxelTextures.end(), "No loaded voxel texture for \"" + textureAsset.filename + "\".");
	const ScopedObjectTextureRef &objectTextureRef = iter->objectTextureRef;
	return objectTextureRef.get();
}

ObjectTextureID SceneGraph::getChasmFloorTextureID(const ChunkInt2 &chunkPos, VoxelChunk::ChasmDefID chasmDefID,
	double chasmAnimPercent) const
{
	const auto keyIter = std::find_if(this->chasmTextureKeys.begin(), this->chasmTextureKeys.end(),
		[&chunkPos, chasmDefID](const LoadedChasmTextureKey &key)
	{
		return (key.chunkPos == chunkPos) && (key.chasmDefID == chasmDefID);
	});

	DebugAssertMsg(keyIter != this->chasmTextureKeys.end(), "No chasm texture key for chasm def ID \"" +
		std::to_string(chasmDefID) + "\" in chunk (" + chunkPos.toString() + ").");

	const int floorListIndex = keyIter->chasmFloorListIndex;
	const LoadedChasmFloorTextureList &textureList = this->chasmFloorTextureLists[floorListIndex];
	const std::vector<ScopedObjectTextureRef> &objectTextureRefs = textureList.objectTextureRefs;
	const int index = textureList.getTextureIndex(chasmAnimPercent);
	DebugAssertIndex(objectTextureRefs, index);
	const ScopedObjectTextureRef &objectTextureRef = objectTextureRefs[index];
	return objectTextureRef.get();
}

ObjectTextureID SceneGraph::getChasmWallTextureID(const ChunkInt2 &chunkPos, VoxelChunk::ChasmDefID chasmDefID) const
{
	const auto keyIter = std::find_if(this->chasmTextureKeys.begin(), this->chasmTextureKeys.end(),
		[&chunkPos, chasmDefID](const LoadedChasmTextureKey &key)
	{
		return (key.chunkPos == chunkPos) && (key.chasmDefID == chasmDefID);
	});

	DebugAssertMsg(keyIter != this->chasmTextureKeys.end(), "No chasm texture key for chasm def ID \"" +
		std::to_string(chasmDefID) + "\" in chunk (" + chunkPos.toString() + ").");

	const int wallIndex = keyIter->chasmWallIndex;
	const LoadedChasmWallTexture &wallTexture = this->chasmWallTextures[wallIndex];
	const ScopedObjectTextureRef &objectTextureRef = wallTexture.objectTextureRef;
	return objectTextureRef.get();
}

std::optional<int> SceneGraph::tryGetGraphChunkIndex(const ChunkInt2 &chunkPos) const
{
	for (int i = 0; i < static_cast<int>(this->graphChunks.size()); i++)
	{
		const SceneGraphChunk &graphChunk = this->graphChunks[i];
		if (graphChunk.position == chunkPos)
		{
			return i;
		}
	}

	return std::nullopt;
}

IndexBufferID SceneGraph::getChasmWallIndexBufferID(bool north, bool east, bool south, bool west) const
{
	const int index = (north ? 0x1 : 0) | (east ? 0x2 : 0) | (south ? 0x4 : 0) | (west ? 0x8 : 0);
	DebugAssertIndex(this->chasmWallIndexBufferIDs, index);
	return this->chasmWallIndexBufferIDs[index];
}

BufferView<const RenderDrawCall> SceneGraph::getVoxelDrawCalls() const
{
	return BufferView<const RenderDrawCall>(this->drawCallsCache.data(), static_cast<int>(this->drawCallsCache.size()));
}

void SceneGraph::loadVoxelTextures(const VoxelChunk &chunk, TextureManager &textureManager, Renderer &renderer)
{
	for (int i = 0; i < chunk.getVoxelTextureDefCount(); i++)
	{
		const VoxelTextureDefinition &voxelTextureDef = chunk.getVoxelTextureDef(i);
		sgTexture::LoadVoxelDefTextures(voxelTextureDef, this->voxelTextures, textureManager, renderer);
	}
	
	for (int i = 0; i < chunk.getChasmDefCount(); i++)
	{
		const VoxelChunk::ChasmDefID chasmDefID = static_cast<VoxelChunk::ChasmDefID>(i);
		sgTexture::LoadChasmDefTextures(chasmDefID, chunk, this->chasmFloorTextureLists, this->chasmTextureKeys,
			textureManager, renderer);
	}
}

void SceneGraph::loadVoxelMeshBuffers(SceneGraphChunk &graphChunk, const VoxelChunk &chunk, double ceilingScale,
	RendererSystem3D &rendererSystem)
{
	const ChunkInt2 &chunkPos = chunk.getPosition();

	// Add scene graph voxel mesh instances and create mappings to them.
	for (int meshDefIndex = 0; meshDefIndex < chunk.getVoxelMeshDefCount(); meshDefIndex++)
	{
		const VoxelChunk::VoxelMeshDefID voxelMeshDefID = static_cast<VoxelChunk::VoxelMeshDefID>(meshDefIndex);
		const VoxelMeshDefinition &voxelMeshDef = chunk.getVoxelMeshDef(voxelMeshDefID);

		SceneGraphVoxelMeshInstance voxelMeshInst;
		if (!voxelMeshDef.isEmpty()) // Only attempt to create buffers for non-air voxels.
		{
			constexpr int positionComponentsPerVertex = MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
			constexpr int normalComponentsPerVertex = MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
			constexpr int texCoordComponentsPerVertex = MeshUtils::TEX_COORDS_PER_VERTEX;

			const int vertexCount = voxelMeshDef.rendererVertexCount;
			if (!rendererSystem.tryCreateVertexBuffer(vertexCount, positionComponentsPerVertex, &voxelMeshInst.vertexBufferID))
			{
				DebugLogError("Couldn't create vertex buffer for voxel mesh ID " + std::to_string(voxelMeshDefID) +
					" in chunk (" + chunk.getPosition().toString() + ").");
				continue;
			}

			if (!rendererSystem.tryCreateAttributeBuffer(vertexCount, normalComponentsPerVertex, &voxelMeshInst.normalBufferID))
			{
				DebugLogError("Couldn't create normal attribute buffer for voxel mesh ID " + std::to_string(voxelMeshDefID) +
					" in chunk (" + chunk.getPosition().toString() + ").");
				voxelMeshInst.freeBuffers(rendererSystem);
				continue;
			}

			if (!rendererSystem.tryCreateAttributeBuffer(vertexCount, texCoordComponentsPerVertex, &voxelMeshInst.texCoordBufferID))
			{
				DebugLogError("Couldn't create tex coord attribute buffer for voxel mesh ID " + std::to_string(voxelMeshDefID) +
					" in chunk (" + chunk.getPosition().toString() + ").");
				voxelMeshInst.freeBuffers(rendererSystem);
				continue;
			}

			ArenaMeshUtils::InitCache meshInitCache;

			// Generate mesh geometry and indices for this voxel definition.
			voxelMeshDef.writeRendererGeometryBuffers(ceilingScale, meshInitCache.verticesView, meshInitCache.normalsView, meshInitCache.texCoordsView);
			voxelMeshDef.writeRendererIndexBuffers(meshInitCache.opaqueIndices0View, meshInitCache.opaqueIndices1View,
				meshInitCache.opaqueIndices2View, meshInitCache.alphaTestedIndices0View);

			rendererSystem.populateVertexBuffer(voxelMeshInst.vertexBufferID,
				BufferView<const double>(meshInitCache.vertices.data(), vertexCount * positionComponentsPerVertex));
			rendererSystem.populateAttributeBuffer(voxelMeshInst.normalBufferID,
				BufferView<const double>(meshInitCache.normals.data(), vertexCount * normalComponentsPerVertex));
			rendererSystem.populateAttributeBuffer(voxelMeshInst.texCoordBufferID,
				BufferView<const double>(meshInitCache.texCoords.data(), vertexCount * texCoordComponentsPerVertex));

			const int opaqueIndexBufferCount = voxelMeshDef.opaqueIndicesListCount;
			for (int bufferIndex = 0; bufferIndex < opaqueIndexBufferCount; bufferIndex++)
			{
				const int opaqueIndexCount = static_cast<int>(voxelMeshDef.getOpaqueIndicesList(bufferIndex).size());
				IndexBufferID &opaqueIndexBufferID = voxelMeshInst.opaqueIndexBufferIDs[bufferIndex];
				if (!rendererSystem.tryCreateIndexBuffer(opaqueIndexCount, &opaqueIndexBufferID))
				{
					DebugLogError("Couldn't create opaque index buffer for voxel mesh ID " +
						std::to_string(voxelMeshDefID) + " in chunk (" + chunk.getPosition().toString() + ").");
					voxelMeshInst.freeBuffers(rendererSystem);
					continue;
				}

				voxelMeshInst.opaqueIndexBufferIdCount++;

				const auto &indices = *meshInitCache.opaqueIndicesPtrs[bufferIndex];
				rendererSystem.populateIndexBuffer(opaqueIndexBufferID,
					BufferView<const int32_t>(indices.data(), opaqueIndexCount));
			}

			const bool hasAlphaTestedIndexBuffer = voxelMeshDef.alphaTestedIndicesListCount > 0;
			if (hasAlphaTestedIndexBuffer)
			{
				const int alphaTestedIndexCount = static_cast<int>(voxelMeshDef.alphaTestedIndices.size());
				if (!rendererSystem.tryCreateIndexBuffer(alphaTestedIndexCount, &voxelMeshInst.alphaTestedIndexBufferID))
				{
					DebugLogError("Couldn't create alpha-tested index buffer for voxel mesh ID " +
						std::to_string(voxelMeshDefID) + " in chunk (" + chunk.getPosition().toString() + ").");
					voxelMeshInst.freeBuffers(rendererSystem);
					continue;
				}

				rendererSystem.populateIndexBuffer(voxelMeshInst.alphaTestedIndexBufferID,
					BufferView<const int32_t>(meshInitCache.alphaTestedIndices0.data(), alphaTestedIndexCount));
			}
		}

		const SceneGraphVoxelMeshInstanceID meshInstID = graphChunk.addMeshInstance(std::move(voxelMeshInst));
		graphChunk.meshInstMappings.emplace(voxelMeshDefID, meshInstID);
	}
}

void SceneGraph::loadVoxelDrawCalls(SceneGraphChunk &graphChunk, const VoxelChunk &chunk, double ceilingScale,
	double chasmAnimPercent)
{
	auto addDrawCall = [ceilingScale](SceneGraphChunk &graphChunk, SNInt x, int y, WEInt z,
		VertexBufferID vertexBufferID, AttributeBufferID normalBufferID, AttributeBufferID texCoordBufferID,
		IndexBufferID indexBufferID, ObjectTextureID textureID, PixelShaderType pixelShaderType, bool allowsBackFaces)
	{
		RenderDrawCall drawCall;
		drawCall.vertexBufferID = vertexBufferID;
		drawCall.normalBufferID = normalBufferID;
		drawCall.texCoordBufferID = texCoordBufferID;
		drawCall.indexBufferID = indexBufferID;
		drawCall.textureIDs[0] = textureID; // @todo: add another parameter for multi-texturing for chasm walls
		drawCall.vertexShaderType = VertexShaderType::Default;
		drawCall.pixelShaderType = pixelShaderType;
		drawCall.worldSpaceOffset = Double3(
			static_cast<SNDouble>(x),
			static_cast<double>(y) * ceilingScale,
			static_cast<WEDouble>(z));
		drawCall.allowBackFaces = allowsBackFaces;

		graphChunk.voxelDrawCalls.emplace_back(std::move(drawCall));
	};

	const ChunkInt2 &chunkPos = graphChunk.position;

	// Generate draw calls for each non-air voxel.
	for (WEInt z = 0; z < graphChunk.meshInstIDs.getDepth(); z++)
	{
		for (int y = 0; y < graphChunk.meshInstIDs.getHeight(); y++)
		{
			for (SNInt x = 0; x < graphChunk.meshInstIDs.getWidth(); x++)
			{
				const VoxelChunk::VoxelMeshDefID voxelMeshDefID = chunk.getVoxelMeshDefID(x, y, z);
				const VoxelChunk::VoxelTextureDefID voxelTextureDefID = chunk.getVoxelTextureDefID(x, y, z);
				const VoxelChunk::VoxelTraitsDefID voxelTraitsDefID = chunk.getVoxelTraitsDefID(x, y, z);
				const VoxelMeshDefinition &voxelMeshDef = chunk.getVoxelMeshDef(voxelMeshDefID);
				const VoxelTextureDefinition &voxelTextureDef = chunk.getVoxelTextureDef(voxelTextureDefID);
				const VoxelTraitsDefinition &voxelTraitsDef = chunk.getVoxelTraitsDef(voxelTraitsDefID);
				if (voxelMeshDef.isEmpty())
				{
					continue;
				}

				const auto defIter = graphChunk.meshInstMappings.find(voxelMeshDefID);
				DebugAssert(defIter != graphChunk.meshInstMappings.end());
				const SceneGraphVoxelMeshInstanceID meshInstID = defIter->second;
				graphChunk.meshInstIDs.set(x, y, z, meshInstID);

				const SceneGraphVoxelMeshInstance &meshInst = graphChunk.meshInsts[meshInstID];

				// Convert voxel XYZ to world space.
				const NewInt2 worldXZ = VoxelUtils::chunkVoxelToNewVoxel(chunkPos, VoxelInt2(x, z));
				const int worldY = y;

				const bool allowsBackFaces = voxelMeshDef.allowsBackFaces;
				const ArenaTypes::VoxelType voxelType = voxelTraitsDef.type;

				VoxelChunk::ChasmDefID chasmDefID;
				const bool usesVoxelTextures = !chunk.tryGetChasmDefID(x, y, z, &chasmDefID);

				for (int bufferIndex = 0; bufferIndex < meshInst.opaqueIndexBufferIdCount; bufferIndex++)
				{
					ObjectTextureID textureID = -1;

					if (usesVoxelTextures)
					{
						const int textureAssetIndex = sgTexture::GetVoxelOpaqueTextureAssetIndex(voxelType, bufferIndex);
						const auto voxelTextureIter = std::find_if(this->voxelTextures.begin(), this->voxelTextures.end(),
							[&voxelTextureDef, textureAssetIndex](const SceneGraph::LoadedVoxelTexture &loadedTexture)
						{
							return loadedTexture.textureAsset == voxelTextureDef.getTextureAsset(textureAssetIndex);
						});

						if (voxelTextureIter != this->voxelTextures.end())
						{
							textureID = voxelTextureIter->objectTextureRef.get();
						}
						else
						{
							DebugLogError("Couldn't find opaque texture asset \"" + voxelTextureDef.getTextureAsset(textureAssetIndex).filename + "\".");
						}
					}
					else
					{
						textureID = this->getChasmFloorTextureID(chunkPos, chasmDefID, chasmAnimPercent);
					}

					if (textureID < 0)
					{
						continue;
					}

					const IndexBufferID opaqueIndexBufferID = meshInst.opaqueIndexBufferIDs[bufferIndex];
					addDrawCall(graphChunk, worldXZ.x, worldY, worldXZ.y, meshInst.vertexBufferID, meshInst.normalBufferID,
						meshInst.texCoordBufferID, opaqueIndexBufferID, textureID, PixelShaderType::Opaque, allowsBackFaces);
				}

				if (meshInst.alphaTestedIndexBufferID >= 0)
				{
					ObjectTextureID textureID = -1;

					if (usesVoxelTextures)
					{
						const int textureAssetIndex = sgTexture::GetVoxelAlphaTestedTextureAssetIndex(voxelType);
						const auto voxelTextureIter = std::find_if(this->voxelTextures.begin(), this->voxelTextures.end(),
							[&voxelTextureDef, textureAssetIndex](const SceneGraph::LoadedVoxelTexture &loadedTexture)
						{
							return loadedTexture.textureAsset == voxelTextureDef.getTextureAsset(textureAssetIndex);
						});

						if (voxelTextureIter != this->voxelTextures.end())
						{
							textureID = voxelTextureIter->objectTextureRef.get();
						}
						else
						{
							DebugLogError("Couldn't find alpha-tested texture asset \"" + voxelTextureDef.getTextureAsset(textureAssetIndex).filename + "\".");
						}
					}
					else
					{
						textureID = this->getChasmWallTextureID(chunkPos, chasmDefID);
					}

					if (textureID < 0)
					{
						continue;
					}

					addDrawCall(graphChunk, worldXZ.x, worldY, worldXZ.y, meshInst.vertexBufferID, meshInst.normalBufferID,
						meshInst.texCoordBufferID, meshInst.alphaTestedIndexBufferID, textureID, PixelShaderType::AlphaTested,
						allowsBackFaces);
				}
			}
		}
	}
}

void SceneGraph::loadVoxelChunk(const VoxelChunk &chunk, double ceilingScale, TextureManager &textureManager,
	Renderer &renderer, RendererSystem3D &rendererSystem)
{
	const ChunkInt2 &chunkPos = chunk.getPosition();
	SceneGraphChunk graphChunk;
	graphChunk.init(chunkPos, chunk.getHeight());

	this->loadVoxelTextures(chunk, textureManager, renderer);
	this->loadVoxelMeshBuffers(graphChunk, chunk, ceilingScale, rendererSystem);

	this->graphChunks.emplace_back(std::move(graphChunk));
}

void SceneGraph::rebuildVoxelChunkDrawCalls(const VoxelChunk &voxelChunk, double ceilingScale, double chasmAnimPercent)
{
	const ChunkInt2 chunkPos = voxelChunk.getPosition();
	const std::optional<int> graphChunkIndex = this->tryGetGraphChunkIndex(chunkPos);
	if (!graphChunkIndex.has_value())
	{
		DebugLogError("No scene graph chunk available at (" + chunkPos.toString() + ").");
		return;
	}

	SceneGraphChunk &graphChunk = this->graphChunks[*graphChunkIndex];
	graphChunk.voxelDrawCalls.clear();

	this->loadVoxelDrawCalls(graphChunk, voxelChunk, ceilingScale, chasmAnimPercent);
}

void SceneGraph::unloadVoxelChunk(const ChunkInt2 &chunkPos, RendererSystem3D &rendererSystem)
{
	const auto iter = std::find_if(this->graphChunks.begin(), this->graphChunks.end(),
		[&chunkPos](const SceneGraphChunk &graphChunk)
	{
		return graphChunk.position == chunkPos;
	});

	if (iter != this->graphChunks.end())
	{
		SceneGraphChunk &graphChunk = *iter;
		graphChunk.freeBuffers(rendererSystem);
		this->graphChunks.erase(iter);
	}
}

void SceneGraph::rebuildVoxelDrawCallsList()
{
	this->drawCallsCache.clear();

	// @todo: eventually this should sort by distance from a CoordDouble2
	for (size_t i = 0; i < this->graphChunks.size(); i++)
	{
		const SceneGraphChunk &graphChunk = this->graphChunks[i];
		const auto &srcDrawCalls = graphChunk.voxelDrawCalls;
		this->drawCallsCache.insert(this->drawCallsCache.end(), srcDrawCalls.begin(), srcDrawCalls.end());
	}
}

void SceneGraph::unloadScene(RendererSystem3D &rendererSystem)
{
	this->voxelTextures.clear();
	this->chasmFloorTextureLists.clear();
	this->chasmWallTextures.clear();
	this->chasmTextureKeys.clear();

	// Free vertex/attribute/index buffer IDs from renderer.
	for (SceneGraphChunk &chunk : this->graphChunks)
	{
		chunk.freeBuffers(rendererSystem);
	}

	this->graphChunks.clear();
	this->drawCallsCache.clear();
}
