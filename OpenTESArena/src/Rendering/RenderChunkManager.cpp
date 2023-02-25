#include <algorithm>
#include <array>
#include <numeric>
#include <optional>

#include "ArenaRenderUtils.h"
#include "RenderCamera.h"
#include "RenderChunkManager.h"
#include "Renderer.h"
#include "RendererSystem3D.h"
#include "../Assets/MIFUtils.h"
#include "../Assets/TextureManager.h"
#include "../Entities/EntityManager.h"
#include "../Entities/EntityVisibilityState.h"
#include "../Math/Constants.h"
#include "../Math/Matrix4.h"
#include "../Voxels/DoorUtils.h"
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

	void LoadChasmDefTextures(VoxelChunk::ChasmDefID chasmDefID, const VoxelChunk &voxelChunk,
		const std::vector<RenderChunkManager::LoadedVoxelTexture> &voxelTextures,
		std::vector<RenderChunkManager::LoadedChasmFloorTextureList> &chasmFloorTextureLists,
		std::vector<RenderChunkManager::LoadedChasmTextureKey> &chasmTextureKeys,
		TextureManager &textureManager, Renderer &renderer)
	{
		const ChunkInt2 chunkPos = voxelChunk.getPosition();
		const ChasmDefinition &chasmDef = voxelChunk.getChasmDef(chasmDefID);

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
				if (!renderer.tryCreateObjectTexture(1, 1, 1, &dryChasmTextureID))
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

				DebugAssert(lockedTexture.bytesPerTexel == 1);
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

	// Creates a buffer of texture refs, intended to be accessed with linearized keyframe indices.
	Buffer<ScopedObjectTextureRef> MakeEntityAnimationTextures(const EntityAnimationDefinition &animDef,
		TextureManager &textureManager, Renderer &renderer)
	{
		const int keyframeCount = animDef.keyframeCount;
		Buffer<ScopedObjectTextureRef> textureRefs(keyframeCount);
		for (int i = 0; i < keyframeCount; i++)
		{
			const EntityAnimationDefinitionKeyframe &keyframe = animDef.keyframes[i];
			const TextureAsset &textureAsset = keyframe.textureAsset;

			const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAsset);
			if (!textureBuilderID.has_value())
			{
				DebugLogWarning("Couldn't load entity anim texture \"" + textureAsset.filename + "\".");
				continue;
			}

			const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
			ObjectTextureID textureID;
			if (!renderer.tryCreateObjectTexture(textureBuilder, &textureID))
			{
				DebugLogWarning("Couldn't create entity anim texture \"" + textureAsset.filename + "\".");
				continue;
			}

			ScopedObjectTextureRef textureRef(textureID, renderer);
			textureRefs.set(i, std::move(textureRef));
		}

		return textureRefs;
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

void RenderChunkManager::LoadedEntityAnimation::init(EntityDefID defID, Buffer<ScopedObjectTextureRef> &&textureRefs)
{
	this->defID = defID;
	this->textureRefs = std::move(textureRefs);
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

	// Populate entity mesh buffers. All entities share the same buffers, and the normals buffer is updated every frame.
	constexpr int positionComponentsPerVertex = MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
	constexpr int normalComponentsPerVertex = MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
	constexpr int texCoordComponentsPerVertex = MeshUtils::TEX_COORDS_PER_VERTEX;
	constexpr int entityMeshVertexCount = 4;
	constexpr int entityMeshIndexCount = 6;

	if (!renderer.tryCreateVertexBuffer(entityMeshVertexCount, positionComponentsPerVertex, &this->entityMeshDef.vertexBufferID))
	{
		DebugLogError("Couldn't create vertex buffer for entity mesh ID.");
		return;
	}

	if (!renderer.tryCreateAttributeBuffer(entityMeshVertexCount, normalComponentsPerVertex, &this->entityMeshDef.normalBufferID))
	{
		DebugLogError("Couldn't create normal attribute buffer for entity mesh def.");
		this->entityMeshDef.freeBuffers(renderer);
		return;
	}

	if (!renderer.tryCreateAttributeBuffer(entityMeshVertexCount, texCoordComponentsPerVertex, &this->entityMeshDef.texCoordBufferID))
	{
		DebugLogError("Couldn't create tex coord attribute buffer for entity mesh def.");
		this->entityMeshDef.freeBuffers(renderer);
		return;
	}

	if (!renderer.tryCreateIndexBuffer(entityMeshIndexCount, &this->entityMeshDef.indexBufferID))
	{
		DebugLogError("Couldn't create index buffer for entity mesh def.");
		this->entityMeshDef.freeBuffers(renderer);
		return;
	}

	constexpr std::array<double, entityMeshVertexCount * positionComponentsPerVertex> entityVertices =
	{
		0.0, 1.0, -0.50,
		0.0, 0.0, -0.50,
		0.0, 0.0, 0.50,
		0.0, 1.0, 0.50
	};

	constexpr std::array<double, entityMeshVertexCount * normalComponentsPerVertex> dummyEntityNormals =
	{
		0.0, 0.0, 0.0,
		0.0, 0.0, 0.0,
		0.0, 0.0, 0.0,
		0.0, 0.0, 0.0
	};

	constexpr std::array<double, entityMeshVertexCount * texCoordComponentsPerVertex> entityTexCoords =
	{
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0
	};

	constexpr std::array<int32_t, entityMeshIndexCount> entityIndices =
	{
		0, 1, 2,
		2, 3, 0
	};

	const BufferView<const double> entityVerticesView(entityVertices.data(), static_cast<int>(entityVertices.size()));
	const BufferView<const double> entityNormalsView(dummyEntityNormals.data(), static_cast<int>(dummyEntityNormals.size()));
	const BufferView<const double> entityTexCoordsView(entityTexCoords.data(), static_cast<int>(entityTexCoords.size()));
	const BufferView<const int32_t> entityIndicesView(entityIndices.data(), static_cast<int>(entityIndices.size()));
	renderer.populateVertexBuffer(this->entityMeshDef.vertexBufferID, entityVerticesView);
	renderer.populateAttributeBuffer(this->entityMeshDef.normalBufferID, entityNormalsView);
	renderer.populateAttributeBuffer(this->entityMeshDef.texCoordBufferID, entityTexCoordsView);
	renderer.populateIndexBuffer(this->entityMeshDef.indexBufferID, entityIndicesView);
}

void RenderChunkManager::shutdown(Renderer &renderer)
{
	for (int i = static_cast<int>(this->activeChunks.size()) - 1; i >= 0; i--)
	{
		ChunkPtr &chunkPtr = this->activeChunks[i];
		chunkPtr->freeBuffers(renderer);
		this->recycleChunk(i);
	}

	for (IndexBufferID &indexBufferID : this->chasmWallIndexBufferIDs)
	{
		renderer.freeIndexBuffer(indexBufferID);
		indexBufferID = -1;
	}

	this->voxelTextures.clear();
	this->chasmFloorTextureLists.clear();
	this->chasmTextureKeys.clear();
	this->entityAnims.clear();
	this->entityMeshDef.freeBuffers(renderer);
	this->voxelDrawCallsCache.clear();
	this->entityDrawCallsCache.clear();
	this->totalDrawCallsCache.clear();
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

ObjectTextureID RenderChunkManager::getEntityTextureID(EntityInstanceID entityInstID, const CoordDouble2 &cameraCoordXZ,
	const EntityChunkManager &entityChunkManager, const EntityDefinitionLibrary &entityDefLibrary) const
{
	const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
	const EntityDefID entityDefID = entityInst.defID;
	const auto defIter = std::find_if(this->entityAnims.begin(), this->entityAnims.end(),
		[entityDefID](const LoadedEntityAnimation &loadedAnim)
	{
		return loadedAnim.defID == entityDefID;
	});

	DebugAssertMsg(defIter != this->entityAnims.end(), "Expected loaded entity animation for def ID " + std::to_string(entityDefID) + ".");

	EntityVisibilityState2D visState;
	entityChunkManager.getEntityVisibilityState2D(entityInstID, cameraCoordXZ, entityDefLibrary, visState);

	const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityDefID, entityDefLibrary);
	const EntityAnimationDefinition &animDef = entityDef.getAnimDef();
	const int linearizedKeyframeIndex = animDef.getLinearizedKeyframeIndex(visState.stateIndex, visState.angleIndex, visState.keyframeIndex);
	const Buffer<ScopedObjectTextureRef> &textureRefs = defIter->textureRefs;
	return textureRefs.get(linearizedKeyframeIndex).get();
}

BufferView<const RenderDrawCall> RenderChunkManager::getVoxelDrawCalls() const
{
	return BufferView<const RenderDrawCall>(this->voxelDrawCallsCache.data(), static_cast<int>(this->voxelDrawCallsCache.size()));
}

BufferView<const RenderDrawCall> RenderChunkManager::getEntityDrawCalls() const
{
	return BufferView<const RenderDrawCall>(this->entityDrawCallsCache.data(), static_cast<int>(this->entityDrawCallsCache.size()));
}

BufferView<const RenderDrawCall> RenderChunkManager::getTotalDrawCalls() const
{
	return BufferView<const RenderDrawCall>(this->totalDrawCallsCache.data(), static_cast<int>(this->totalDrawCallsCache.size()));
}

void RenderChunkManager::loadVoxelTextures(const VoxelChunk &voxelChunk, TextureManager &textureManager, Renderer &renderer)
{
	for (int i = 0; i < voxelChunk.getTextureDefCount(); i++)
	{
		const VoxelTextureDefinition &voxelTextureDef = voxelChunk.getTextureDef(i);
		sgTexture::LoadVoxelDefTextures(voxelTextureDef, this->voxelTextures, textureManager, renderer);
	}

	for (int i = 0; i < voxelChunk.getChasmDefCount(); i++)
	{
		const VoxelChunk::ChasmDefID chasmDefID = static_cast<VoxelChunk::ChasmDefID>(i);
		sgTexture::LoadChasmDefTextures(chasmDefID, voxelChunk, this->voxelTextures, this->chasmFloorTextureLists,
			this->chasmTextureKeys, textureManager, renderer);
	}
}

void RenderChunkManager::loadVoxelMeshBuffers(RenderChunk &renderChunk, const VoxelChunk &voxelChunk, double ceilingScale, Renderer &renderer)
{
	const ChunkInt2 &chunkPos = voxelChunk.getPosition();

	// Add render chunk voxel mesh instances and create mappings to them.
	for (int meshDefIndex = 0; meshDefIndex < voxelChunk.getMeshDefCount(); meshDefIndex++)
	{
		const VoxelChunk::VoxelMeshDefID voxelMeshDefID = static_cast<VoxelChunk::VoxelMeshDefID>(meshDefIndex);
		const VoxelMeshDefinition &voxelMeshDef = voxelChunk.getMeshDef(voxelMeshDefID);

		RenderVoxelMeshDefinition renderVoxelMeshDef;
		if (!voxelMeshDef.isEmpty()) // Only attempt to create buffers for non-air voxels.
		{
			constexpr int positionComponentsPerVertex = MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
			constexpr int normalComponentsPerVertex = MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
			constexpr int texCoordComponentsPerVertex = MeshUtils::TEX_COORDS_PER_VERTEX;

			const int vertexCount = voxelMeshDef.rendererVertexCount;
			if (!renderer.tryCreateVertexBuffer(vertexCount, positionComponentsPerVertex, &renderVoxelMeshDef.vertexBufferID))
			{
				DebugLogError("Couldn't create vertex buffer for voxel mesh ID " + std::to_string(voxelMeshDefID) +
					" in chunk (" + voxelChunk.getPosition().toString() + ").");
				continue;
			}

			if (!renderer.tryCreateAttributeBuffer(vertexCount, normalComponentsPerVertex, &renderVoxelMeshDef.normalBufferID))
			{
				DebugLogError("Couldn't create normal attribute buffer for voxel mesh ID " + std::to_string(voxelMeshDefID) +
					" in chunk (" + voxelChunk.getPosition().toString() + ").");
				renderVoxelMeshDef.freeBuffers(renderer);
				continue;
			}

			if (!renderer.tryCreateAttributeBuffer(vertexCount, texCoordComponentsPerVertex, &renderVoxelMeshDef.texCoordBufferID))
			{
				DebugLogError("Couldn't create tex coord attribute buffer for voxel mesh ID " + std::to_string(voxelMeshDefID) +
					" in chunk (" + voxelChunk.getPosition().toString() + ").");
				renderVoxelMeshDef.freeBuffers(renderer);
				continue;
			}

			ArenaMeshUtils::RenderMeshInitCache meshInitCache;

			// Generate mesh geometry and indices for this voxel definition.
			voxelMeshDef.writeRendererGeometryBuffers(ceilingScale, meshInitCache.verticesView, meshInitCache.normalsView, meshInitCache.texCoordsView);
			voxelMeshDef.writeRendererIndexBuffers(meshInitCache.opaqueIndices0View, meshInitCache.opaqueIndices1View,
				meshInitCache.opaqueIndices2View, meshInitCache.alphaTestedIndices0View);

			renderer.populateVertexBuffer(renderVoxelMeshDef.vertexBufferID,
				BufferView<const double>(meshInitCache.vertices.data(), vertexCount * positionComponentsPerVertex));
			renderer.populateAttributeBuffer(renderVoxelMeshDef.normalBufferID,
				BufferView<const double>(meshInitCache.normals.data(), vertexCount * normalComponentsPerVertex));
			renderer.populateAttributeBuffer(renderVoxelMeshDef.texCoordBufferID,
				BufferView<const double>(meshInitCache.texCoords.data(), vertexCount * texCoordComponentsPerVertex));

			const int opaqueIndexBufferCount = voxelMeshDef.opaqueIndicesListCount;
			for (int bufferIndex = 0; bufferIndex < opaqueIndexBufferCount; bufferIndex++)
			{
				const int opaqueIndexCount = static_cast<int>(voxelMeshDef.getOpaqueIndicesList(bufferIndex).size());
				IndexBufferID &opaqueIndexBufferID = renderVoxelMeshDef.opaqueIndexBufferIDs[bufferIndex];
				if (!renderer.tryCreateIndexBuffer(opaqueIndexCount, &opaqueIndexBufferID))
				{
					DebugLogError("Couldn't create opaque index buffer for voxel mesh ID " +
						std::to_string(voxelMeshDefID) + " in chunk (" + voxelChunk.getPosition().toString() + ").");
					renderVoxelMeshDef.freeBuffers(renderer);
					continue;
				}

				renderVoxelMeshDef.opaqueIndexBufferIdCount++;

				const auto &indices = *meshInitCache.opaqueIndicesPtrs[bufferIndex];
				renderer.populateIndexBuffer(opaqueIndexBufferID,
					BufferView<const int32_t>(indices.data(), opaqueIndexCount));
			}

			const bool hasAlphaTestedIndexBuffer = voxelMeshDef.alphaTestedIndicesListCount > 0;
			if (hasAlphaTestedIndexBuffer)
			{
				const int alphaTestedIndexCount = static_cast<int>(voxelMeshDef.alphaTestedIndices.size());
				if (!renderer.tryCreateIndexBuffer(alphaTestedIndexCount, &renderVoxelMeshDef.alphaTestedIndexBufferID))
				{
					DebugLogError("Couldn't create alpha-tested index buffer for voxel mesh ID " +
						std::to_string(voxelMeshDefID) + " in chunk (" + voxelChunk.getPosition().toString() + ").");
					renderVoxelMeshDef.freeBuffers(renderer);
					continue;
				}

				renderer.populateIndexBuffer(renderVoxelMeshDef.alphaTestedIndexBufferID,
					BufferView<const int32_t>(meshInitCache.alphaTestedIndices0.data(), alphaTestedIndexCount));
			}
		}

		const RenderVoxelMeshDefID renderMeshDefID = renderChunk.addMeshDefinition(std::move(renderVoxelMeshDef));
		renderChunk.meshDefMappings.emplace(voxelMeshDefID, renderMeshDefID);
	}
}

void RenderChunkManager::loadVoxelChasmWalls(RenderChunk &renderChunk, const VoxelChunk &voxelChunk)
{
	DebugAssert(renderChunk.chasmWallIndexBufferIDs.empty());

	for (WEInt z = 0; z < Chunk::DEPTH; z++)
	{
		for (int y = 0; y < voxelChunk.getHeight(); y++)
		{
			for (SNInt x = 0; x < Chunk::WIDTH; x++)
			{
				int chasmWallInstIndex;
				if (!voxelChunk.tryGetChasmWallInstIndex(x, y, z, &chasmWallInstIndex))
				{
					continue;
				}

				const VoxelChasmWallInstance &chasmWallInst = voxelChunk.getChasmWallInst(chasmWallInstIndex);
				DebugAssert(chasmWallInst.getFaceCount() > 0);

				const int chasmWallIndexBufferIndex = ArenaMeshUtils::GetChasmWallIndex(
					chasmWallInst.north, chasmWallInst.east, chasmWallInst.south, chasmWallInst.west);
				const IndexBufferID indexBufferID = this->chasmWallIndexBufferIDs[chasmWallIndexBufferIndex];

				renderChunk.chasmWallIndexBufferIDs.emplace(VoxelInt3(x, y, z), indexBufferID);
			}
		}
	}
}

void RenderChunkManager::loadEntityTextures(const EntityChunk &entityChunk, const EntityChunkManager &entityChunkManager,
	const EntityDefinitionLibrary &entityDefLibrary, TextureManager &textureManager, Renderer &renderer)
{
	for (const EntityInstanceID entityInstID : entityChunk.entityIDs)
	{
		const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
		const EntityDefID entityDefID = entityInst.defID;

		const auto animIter = std::find_if(this->entityAnims.begin(), this->entityAnims.end(),
			[entityDefID](const LoadedEntityAnimation &loadedAnim)
		{
			return loadedAnim.defID == entityDefID;
		});

		if (animIter != this->entityAnims.end())
		{
			continue;
		}

		const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityDefID, entityDefLibrary);
		const EntityAnimationDefinition &animDef = entityDef.getAnimDef();
		Buffer<ScopedObjectTextureRef> textureRefs = sgTexture::MakeEntityAnimationTextures(animDef, textureManager, renderer);

		LoadedEntityAnimation loadedEntityAnim;
		loadedEntityAnim.init(entityDefID, std::move(textureRefs));
		this->entityAnims.emplace_back(std::move(loadedEntityAnim));
	}
}

