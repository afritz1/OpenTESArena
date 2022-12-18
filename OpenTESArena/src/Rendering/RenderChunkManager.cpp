#include <array>
#include <numeric>
#include <optional>

#include "RenderChunkManager.h"
#include "ArenaRenderUtils.h"
#include "RenderCamera.h"
#include "Renderer.h"
#include "RendererSystem3D.h"
#include "../Assets/MIFUtils.h"
#include "../Assets/TextureManager.h"
#include "../Entities/EntityManager.h"
#include "../Entities/EntityVisibilityState.h"
#include "../Math/Constants.h"
#include "../Math/Matrix4.h"
#include "../Voxels/VoxelFacing2D.h"
#include "../World/ArenaMeshUtils.h"
#include "../World/ChunkManager.h"
#include "../World/LevelInstance.h"
#include "../World/MapDefinition.h"
#include "../World/MapType.h"

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
	void LoadVoxelDefTextures(const VoxelTextureDefinition &voxelTextureDef,
		std::vector<RenderChunkManager::LoadedVoxelTexture> &voxelTextures, TextureManager &textureManager, Renderer &renderer)
	{
		for (int i = 0; i < voxelTextureDef.textureCount; i++)
		{
			const TextureAsset &textureAsset = voxelTextureDef.getTextureAsset(i);
			const auto cacheIter = std::find_if(voxelTextures.begin(), voxelTextures.end(),
				[&textureAsset](const RenderChunkManager::LoadedVoxelTexture &loadedTexture)
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
				RenderChunkManager::LoadedVoxelTexture newTexture;
				newTexture.init(textureAsset, std::move(voxelTextureRef));
				voxelTextures.emplace_back(std::move(newTexture));
			}
		}
	}

	bool LoadedChasmFloorComparer(const RenderChunkManager::LoadedChasmFloorTextureList &textureList, const ChasmDefinition &chasmDef)
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

	void LoadChasmDefTextures(VoxelChunk::ChasmDefID chasmDefID, const VoxelChunk &chunk,
		const std::vector<RenderChunkManager::LoadedVoxelTexture> &voxelTextures,
		std::vector<RenderChunkManager::LoadedChasmFloorTextureList> &chasmFloorTextureLists,
		std::vector<RenderChunkManager::LoadedChasmTextureKey> &chasmTextureKeys,
		TextureManager &textureManager, Renderer &renderer)
	{
		const ChunkInt2 chunkPos = chunk.getPosition();
		const ChasmDefinition &chasmDef = chunk.getChasmDef(chasmDefID);

		// Check if this chasm already has a mapping (i.e. have we seen this chunk before?).
		const auto keyIter = std::find_if(chasmTextureKeys.begin(), chasmTextureKeys.end(),
			[chasmDefID, &chunkPos](const RenderChunkManager::LoadedChasmTextureKey &loadedKey)
		{
			return (loadedKey.chasmDefID == chasmDefID) && (loadedKey.chunkPos == chunkPos);
		});

		if (keyIter != chasmTextureKeys.end())
		{
			return;
		}

		// Check if any loaded chasm floors reference the same asset(s).
		const auto chasmFloorIter = std::find_if(chasmFloorTextureLists.begin(), chasmFloorTextureLists.end(),
			[&chasmDef](const RenderChunkManager::LoadedChasmFloorTextureList &textureList)
		{
			return LoadedChasmFloorComparer(textureList, chasmDef);
		});

		int chasmFloorListIndex = -1;
		if (chasmFloorIter != chasmFloorTextureLists.end())
		{
			chasmFloorListIndex = static_cast<int>(std::distance(chasmFloorTextureLists.begin(), chasmFloorIter));
		}
		else
		{
			// Load the required textures and add a key for them.
			RenderChunkManager::LoadedChasmFloorTextureList newTextureList;
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
				chasmFloorTextureLists.emplace_back(std::move(newTextureList));
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
				chasmFloorTextureLists.emplace_back(std::move(newTextureList));
			}
			else
			{
				DebugNotImplementedMsg(std::to_string(static_cast<int>(chasmDef.animType)));
			}

			chasmFloorListIndex = static_cast<int>(chasmFloorTextureLists.size()) - 1;
		}

		// The chasm wall (if any) should already be loaded as a voxel texture during map gen.
		// @todo: support chasm walls adding to the voxel textures list (i.e. for destroyed voxels; the list would have to be non-const)
		const auto chasmWallIter = std::find_if(voxelTextures.begin(), voxelTextures.end(),
			[&chasmDef](const RenderChunkManager::LoadedVoxelTexture &voxelTexture)
		{
			return voxelTexture.textureAsset == chasmDef.wallTextureAsset;
		});

		DebugAssert(chasmWallIter != voxelTextures.end());
		const int chasmWallIndex = static_cast<int>(std::distance(voxelTextures.begin(), chasmWallIter));

		DebugAssert(chasmFloorListIndex >= 0);
		DebugAssert(chasmWallIndex >= 0);

		RenderChunkManager::LoadedChasmTextureKey key;
		key.init(chunkPos, chasmDefID, chasmFloorListIndex, chasmWallIndex);
		chasmTextureKeys.emplace_back(std::move(key));
	}
}