void RenderChunkManager::addVoxelDrawCall(const Double3 &position, const Double3 &preScaleTranslation, const Matrix4d &rotationMatrix,
	const Matrix4d &scaleMatrix, VertexBufferID vertexBufferID, AttributeBufferID normalBufferID, AttributeBufferID texCoordBufferID,
	IndexBufferID indexBufferID, ObjectTextureID textureID0, const std::optional<ObjectTextureID> &textureID1,
	TextureSamplingType textureSamplingType, VertexShaderType vertexShaderType, PixelShaderType pixelShaderType,
	double pixelShaderParam0, std::vector<RenderDrawCall> &drawCalls)
{
	RenderDrawCall drawCall;
	drawCall.position = position;
	drawCall.preScaleTranslation = preScaleTranslation;
	drawCall.rotation = rotationMatrix;
	drawCall.scale = scaleMatrix;
	drawCall.vertexBufferID = vertexBufferID;
	drawCall.normalBufferID = normalBufferID;
	drawCall.texCoordBufferID = texCoordBufferID;
	drawCall.indexBufferID = indexBufferID;
	drawCall.textureIDs[0] = textureID0;
	drawCall.textureIDs[1] = textureID1;
	drawCall.textureSamplingType = textureSamplingType;
	drawCall.vertexShaderType = vertexShaderType;
	drawCall.pixelShaderType = pixelShaderType;
	drawCall.pixelShaderParam0 = pixelShaderParam0;

	drawCalls.emplace_back(std::move(drawCall));
}

void RenderChunkManager::loadVoxelDrawCalls(RenderChunk &renderChunk, const VoxelChunk &voxelChunk, double ceilingScale,
	double chasmAnimPercent, bool updateStatics, bool updateAnimating)
{
	const ChunkInt2 &chunkPos = renderChunk.getPosition();

	// Generate draw calls for each non-air voxel.
	for (WEInt z = 0; z < renderChunk.meshDefIDs.getDepth(); z++)
	{
		for (int y = 0; y < renderChunk.meshDefIDs.getHeight(); y++)
		{
			for (SNInt x = 0; x < renderChunk.meshDefIDs.getWidth(); x++)
			{
				const VoxelInt3 voxel(x, y, z);
				const VoxelChunk::VoxelMeshDefID voxelMeshDefID = voxelChunk.getMeshDefID(x, y, z);
				const VoxelMeshDefinition &voxelMeshDef = voxelChunk.getMeshDef(voxelMeshDefID);
				if (voxelMeshDef.isEmpty())
				{
					continue;
				}

				const VoxelChunk::VoxelTextureDefID voxelTextureDefID = voxelChunk.getTextureDefID(x, y, z);
				const VoxelChunk::VoxelTraitsDefID voxelTraitsDefID = voxelChunk.getTraitsDefID(x, y, z);
				const VoxelTextureDefinition &voxelTextureDef = voxelChunk.getTextureDef(voxelTextureDefID);
				const VoxelTraitsDefinition &voxelTraitsDef = voxelChunk.getTraitsDef(voxelTraitsDefID);

				const auto defIter = renderChunk.meshDefMappings.find(voxelMeshDefID);
				DebugAssert(defIter != renderChunk.meshDefMappings.end());
				const RenderVoxelMeshDefID renderMeshDefID = defIter->second;
				renderChunk.meshDefIDs.set(x, y, z, renderMeshDefID);

				const RenderVoxelMeshDefinition &renderMeshDef = renderChunk.meshDefs[renderMeshDefID];

				// Convert voxel XYZ to world space.
				const WorldInt2 worldXZ = VoxelUtils::chunkVoxelToWorldVoxel(chunkPos, VoxelInt2(x, z));
				const int worldY = y;
				const Double3 worldPos(
					static_cast<SNDouble>(worldXZ.x),
					static_cast<double>(worldY) * ceilingScale,
					static_cast<WEDouble>(worldXZ.y));

				const ArenaTypes::VoxelType voxelType = voxelTraitsDef.type;

				VoxelChunk::DoorDefID doorDefID;
				const bool isDoor = voxelChunk.tryGetDoorDefID(x, y, z, &doorDefID);

				VoxelChunk::ChasmDefID chasmDefID;
				const bool isChasm = voxelChunk.tryGetChasmDefID(x, y, z, &chasmDefID);

				int fadeAnimInstIndex;
				const VoxelFadeAnimationInstance *fadeAnimInst = nullptr;
				const bool isFading = voxelChunk.tryGetFadeAnimInstIndex(x, y, z, &fadeAnimInstIndex);
				if (isFading)
				{
					fadeAnimInst = &voxelChunk.getFadeAnimInst(fadeAnimInstIndex);
				}

				const bool canAnimate = isDoor || isChasm || isFading;
				if ((!canAnimate && updateStatics) || (canAnimate && updateAnimating))
				{
					for (int bufferIndex = 0; bufferIndex < renderMeshDef.opaqueIndexBufferIdCount; bufferIndex++)
					{
						ObjectTextureID textureID = -1;

						if (!isChasm)
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

						const IndexBufferID opaqueIndexBufferID = renderMeshDef.opaqueIndexBufferIDs[bufferIndex];
						const Double3 preScaleTranslation = Double3::Zero;
						const Matrix4d rotationMatrix = Matrix4d::identity();
						const Matrix4d scaleMatrix = Matrix4d::identity();
						const TextureSamplingType textureSamplingType = !isChasm ? TextureSamplingType::Default : TextureSamplingType::ScreenSpaceRepeatY;

						PixelShaderType pixelShaderType = PixelShaderType::Opaque;
						double pixelShaderParam0 = 0.0;
						if (isFading)
						{
							pixelShaderType = PixelShaderType::OpaqueWithFade;
							pixelShaderParam0 = fadeAnimInst->percentFaded;
						}

						std::vector<RenderDrawCall> *drawCallsPtr = nullptr;
						if (isChasm)
						{
							drawCallsPtr = &renderChunk.chasmDrawCalls;
						}
						else if (isFading)
						{
							drawCallsPtr = &renderChunk.fadingDrawCalls;
						}
						else
						{
							drawCallsPtr = &renderChunk.staticDrawCalls;
						}

						this->addVoxelDrawCall(worldPos, preScaleTranslation, rotationMatrix, scaleMatrix, renderMeshDef.vertexBufferID,
							renderMeshDef.normalBufferID, renderMeshDef.texCoordBufferID, opaqueIndexBufferID, textureID, std::nullopt,
							textureSamplingType, VertexShaderType::Voxel, pixelShaderType, pixelShaderParam0, *drawCallsPtr);
					}
				}

				if (renderMeshDef.alphaTestedIndexBufferID >= 0)
				{
					if (updateStatics || (updateAnimating && isDoor))
					{
						DebugAssert(!isChasm);
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

						if (isDoor)
						{
							double doorAnimPercent = 0.0;
							int doorAnimInstIndex;
							if (voxelChunk.tryGetDoorAnimInstIndex(x, y, z, &doorAnimInstIndex))
							{
								const VoxelDoorAnimationInstance &doorAnimInst = voxelChunk.getDoorAnimInst(doorAnimInstIndex);
								doorAnimPercent = doorAnimInst.percentOpen;
							}

							int doorVisInstIndex;
							if (!voxelChunk.tryGetDoorVisibilityInstIndex(x, y, z, &doorVisInstIndex))
							{
								DebugLogError("Expected door visibility instance at (" + std::to_string(x) + ", " +
									std::to_string(y) + ", " + std::to_string(z) + ") in chunk (" + chunkPos.toString() + ").");
								continue;
							}

							const VoxelDoorVisibilityInstance &doorVisInst = voxelChunk.getDoorVisibilityInst(doorVisInstIndex);
							bool visibleDoorFaces[DoorUtils::FACE_COUNT];
							std::fill(std::begin(visibleDoorFaces), std::end(visibleDoorFaces), false);

							for (size_t i = 0; i < std::size(visibleDoorFaces); i++)
							{
								const VoxelFacing2D doorFacing = DoorUtils::Facings[i];
								bool &canRenderFace = visibleDoorFaces[i];
								for (int j = 0; j < doorVisInst.visibleFaceCount; j++)
								{
									if (doorVisInst.visibleFaces[j] == doorFacing)
									{
										canRenderFace = true;
										break;
									}
								}
							}

							DebugAssert(std::count(std::begin(visibleDoorFaces), std::end(visibleDoorFaces), true) <= VoxelDoorVisibilityInstance::MAX_FACE_COUNT);

							// Get the door type and generate draw calls. One draw call for each door face since
							// they have independent transforms.
							const DoorDefinition &doorDef = voxelChunk.getDoorDef(doorDefID);
							const ArenaTypes::DoorType doorType = doorDef.getType();
							switch (doorType)
							{
							case ArenaTypes::DoorType::Swinging:
							{
								const Radians rotationAmount = -(Constants::HalfPi - Constants::Epsilon) * doorAnimPercent;

								for (int i = 0; i < DoorUtils::FACE_COUNT; i++)
								{
									if (!visibleDoorFaces[i])
									{
										continue;
									}

									const Double3 &doorHingeOffset = DoorUtils::SwingingHingeOffsets[i];
									const Double3 doorHingePosition = worldPos + doorHingeOffset;
									const Radians doorBaseAngle = DoorUtils::BaseAngles[i];
									const Double3 doorPreScaleTranslation = Double3::Zero;
									const Matrix4d doorRotationMatrix = Matrix4d::yRotation(doorBaseAngle + rotationAmount);
									const Matrix4d doorScaleMatrix = Matrix4d::identity();
									constexpr double pixelShaderParam0 = 0.0;
									this->addVoxelDrawCall(doorHingePosition, doorPreScaleTranslation, doorRotationMatrix, doorScaleMatrix,
										renderMeshDef.vertexBufferID, renderMeshDef.normalBufferID, renderMeshDef.texCoordBufferID,
										renderMeshDef.alphaTestedIndexBufferID, textureID, std::nullopt, TextureSamplingType::Default,
										VertexShaderType::SwingingDoor, PixelShaderType::AlphaTested, pixelShaderParam0, renderChunk.doorDrawCalls);
								}

								break;
							}
							case ArenaTypes::DoorType::Sliding:
							{
								const double uMin = (1.0 - ArenaRenderUtils::DOOR_MIN_VISIBLE) * doorAnimPercent;
								const double scaleAmount = 1.0 - uMin;

								for (int i = 0; i < DoorUtils::FACE_COUNT; i++)
								{
									if (!visibleDoorFaces[i])
									{
										continue;
									}

									const Double3 &doorHingeOffset = DoorUtils::SwingingHingeOffsets[i];
									const Double3 doorHingePosition = worldPos + doorHingeOffset;
									const Radians doorBaseAngle = DoorUtils::BaseAngles[i];
									const Double3 doorPreScaleTranslation = Double3::Zero;
									const Matrix4d doorRotationMatrix = Matrix4d::yRotation(doorBaseAngle);
									const Matrix4d doorScaleMatrix = Matrix4d::scale(1.0, 1.0, scaleAmount);
									const double pixelShaderParam0 = uMin;
									this->addVoxelDrawCall(doorHingePosition, doorPreScaleTranslation, doorRotationMatrix, doorScaleMatrix,
										renderMeshDef.vertexBufferID, renderMeshDef.normalBufferID, renderMeshDef.texCoordBufferID,
										renderMeshDef.alphaTestedIndexBufferID, textureID, std::nullopt, TextureSamplingType::Default,
										VertexShaderType::SlidingDoor, PixelShaderType::AlphaTestedWithVariableTexCoordUMin, pixelShaderParam0,
										renderChunk.doorDrawCalls);
								}

								break;
							}
							case ArenaTypes::DoorType::Raising:
							{
								const double preScaleTranslationY = -ceilingScale;
								const double vMin = (1.0 - ArenaRenderUtils::DOOR_MIN_VISIBLE) * doorAnimPercent;
								const double scaleAmount = 1.0 - vMin;

								for (int i = 0; i < DoorUtils::FACE_COUNT; i++)
								{
									if (!visibleDoorFaces[i])
									{
										continue;
									}

									const Double3 &doorHingeOffset = DoorUtils::SwingingHingeOffsets[i];
									const Double3 doorHingePosition = worldPos + doorHingeOffset;
									const Radians doorBaseAngle = DoorUtils::BaseAngles[i];
									const Double3 doorPreScaleTranslation = Double3(1.0, preScaleTranslationY, 1.0);
									const Matrix4d doorRotationMatrix = Matrix4d::yRotation(doorBaseAngle);
									const Matrix4d doorScaleMatrix = Matrix4d::scale(1.0, scaleAmount, 1.0);
									const double pixelShaderParam0 = vMin;
									this->addVoxelDrawCall(doorHingePosition, doorPreScaleTranslation, doorRotationMatrix, doorScaleMatrix,
										renderMeshDef.vertexBufferID, renderMeshDef.normalBufferID, renderMeshDef.texCoordBufferID,
										renderMeshDef.alphaTestedIndexBufferID, textureID, std::nullopt, TextureSamplingType::Default,
										VertexShaderType::RaisingDoor, PixelShaderType::AlphaTestedWithVariableTexCoordVMin, pixelShaderParam0,
										renderChunk.doorDrawCalls);
								}

								break;
							}
							case ArenaTypes::DoorType::Splitting:
								DebugNotImplementedMsg("Splitting door draw calls");
								break;
							default:
								DebugNotImplementedMsg(std::to_string(static_cast<int>(doorType)));
								break;
							}
						}
						else
						{
							const Double3 preScaleTranslation = Double3::Zero;
							const Matrix4d rotationMatrix = Matrix4d::identity();
							const Matrix4d scaleMatrix = Matrix4d::identity();
							constexpr double pixelShaderParam0 = 0.0;
							this->addVoxelDrawCall(worldPos, preScaleTranslation, rotationMatrix, scaleMatrix, renderMeshDef.vertexBufferID,
								renderMeshDef.normalBufferID, renderMeshDef.texCoordBufferID, renderMeshDef.alphaTestedIndexBufferID,
								textureID, std::nullopt, TextureSamplingType::Default, VertexShaderType::Voxel, PixelShaderType::AlphaTested,
								pixelShaderParam0, renderChunk.staticDrawCalls);
						}
					}
				}

				if (isChasm)
				{
					const auto chasmWallIter = renderChunk.chasmWallIndexBufferIDs.find(voxel);
					if (chasmWallIter != renderChunk.chasmWallIndexBufferIDs.end())
					{
						DebugAssert(voxelTraitsDef.type == ArenaTypes::VoxelType::Chasm);
						const bool isAnimatingChasm = voxelTraitsDef.chasm.type != ArenaTypes::ChasmType::Dry;
						const IndexBufferID chasmWallIndexBufferID = chasmWallIter->second;

						// Need to give two textures since chasm walls are multi-textured.
						ObjectTextureID textureID0 = this->getChasmFloorTextureID(chunkPos, chasmDefID, chasmAnimPercent);
						ObjectTextureID textureID1 = this->getChasmWallTextureID(chunkPos, chasmDefID);

						const Double3 preScaleTranslation = Double3::Zero;
						const Matrix4d rotationMatrix = Matrix4d::identity();
						const Matrix4d scaleMatrix = Matrix4d::identity();
						const TextureSamplingType textureSamplingType = isAnimatingChasm ? TextureSamplingType::ScreenSpaceRepeatY : TextureSamplingType::Default;
						constexpr double pixelShaderParam0 = 0.0;
						this->addVoxelDrawCall(worldPos, preScaleTranslation, rotationMatrix, scaleMatrix, renderMeshDef.vertexBufferID,
							renderMeshDef.normalBufferID, renderMeshDef.texCoordBufferID, chasmWallIndexBufferID, textureID0, textureID1,
							textureSamplingType, VertexShaderType::Voxel, PixelShaderType::OpaqueWithAlphaTestLayer,
							pixelShaderParam0, renderChunk.chasmDrawCalls);
					}
				}
			}
		}
	}
}