void RenderChunkManager::LoadedVoxelTexture::init(const TextureAsset &textureAsset,
	ScopedObjectTextureRef &&objectTextureRef)
{
	this->textureAsset = textureAsset;
	this->objectTextureRef = std::move(objectTextureRef);
}

RenderChunkManager::LoadedChasmFloorTextureList::LoadedChasmFloorTextureList()
{
	this->animType = static_cast<ChasmDefinition::AnimationType>(-1);
	this->paletteIndex = 0;
}

void RenderChunkManager::LoadedChasmFloorTextureList::initColor(uint8_t paletteIndex,
	ScopedObjectTextureRef &&objectTextureRef)
{
	this->animType = ChasmDefinition::AnimationType::SolidColor;
	this->paletteIndex = paletteIndex;
	this->objectTextureRefs.emplace_back(std::move(objectTextureRef));
}

void RenderChunkManager::LoadedChasmFloorTextureList::initTextured(std::vector<TextureAsset> &&textureAssets,
	std::vector<ScopedObjectTextureRef> &&objectTextureRefs)
{
	this->animType = ChasmDefinition::AnimationType::Animated;
	this->textureAssets = std::move(textureAssets);
	this->objectTextureRefs = std::move(objectTextureRefs);
}

int RenderChunkManager::LoadedChasmFloorTextureList::getTextureIndex(double chasmAnimPercent) const
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

void RenderChunkManager::LoadedChasmTextureKey::init(const ChunkInt2 &chunkPos, VoxelChunk::ChasmDefID chasmDefID,
	int chasmFloorListIndex, int chasmWallIndex)
{
	this->chunkPos = chunkPos;
	this->chasmDefID = chasmDefID;
	this->chasmFloorListIndex = chasmFloorListIndex;
	this->chasmWallIndex = chasmWallIndex;
}