void RenderChunkManager::rebuildVoxelChunkDrawCalls(RenderChunk &renderChunk, const VoxelChunk &voxelChunk,
	double ceilingScale, double chasmAnimPercent, bool updateStatics, bool updateAnimating)
{
	if (updateStatics)
	{
		renderChunk.staticDrawCalls.clear();
	}

	if (updateAnimating)
	{
		renderChunk.doorDrawCalls.clear();
		renderChunk.chasmDrawCalls.clear();
		renderChunk.fadingDrawCalls.clear();
	}

	this->loadVoxelDrawCalls(renderChunk, voxelChunk, ceilingScale, chasmAnimPercent, updateStatics, updateAnimating);
}

void RenderChunkManager::rebuildVoxelDrawCallsList()
{
	this->voxelDrawCallsCache.clear();

	// @todo: eventually this should sort by distance from a CoordDouble2
	for (size_t i = 0; i < this->activeChunks.size(); i++)
	{
		const ChunkPtr &chunkPtr = this->activeChunks[i];
		const std::vector<RenderDrawCall> &staticDrawCalls = chunkPtr->staticDrawCalls;
		const std::vector<RenderDrawCall> &doorDrawCalls = chunkPtr->doorDrawCalls;
		const std::vector<RenderDrawCall> &chasmDrawCalls = chunkPtr->chasmDrawCalls;
		const std::vector<RenderDrawCall> &fadingDrawCalls = chunkPtr->fadingDrawCalls;
		this->voxelDrawCallsCache.insert(this->voxelDrawCallsCache.end(), staticDrawCalls.begin(), staticDrawCalls.end());
		this->voxelDrawCallsCache.insert(this->voxelDrawCallsCache.end(), doorDrawCalls.begin(), doorDrawCalls.end());
		this->voxelDrawCallsCache.insert(this->voxelDrawCallsCache.end(), chasmDrawCalls.begin(), chasmDrawCalls.end());
		this->voxelDrawCallsCache.insert(this->voxelDrawCallsCache.end(), fadingDrawCalls.begin(), fadingDrawCalls.end());
	}
}

void RenderChunkManager::addEntityDrawCall(const Double3 &position, const Matrix4d &rotationMatrix, ObjectTextureID textureID0,
	double width, double height, PixelShaderType pixelShaderType, double pixelShaderParam0, std::vector<RenderDrawCall> &drawCalls)
{
	RenderDrawCall drawCall;
	drawCall.position = position;
	drawCall.preScaleTranslation = Double3::Zero;
	drawCall.rotation = rotationMatrix;
	drawCall.scale = Matrix4d::identity();
	drawCall.vertexBufferID = this->entityMeshDef.vertexBufferID;
	drawCall.normalBufferID = this->entityMeshDef.normalBufferID;
	drawCall.texCoordBufferID = this->entityMeshDef.texCoordBufferID;
	drawCall.indexBufferID = this->entityMeshDef.indexBufferID;
	drawCall.textureIDs[0] = textureID0;
	drawCall.textureIDs[1] = std::nullopt;
	drawCall.textureSamplingType = TextureSamplingType::Default;
	drawCall.vertexShaderType = VertexShaderType::Entity;
	// @todo: vertex shader params
	drawCall.pixelShaderType = pixelShaderType;
	drawCall.pixelShaderParam0 = pixelShaderParam0;

	drawCalls.emplace_back(std::move(drawCall));
}