void RenderChunkManager::init(Renderer &renderer)
{
	// Populate chasm wall index buffers.
	ArenaMeshUtils::ChasmWallIndexBuffer northIndices, eastIndices, southIndices, westIndices;
	ArenaMeshUtils::WriteChasmWallRendererIndexBuffers(&northIndices, &eastIndices, &southIndices, &westIndices);
	constexpr int indicesPerFace = static_cast<int>(northIndices.size());

	this->chasmWallIndexBufferIDs.fill(-1);

	for (int i = 0; i < static_cast<int>(this->chasmWallIndexBufferIDs.size()); i++)
	{
		const int baseIndex = i + 1;
		const bool hasNorth = (baseIndex & ArenaMeshUtils::CHASM_WALL_NORTH) != 0;
		const bool hasEast = (baseIndex & ArenaMeshUtils::CHASM_WALL_EAST) != 0;
		const bool hasSouth = (baseIndex & ArenaMeshUtils::CHASM_WALL_SOUTH) != 0;
		const bool hasWest = (baseIndex & ArenaMeshUtils::CHASM_WALL_WEST) != 0;

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
		if (!renderer.tryCreateIndexBuffer(indexCount, &indexBufferID))
		{
			DebugLogError("Couldn't create chasm wall index buffer " + std::to_string(i) + ".");
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

		renderer.populateIndexBuffer(indexBufferID, BufferView<const int32_t>(totalIndicesBuffer.data(), writingIndex));
	}
}

void RenderChunkManager::shutdown(Renderer &renderer)
{
	for (RenderChunk &renderChunk : this->renderChunks)
	{
		renderChunk.freeBuffers(renderer);
	}

	this->renderChunks.clear();

	for (IndexBufferID &indexBufferID : this->chasmWallIndexBufferIDs)
	{
		renderer.freeIndexBuffer(indexBufferID);
		indexBufferID = -1;
	}

	this->voxelTextures.clear();
	this->chasmFloorTextureLists.clear();
	this->chasmTextureKeys.clear();
}

ObjectTextureID RenderChunkManager::getVoxelTextureID(const TextureAsset &textureAsset) const
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

ObjectTextureID RenderChunkManager::getChasmFloorTextureID(const ChunkInt2 &chunkPos, VoxelChunk::ChasmDefID chasmDefID,
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
	DebugAssertIndex(this->chasmFloorTextureLists, floorListIndex);
	const LoadedChasmFloorTextureList &textureList = this->chasmFloorTextureLists[floorListIndex];
	const std::vector<ScopedObjectTextureRef> &objectTextureRefs = textureList.objectTextureRefs;
	const int index = textureList.getTextureIndex(chasmAnimPercent);
	DebugAssertIndex(objectTextureRefs, index);
	const ScopedObjectTextureRef &objectTextureRef = objectTextureRefs[index];
	return objectTextureRef.get();
}

ObjectTextureID RenderChunkManager::getChasmWallTextureID(const ChunkInt2 &chunkPos, VoxelChunk::ChasmDefID chasmDefID) const
{
	const auto keyIter = std::find_if(this->chasmTextureKeys.begin(), this->chasmTextureKeys.end(),
		[&chunkPos, chasmDefID](const LoadedChasmTextureKey &key)
	{
		return (key.chunkPos == chunkPos) && (key.chasmDefID == chasmDefID);
	});

	DebugAssertMsg(keyIter != this->chasmTextureKeys.end(), "No chasm texture key for chasm def ID \"" +
		std::to_string(chasmDefID) + "\" in chunk (" + chunkPos.toString() + ").");

	const int wallIndex = keyIter->chasmWallIndex;
	const LoadedVoxelTexture &voxelTexture = this->voxelTextures[wallIndex];
	const ScopedObjectTextureRef &objectTextureRef = voxelTexture.objectTextureRef;
	return objectTextureRef.get();
}

std::optional<int> RenderChunkManager::tryGetRenderChunkIndex(const ChunkInt2 &chunkPos) const
{
	for (int i = 0; i < static_cast<int>(this->renderChunks.size()); i++)
	{
		const RenderChunk &renderChunk = this->renderChunks[i];
		if (renderChunk.position == chunkPos)
		{
			return i;
		}
	}

	return std::nullopt;
}

BufferView<const RenderDrawCall> RenderChunkManager::getVoxelDrawCalls() const
{
	return BufferView<const RenderDrawCall>(this->drawCallsCache.data(), static_cast<int>(this->drawCallsCache.size()));
}

void RenderChunkManager::loadVoxelTextures(const VoxelChunk &chunk, TextureManager &textureManager, Renderer &renderer)
{
	for (int i = 0; i < chunk.getVoxelTextureDefCount(); i++)
	{
		const VoxelTextureDefinition &voxelTextureDef = chunk.getVoxelTextureDef(i);
		sgTexture::LoadVoxelDefTextures(voxelTextureDef, this->voxelTextures, textureManager, renderer);
	}

	for (int i = 0; i < chunk.getChasmDefCount(); i++)
	{
		const VoxelChunk::ChasmDefID chasmDefID = static_cast<VoxelChunk::ChasmDefID>(i);
		sgTexture::LoadChasmDefTextures(chasmDefID, chunk, this->voxelTextures, this->chasmFloorTextureLists,
			this->chasmTextureKeys, textureManager, renderer);
	}
}

void RenderChunkManager::loadVoxelMeshBuffers(RenderChunk &renderChunk, const VoxelChunk &chunk, double ceilingScale, Renderer &renderer)
{
	const ChunkInt2 &chunkPos = chunk.getPosition();

	// Add render chunk voxel mesh instances and create mappings to them.
	for (int meshDefIndex = 0; meshDefIndex < chunk.getVoxelMeshDefCount(); meshDefIndex++)
	{
		const VoxelChunk::VoxelMeshDefID voxelMeshDefID = static_cast<VoxelChunk::VoxelMeshDefID>(meshDefIndex);
		const VoxelMeshDefinition &voxelMeshDef = chunk.getVoxelMeshDef(voxelMeshDefID);

		RenderChunkVoxelMeshInstance voxelMeshInst;
		if (!voxelMeshDef.isEmpty()) // Only attempt to create buffers for non-air voxels.
		{
			constexpr int positionComponentsPerVertex = MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
			constexpr int normalComponentsPerVertex = MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
			constexpr int texCoordComponentsPerVertex = MeshUtils::TEX_COORDS_PER_VERTEX;

			const int vertexCount = voxelMeshDef.rendererVertexCount;
			if (!renderer.tryCreateVertexBuffer(vertexCount, positionComponentsPerVertex, &voxelMeshInst.vertexBufferID))
			{
				DebugLogError("Couldn't create vertex buffer for voxel mesh ID " + std::to_string(voxelMeshDefID) +
					" in chunk (" + chunk.getPosition().toString() + ").");
				continue;
			}

			if (!renderer.tryCreateAttributeBuffer(vertexCount, normalComponentsPerVertex, &voxelMeshInst.normalBufferID))
			{
				DebugLogError("Couldn't create normal attribute buffer for voxel mesh ID " + std::to_string(voxelMeshDefID) +
					" in chunk (" + chunk.getPosition().toString() + ").");
				voxelMeshInst.freeBuffers(renderer);
				continue;
			}

			if (!renderer.tryCreateAttributeBuffer(vertexCount, texCoordComponentsPerVertex, &voxelMeshInst.texCoordBufferID))
			{
				DebugLogError("Couldn't create tex coord attribute buffer for voxel mesh ID " + std::to_string(voxelMeshDefID) +
					" in chunk (" + chunk.getPosition().toString() + ").");
				voxelMeshInst.freeBuffers(renderer);
				continue;
			}

			ArenaMeshUtils::RenderMeshInitCache meshInitCache;

			// Generate mesh geometry and indices for this voxel definition.
			voxelMeshDef.writeRendererGeometryBuffers(ceilingScale, meshInitCache.verticesView, meshInitCache.normalsView, meshInitCache.texCoordsView);
			voxelMeshDef.writeRendererIndexBuffers(meshInitCache.opaqueIndices0View, meshInitCache.opaqueIndices1View,
				meshInitCache.opaqueIndices2View, meshInitCache.alphaTestedIndices0View);

			renderer.populateVertexBuffer(voxelMeshInst.vertexBufferID,
				BufferView<const double>(meshInitCache.vertices.data(), vertexCount * positionComponentsPerVertex));
			renderer.populateAttributeBuffer(voxelMeshInst.normalBufferID,
				BufferView<const double>(meshInitCache.normals.data(), vertexCount * normalComponentsPerVertex));
			renderer.populateAttributeBuffer(voxelMeshInst.texCoordBufferID,
				BufferView<const double>(meshInitCache.texCoords.data(), vertexCount * texCoordComponentsPerVertex));

			const int opaqueIndexBufferCount = voxelMeshDef.opaqueIndicesListCount;
			for (int bufferIndex = 0; bufferIndex < opaqueIndexBufferCount; bufferIndex++)
			{
				const int opaqueIndexCount = static_cast<int>(voxelMeshDef.getOpaqueIndicesList(bufferIndex).size());
				IndexBufferID &opaqueIndexBufferID = voxelMeshInst.opaqueIndexBufferIDs[bufferIndex];
				if (!renderer.tryCreateIndexBuffer(opaqueIndexCount, &opaqueIndexBufferID))
				{
					DebugLogError("Couldn't create opaque index buffer for voxel mesh ID " +
						std::to_string(voxelMeshDefID) + " in chunk (" + chunk.getPosition().toString() + ").");
					voxelMeshInst.freeBuffers(renderer);
					continue;
				}

				voxelMeshInst.opaqueIndexBufferIdCount++;

				const auto &indices = *meshInitCache.opaqueIndicesPtrs[bufferIndex];
				renderer.populateIndexBuffer(opaqueIndexBufferID,
					BufferView<const int32_t>(indices.data(), opaqueIndexCount));
			}

			const bool hasAlphaTestedIndexBuffer = voxelMeshDef.alphaTestedIndicesListCount > 0;
			if (hasAlphaTestedIndexBuffer)
			{
				const int alphaTestedIndexCount = static_cast<int>(voxelMeshDef.alphaTestedIndices.size());
				if (!renderer.tryCreateIndexBuffer(alphaTestedIndexCount, &voxelMeshInst.alphaTestedIndexBufferID))
				{
					DebugLogError("Couldn't create alpha-tested index buffer for voxel mesh ID " +
						std::to_string(voxelMeshDefID) + " in chunk (" + chunk.getPosition().toString() + ").");
					voxelMeshInst.freeBuffers(renderer);
					continue;
				}

				renderer.populateIndexBuffer(voxelMeshInst.alphaTestedIndexBufferID,
					BufferView<const int32_t>(meshInitCache.alphaTestedIndices0.data(), alphaTestedIndexCount));
			}
		}

		const RenderChunkVoxelMeshInstanceID meshInstID = renderChunk.addMeshInstance(std::move(voxelMeshInst));
		renderChunk.meshInstMappings.emplace(voxelMeshDefID, meshInstID);
	}
}

void RenderChunkManager::loadVoxelChasmWalls(RenderChunk &renderChunk, const VoxelChunk &chunk)
{
	DebugAssert(renderChunk.chasmWallIndexBufferIDs.empty());

	for (WEInt z = 0; z < VoxelChunk::DEPTH; z++)
	{
		for (int y = 0; y < chunk.getHeight(); y++)
		{
			for (SNInt x = 0; x < VoxelChunk::WIDTH; x++)
			{
				int chasmWallInstIndex;
				if (!chunk.tryGetChasmWallInstIndex(x, y, z, &chasmWallInstIndex))
				{
					continue;
				}

				const VoxelChasmWallInstance &chasmWallInst = chunk.getChasmWallInst(chasmWallInstIndex);
				DebugAssert(chasmWallInst.getFaceCount() > 0);

				const int chasmWallIndexBufferIndex = ArenaMeshUtils::GetChasmWallIndex(
					chasmWallInst.north, chasmWallInst.east, chasmWallInst.south, chasmWallInst.west);
				const IndexBufferID indexBufferID = this->chasmWallIndexBufferIDs[chasmWallIndexBufferIndex];

				renderChunk.chasmWallIndexBufferIDs.emplace(VoxelInt3(x, y, z), indexBufferID);
			}
		}
	}
}

void RenderChunkManager::loadVoxelDrawCalls(RenderChunk &renderChunk, const VoxelChunk &chunk, double ceilingScale,
	double chasmAnimPercent, bool updateStatics, bool updateAnimating)
{
	auto addDrawCall = [&renderChunk, ceilingScale](SNInt x, int y, WEInt z, VertexBufferID vertexBufferID,
		AttributeBufferID normalBufferID, AttributeBufferID texCoordBufferID, IndexBufferID indexBufferID,
		ObjectTextureID textureID0, const std::optional<ObjectTextureID> &textureID1, TextureSamplingType textureSamplingType,
		PixelShaderType pixelShaderType, bool isAnimating)
	{
		RenderDrawCall drawCall;
		drawCall.vertexBufferID = vertexBufferID;
		drawCall.normalBufferID = normalBufferID;
		drawCall.texCoordBufferID = texCoordBufferID;
		drawCall.indexBufferID = indexBufferID;
		drawCall.textureIDs[0] = textureID0;
		drawCall.textureIDs[1] = textureID1;
		drawCall.textureSamplingType = textureSamplingType;
		drawCall.vertexShaderType = VertexShaderType::Default;
		drawCall.pixelShaderType = pixelShaderType;
		drawCall.worldSpaceOffset = Double3(
			static_cast<SNDouble>(x),
			static_cast<double>(y) * ceilingScale,
			static_cast<WEDouble>(z));

		if (!isAnimating)
		{
			renderChunk.staticDrawCalls.emplace_back(std::move(drawCall));
		}
		else
		{
			renderChunk.animatingDrawCalls.emplace_back(std::move(drawCall));
		}
	};

	const ChunkInt2 &chunkPos = renderChunk.position;

	// Generate draw calls for each non-air voxel.
	for (WEInt z = 0; z < renderChunk.meshInstIDs.getDepth(); z++)
	{
		for (int y = 0; y < renderChunk.meshInstIDs.getHeight(); y++)
		{
			for (SNInt x = 0; x < renderChunk.meshInstIDs.getWidth(); x++)
			{
				const VoxelInt3 voxel(x, y, z);
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

				const auto defIter = renderChunk.meshInstMappings.find(voxelMeshDefID);
				DebugAssert(defIter != renderChunk.meshInstMappings.end());
				const RenderChunkVoxelMeshInstanceID meshInstID = defIter->second;
				renderChunk.meshInstIDs.set(x, y, z, meshInstID);

				const RenderChunkVoxelMeshInstance &meshInst = renderChunk.meshInsts[meshInstID];

				// Convert voxel XYZ to world space.
				const NewInt2 worldXZ = VoxelUtils::chunkVoxelToNewVoxel(chunkPos, VoxelInt2(x, z));
				const int worldY = y;

				const ArenaTypes::VoxelType voxelType = voxelTraitsDef.type;

				VoxelChunk::ChasmDefID chasmDefID;
				const bool usesVoxelTextures = !chunk.tryGetChasmDefID(x, y, z, &chasmDefID);

				const bool isAnimating = !usesVoxelTextures;
				if ((!isAnimating && updateStatics) || (isAnimating && updateAnimating))
				{
					for (int bufferIndex = 0; bufferIndex < meshInst.opaqueIndexBufferIdCount; bufferIndex++)
					{
						ObjectTextureID textureID = -1;

						if (usesVoxelTextures)
						{
							const int textureAssetIndex = sgTexture::GetVoxelOpaqueTextureAssetIndex(voxelType, bufferIndex);
							const auto voxelTextureIter = std::find_if(this->voxelTextures.begin(), this->voxelTextures.end(),
								[&voxelTextureDef, textureAssetIndex](const RenderChunkManager::LoadedVoxelTexture &loadedTexture)
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
						const TextureSamplingType textureSamplingType = usesVoxelTextures ? TextureSamplingType::Default : TextureSamplingType::ScreenSpaceRepeatY;
						addDrawCall(worldXZ.x, worldY, worldXZ.y, meshInst.vertexBufferID, meshInst.normalBufferID,
							meshInst.texCoordBufferID, opaqueIndexBufferID, textureID, std::nullopt, textureSamplingType,
							PixelShaderType::Opaque, isAnimating);
					}
				}

				if (meshInst.alphaTestedIndexBufferID >= 0)
				{
					if (updateStatics)
					{
						DebugAssert(usesVoxelTextures);
						ObjectTextureID textureID = -1;

						const int textureAssetIndex = sgTexture::GetVoxelAlphaTestedTextureAssetIndex(voxelType);
						const auto voxelTextureIter = std::find_if(this->voxelTextures.begin(), this->voxelTextures.end(),
							[&voxelTextureDef, textureAssetIndex](const RenderChunkManager::LoadedVoxelTexture &loadedTexture)
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

						if (textureID < 0)
						{
							continue;
						}

						const bool isAnimating = false;
						addDrawCall(worldXZ.x, worldY, worldXZ.y, meshInst.vertexBufferID, meshInst.normalBufferID,
							meshInst.texCoordBufferID, meshInst.alphaTestedIndexBufferID, textureID, std::nullopt,
							TextureSamplingType::Default, PixelShaderType::AlphaTested, isAnimating);
					}
				}

				if (!usesVoxelTextures)
				{
					const auto chasmWallIter = renderChunk.chasmWallIndexBufferIDs.find(voxel);
					if (chasmWallIter != renderChunk.chasmWallIndexBufferIDs.end())
					{
						DebugAssert(voxelTraitsDef.type == ArenaTypes::VoxelType::Chasm);
						const bool isAnimating = voxelTraitsDef.chasm.type != ArenaTypes::ChasmType::Dry;

						if ((!isAnimating && updateStatics) || (isAnimating && updateAnimating))
						{
							const IndexBufferID chasmWallIndexBufferID = chasmWallIter->second;

							// Need to give two textures since chasm walls are multi-textured.
							ObjectTextureID textureID0 = this->getChasmFloorTextureID(chunkPos, chasmDefID, chasmAnimPercent);
							ObjectTextureID textureID1 = this->getChasmWallTextureID(chunkPos, chasmDefID);

							addDrawCall(worldXZ.x, worldY, worldXZ.y, meshInst.vertexBufferID, meshInst.normalBufferID,
								meshInst.texCoordBufferID, chasmWallIndexBufferID, textureID0, textureID1, TextureSamplingType::ScreenSpaceRepeatY,
								PixelShaderType::OpaqueWithAlphaTestLayer, isAnimating);
						}
					}
				}
			}
		}
	}
}

void RenderChunkManager::loadVoxelChunk(const VoxelChunk &chunk, double ceilingScale, TextureManager &textureManager, Renderer &renderer)
{
	const ChunkInt2 &chunkPos = chunk.getPosition();
	RenderChunk renderChunk;
	renderChunk.init(chunkPos, chunk.getHeight());

	this->loadVoxelTextures(chunk, textureManager, renderer);
	this->loadVoxelMeshBuffers(renderChunk, chunk, ceilingScale, renderer);
	this->loadVoxelChasmWalls(renderChunk, chunk);

	this->renderChunks.emplace_back(std::move(renderChunk));
}

void RenderChunkManager::rebuildVoxelChunkDrawCalls(const VoxelChunk &voxelChunk, double ceilingScale,
	double chasmAnimPercent, bool updateStatics, bool updateAnimating)
{
	const ChunkInt2 chunkPos = voxelChunk.getPosition();
	const std::optional<int> renderChunkIndex = this->tryGetRenderChunkIndex(chunkPos);
	if (!renderChunkIndex.has_value())
	{
		DebugLogError("No render chunk available at (" + chunkPos.toString() + ").");
		return;
	}

	RenderChunk &renderChunk = this->renderChunks[*renderChunkIndex];
	if (updateStatics)
	{
		renderChunk.staticDrawCalls.clear();
	}

	if (updateAnimating)
	{
		renderChunk.animatingDrawCalls.clear();
	}

	this->loadVoxelDrawCalls(renderChunk, voxelChunk, ceilingScale, chasmAnimPercent, updateStatics, updateAnimating);
}

void RenderChunkManager::unloadVoxelChunk(const ChunkInt2 &chunkPos, Renderer &renderer)
{
	const auto iter = std::find_if(this->renderChunks.begin(), this->renderChunks.end(),
		[&chunkPos](const RenderChunk &renderChunk)
	{
		return renderChunk.position == chunkPos;
	});

	if (iter != this->renderChunks.end())
	{
		RenderChunk &renderChunk = *iter;
		renderChunk.freeBuffers(renderer);
		this->renderChunks.erase(iter);
	}
}

void RenderChunkManager::rebuildVoxelDrawCallsList()
{
	this->drawCallsCache.clear();

	// @todo: eventually this should sort by distance from a CoordDouble2
	for (size_t i = 0; i < this->renderChunks.size(); i++)
	{
		const RenderChunk &renderChunk = this->renderChunks[i];
		const std::vector<RenderDrawCall> &staticDrawCalls = renderChunk.staticDrawCalls;
		const std::vector<RenderDrawCall> &animatingDrawCalls = renderChunk.animatingDrawCalls;
		this->drawCallsCache.insert(this->drawCallsCache.end(), staticDrawCalls.begin(), staticDrawCalls.end());
		this->drawCallsCache.insert(this->drawCallsCache.end(), animatingDrawCalls.begin(), animatingDrawCalls.end());
	}
}

void RenderChunkManager::unloadScene(Renderer &renderer)
{
	this->voxelTextures.clear();
	this->chasmFloorTextureLists.clear();
	this->chasmTextureKeys.clear();

	// Free vertex/attribute/index buffer IDs from renderer.
	for (RenderChunk &renderChunk : this->renderChunks)
	{
		renderChunk.freeBuffers(renderer);
	}

	this->renderChunks.clear();
	this->drawCallsCache.clear();
}