void RenderChunkManager::rebuildEntityChunkDrawCalls(RenderChunk &renderChunk, const EntityChunk &entityChunk,
	const CoordDouble2 &cameraCoordXZ, double ceilingScale, const EntityChunkManager &entityChunkManager,
	const EntityDefinitionLibrary &entityDefLibrary)
{
	renderChunk.entityDrawCalls.clear();

	const std::vector<EntityInstanceID> &entityIDs = entityChunk.entityIDs;
	const int entityCount = static_cast<int>(entityIDs.size());
	for (int i = 0; i < entityCount; i++)
	{
		const EntityInstanceID entityInstID = entityIDs[i];
		const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
		const CoordDouble2 &entityCoord = entityChunkManager.getEntityPosition(entityInst.positionID);

		// @todo: get the correct anim frame, vis state 3D, call addEntityDrawCall(), etc. etc.

		// Convert entity XYZ to world space.
		const WorldDouble2 worldXZ = VoxelUtils::coordToWorldPoint(entityCoord);
		const double worldY = ceilingScale;
		const Double3 worldPos(
			static_cast<SNDouble>(worldXZ.x),
			static_cast<double>(worldY),
			static_cast<WEDouble>(worldXZ.y));

		const Matrix4d rotationMatrix = Matrix4d::yRotation(0.0);
		const ObjectTextureID textureID = this->getEntityTextureID(entityInstID, cameraCoordXZ, entityChunkManager, entityDefLibrary);
		const double width = 1.0; // @todo: get from entity def? EntityUtils?
		const double height = 1.0;
		const double pixelShaderParam0 = 0.0;
		this->addEntityDrawCall(worldPos, rotationMatrix, textureID, width, height, PixelShaderType::AlphaTested,
			pixelShaderParam0, renderChunk.entityDrawCalls);
	}
}

void RenderChunkManager::rebuildEntityDrawCallsList()
{
	this->entityDrawCallsCache.clear();

	// @todo: eventually this should sort by distance from a CoordDouble2
	for (size_t i = 0; i < this->activeChunks.size(); i++)
	{
		const ChunkPtr &chunkPtr = this->activeChunks[i];
		const std::vector<RenderDrawCall> &entityDrawCalls = chunkPtr->entityDrawCalls;
		this->entityDrawCallsCache.insert(this->entityDrawCallsCache.end(), entityDrawCalls.begin(), entityDrawCalls.end());
	}
}

void RenderChunkManager::updateActiveChunks(const BufferView<const ChunkInt2> &activeChunkPositions,
	const BufferView<const ChunkInt2> &newChunkPositions, const BufferView<const ChunkInt2> &freedChunkPositions,
	const VoxelChunkManager &voxelChunkManager, Renderer &renderer)
{
	for (int i = 0; i < freedChunkPositions.getCount(); i++)
	{
		const ChunkInt2 &chunkPos = freedChunkPositions.get(i);
		const int chunkIndex = this->getChunkIndex(chunkPos);
		RenderChunk &renderChunk = this->getChunkAtIndex(chunkIndex);
		renderChunk.freeBuffers(renderer);
		this->recycleChunk(chunkIndex);
	}

	for (int i = 0; i < newChunkPositions.getCount(); i++)
	{
		const ChunkInt2 &chunkPos = newChunkPositions.get(i);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);

		const int spawnIndex = this->spawnChunk();
		RenderChunk &renderChunk = this->getChunkAtIndex(spawnIndex);
		renderChunk.init(chunkPos, voxelChunk.getHeight());
	}

	// Free any unneeded chunks for memory savings in case the chunk distance was once large
	// and is now small. This is significant even for chunk distance 2->1, or 25->9 chunks.
	this->chunkPool.clear();
}

void RenderChunkManager::updateVoxels(const BufferView<const ChunkInt2> &activeChunkPositions,
	const BufferView<const ChunkInt2> &newChunkPositions, double ceilingScale, double chasmAnimPercent,
	const VoxelChunkManager &voxelChunkManager, TextureManager &textureManager, Renderer &renderer)
{
	for (int i = 0; i < newChunkPositions.getCount(); i++)
	{
		const ChunkInt2 &chunkPos = newChunkPositions.get(i);
		RenderChunk &renderChunk = this->getChunkAtPosition(chunkPos);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		this->loadVoxelTextures(voxelChunk, textureManager, renderer);
		this->loadVoxelMeshBuffers(renderChunk, voxelChunk, ceilingScale, renderer);
		this->loadVoxelChasmWalls(renderChunk, voxelChunk);
		this->rebuildVoxelChunkDrawCalls(renderChunk, voxelChunk, ceilingScale, chasmAnimPercent, true, false);
	}

	for (int i = 0; i < activeChunkPositions.getCount(); i++)
	{
		const ChunkInt2 &chunkPos = activeChunkPositions.get(i);
		RenderChunk &renderChunk = this->getChunkAtPosition(chunkPos);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		const bool updateStatics = (voxelChunk.getDirtyMeshDefPositionCount() > 0) || (voxelChunk.getDirtyFadeAnimInstPositionCount() > 0); // @temp fix for fading voxels being covered by their non-fading draw call
		this->rebuildVoxelChunkDrawCalls(renderChunk, voxelChunk, ceilingScale, chasmAnimPercent, updateStatics, true);
	}

	// @todo: only rebuild if needed; currently we assume that all scenes in the game have some kind of animating chasms/etc., which is inefficient
	//if ((freedChunkCount > 0) || (newChunkCount > 0))
	{
		this->rebuildVoxelDrawCallsList();
	}
}

void RenderChunkManager::updateEntities(const BufferView<const ChunkInt2> &activeChunkPositions,
	const BufferView<const ChunkInt2> &newChunkPositions, const CoordDouble2 &cameraCoordXZ, const VoxelDouble2 &cameraDirXZ,
	double ceilingScale, const EntityChunkManager &entityChunkManager, const EntityDefinitionLibrary &entityDefLibrary,
	TextureManager &textureManager, Renderer &renderer)
{
	for (int i = 0; i < newChunkPositions.getCount(); i++)
	{
		const ChunkInt2 &chunkPos = newChunkPositions.get(i);
		RenderChunk &renderChunk = this->getChunkAtPosition(chunkPos);
		const EntityChunk &entityChunk = entityChunkManager.getChunkAtPosition(chunkPos);
		this->loadEntityTextures(entityChunk, entityChunkManager, entityDefLibrary, textureManager, renderer);
	}

	for (int i = 0; i < activeChunkPositions.getCount(); i++)
	{
		const ChunkInt2 &chunkPos = activeChunkPositions.get(i);
		RenderChunk &renderChunk = this->getChunkAtPosition(chunkPos);
		const EntityChunk &entityChunk = entityChunkManager.getChunkAtPosition(chunkPos);
		this->rebuildEntityChunkDrawCalls(renderChunk, entityChunk, cameraCoordXZ, ceilingScale, entityChunkManager, entityDefLibrary);
	}

	this->rebuildEntityDrawCallsList();

	// Update normals buffer.
	const VoxelDouble2 entityDir = -cameraDirXZ;
	constexpr int entityMeshVertexCount = 4;
	std::array<double, entityMeshVertexCount * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX> entityNormals =
	{
		entityDir.x, 0.0, entityDir.y,
		entityDir.x, 0.0, entityDir.y,
		entityDir.x, 0.0, entityDir.y,
		entityDir.x, 0.0, entityDir.y
	};

	const BufferView<const double> entityNormalsView(entityNormals.data(), static_cast<int>(entityNormals.size()));
	renderer.populateAttributeBuffer(this->entityMeshDef.normalBufferID, entityNormalsView);
	
	// @todo: move this some place better
	this->totalDrawCallsCache.clear();
	this->totalDrawCallsCache.insert(this->totalDrawCallsCache.end(), this->voxelDrawCallsCache.begin(), this->voxelDrawCallsCache.end());
	this->totalDrawCallsCache.insert(this->totalDrawCallsCache.end(), this->entityDrawCallsCache.begin(), this->entityDrawCallsCache.end());
}

void RenderChunkManager::unloadScene(Renderer &renderer)
{
	this->voxelTextures.clear();
	this->chasmFloorTextureLists.clear();
	this->chasmTextureKeys.clear();
	this->entityAnims.clear();

	// Free vertex/attribute/index buffer IDs.
	for (int i = static_cast<int>(this->activeChunks.size()) - 1; i >= 0; i--)
	{
		ChunkPtr &chunkPtr = this->activeChunks[i];
		chunkPtr->freeBuffers(renderer);
		this->recycleChunk(i);
	}

	this->voxelDrawCallsCache.clear();
	this->entityDrawCallsCache.clear();
	this->totalDrawCallsCache.clear();
}
