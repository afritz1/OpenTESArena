#include <algorithm>
#include <array>
#include <numeric>
#include <optional>

#include "ArenaRenderUtils.h"
#include "RenderCamera.h"
#include "RenderChunkManager.h"
#include "Renderer.h"
#include "RendererSystem3D.h"
#include "RendererUtils.h"
#include "../Assets/MIFUtils.h"
#include "../Assets/TextureManager.h"
#include "../Entities/EntityChunkManager.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Entities/EntityVisibilityState.h"
#include "../Math/Constants.h"
#include "../Math/Matrix4.h"
#include "../Voxels/DoorUtils.h"
#include "../Voxels/VoxelChunkManager.h"
#include "../Voxels/VoxelFacing2D.h"
#include "../World/ArenaMeshUtils.h"
#include "../World/ChunkManager.h"
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
		BufferView<const RenderChunkManager::LoadedVoxelTexture> voxelTextures,
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

				for (const TextureAsset &textureAsset : chasmDef.animated.textureAssets)
				{
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
		Buffer<ScopedObjectTextureRef> textureRefs(animDef.keyframeCount);

		// Need to go by state + keyframe list because the keyframes don't know whether they're flipped.
		int writeIndex = 0;
		for (int i = 0; i < animDef.stateCount; i++)
		{
			const EntityAnimationDefinitionState &defState = animDef.states[i];
			for (int j = 0; j < defState.keyframeListCount; j++)
			{
				const int keyframeListIndex = defState.keyframeListsIndex + j;
				DebugAssertIndex(animDef.keyframeLists, keyframeListIndex);
				const EntityAnimationDefinitionKeyframeList &keyframeList = animDef.keyframeLists[keyframeListIndex];
				for (int k = 0; k < keyframeList.keyframeCount; k++)
				{
					const int keyframeIndex = keyframeList.keyframesIndex + k;
					DebugAssertIndex(animDef.keyframes, keyframeIndex);
					const EntityAnimationDefinitionKeyframe &keyframe = animDef.keyframes[keyframeIndex];
					const TextureAsset &textureAsset = keyframe.textureAsset;

					const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAsset);
					if (!textureBuilderID.has_value())
					{
						DebugLogWarning("Couldn't load entity anim texture \"" + textureAsset.filename + "\".");
						continue;
					}

					const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
					const int textureWidth = textureBuilder.getWidth();
					const int textureHeight = textureBuilder.getHeight();
					const int texelCount = textureWidth * textureHeight;
					constexpr int bytesPerTexel = 1;
					DebugAssert(textureBuilder.getType() == TextureBuilderType::Paletted);

					ObjectTextureID textureID;
					if (!renderer.tryCreateObjectTexture(textureWidth, textureHeight, bytesPerTexel, &textureID))
					{
						DebugLogWarning("Couldn't create entity anim texture \"" + textureAsset.filename + "\".");
						continue;
					}

					ScopedObjectTextureRef textureRef(textureID, renderer);

					const TextureBuilder::PalettedTexture &palettedTexture = textureBuilder.getPaletted();
					const uint8_t *srcTexels = palettedTexture.texels.begin();

					LockedTexture lockedTexture = renderer.lockObjectTexture(textureID);
					uint8_t *dstTexels = static_cast<uint8_t*>(lockedTexture.texels);

					// Copy texels from source texture, mirroring if necessary.
					for (int y = 0; y < textureHeight; y++)
					{
						for (int x = 0; x < textureWidth; x++)
						{
							const int srcX = !keyframeList.isFlipped ? x : (textureWidth - 1 - x);
							const int srcIndex = srcX + (y * textureWidth);
							const int dstIndex = x + (y * textureWidth);
							dstTexels[dstIndex] = srcTexels[srcIndex];
						}
					}

					renderer.unlockObjectTexture(textureID);
					textureRefs.set(writeIndex, std::move(textureRef));
					writeIndex++;
				}
			}
		}

		DebugAssert(writeIndex == textureRefs.getCount());
		return textureRefs;
	}

	ScopedObjectTextureRef MakeEntityPaletteIndicesTextureRef(const PaletteIndices &paletteIndices, Renderer &renderer)
	{
		const int textureWidth = static_cast<int>(paletteIndices.size());
		constexpr int textureHeight = 1;
		constexpr int bytesPerTexel = 1;

		ObjectTextureID textureID;
		if (!renderer.tryCreateObjectTexture(textureWidth, textureHeight, bytesPerTexel, &textureID))
		{
			DebugCrash("Couldn't create entity palette indices texture.");
		}

		LockedTexture lockedTexture = renderer.lockObjectTexture(textureID);
		uint8_t *dstTexels = static_cast<uint8_t*>(lockedTexture.texels);
		std::copy(paletteIndices.begin(), paletteIndices.end(), dstTexels);
		renderer.unlockObjectTexture(textureID);
		return ScopedObjectTextureRef(textureID, renderer);
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

RenderChunkManager::Light::Light()
{
	this->lightID = -1;
	this->enabled = false;
}

void RenderChunkManager::Light::init(RenderLightID lightID, bool enabled)
{
	this->lightID = lightID;
	this->enabled = enabled;
}

RenderChunkManager::RenderChunkManager()
{
	this->chasmWallIndexBufferIDs.fill(-1);
	this->playerLightID = -1;
}

void RenderChunkManager::init(Renderer &renderer)
{
	// Populate chasm wall index buffers.
	ArenaMeshUtils::ChasmWallIndexBuffer northIndices, eastIndices, southIndices, westIndices;
	ArenaMeshUtils::WriteChasmWallRendererIndexBuffers(&northIndices, &eastIndices, &southIndices, &westIndices);
	constexpr int indicesPerFace = static_cast<int>(northIndices.size());

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

	renderer.populateVertexBuffer(this->entityMeshDef.vertexBufferID, entityVertices);
	renderer.populateAttributeBuffer(this->entityMeshDef.normalBufferID, dummyEntityNormals);
	renderer.populateAttributeBuffer(this->entityMeshDef.texCoordBufferID, entityTexCoords);
	renderer.populateIndexBuffer(this->entityMeshDef.indexBufferID, entityIndices);

	// Populate global lights.
	if (!renderer.tryCreateLight(&this->playerLightID))
	{
		DebugLogError("Couldn't create render light ID for player.");
		this->entityMeshDef.freeBuffers(renderer);
		return;
	}

	renderer.setLightRadius(this->playerLightID, ArenaRenderUtils::PLAYER_LIGHT_START_RADIUS, ArenaRenderUtils::PLAYER_LIGHT_END_RADIUS);
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

	if (this->playerLightID >= 0)
	{
		renderer.freeLight(this->playerLightID);
		this->playerLightID = -1;
	}

	for (auto &pair : this->entityLights)
	{
		Light &light = pair.second;
		if (light.lightID >= 0)
		{
			renderer.freeLight(light.lightID);
			light.lightID = -1;
		}
	}

	this->entityLights.clear();
	this->voxelTextures.clear();
	this->chasmFloorTextureLists.clear();
	this->chasmTextureKeys.clear();
	this->entityAnims.clear();
	this->entityMeshDef.freeBuffers(renderer);
	this->entityPaletteIndicesTextureRefs.clear();
	this->voxelDrawCallsCache.clear();
	this->entityDrawCallsCache.clear();
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
	BufferView<const ScopedObjectTextureRef> objectTextureRefs = textureList.objectTextureRefs;
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
	const EntityChunkManager &entityChunkManager) const
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
	entityChunkManager.getEntityVisibilityState2D(entityInstID, cameraCoordXZ, visState);

	const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityDefID);
	const EntityAnimationDefinition &animDef = entityDef.getAnimDef();
	const int linearizedKeyframeIndex = animDef.getLinearizedKeyframeIndex(visState.stateIndex, visState.angleIndex, visState.keyframeIndex);
	const Buffer<ScopedObjectTextureRef> &textureRefs = defIter->textureRefs;
	return textureRefs.get(linearizedKeyframeIndex).get();
}

BufferView<const RenderDrawCall> RenderChunkManager::getVoxelDrawCalls() const
{
	return BufferView<const RenderDrawCall>(this->voxelDrawCallsCache);
}

BufferView<const RenderDrawCall> RenderChunkManager::getEntityDrawCalls() const
{
	return BufferView<const RenderDrawCall>(this->entityDrawCallsCache);
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
				const int opaqueIndexCount = voxelMeshDef.getOpaqueIndicesList(bufferIndex).getCount();
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

void RenderChunkManager::loadVoxelChasmWall(RenderChunk &renderChunk, const VoxelChunk &voxelChunk, SNInt x, int y, WEInt z)
{
	int chasmWallInstIndex;
	if (!voxelChunk.tryGetChasmWallInstIndex(x, y, z, &chasmWallInstIndex))
	{
		return;
	}

	BufferView<const VoxelChasmWallInstance> chasmWallInsts = voxelChunk.getChasmWallInsts();
	const VoxelChasmWallInstance &chasmWallInst = chasmWallInsts[chasmWallInstIndex];
	DebugAssert(chasmWallInst.getFaceCount() > 0);

	const int chasmWallIndexBufferIndex = ArenaMeshUtils::GetChasmWallIndex(
		chasmWallInst.north, chasmWallInst.east, chasmWallInst.south, chasmWallInst.west);
	const IndexBufferID indexBufferID = this->chasmWallIndexBufferIDs[chasmWallIndexBufferIndex];

	const VoxelInt3 voxel(x, y, z);
	auto &chasmWallIndexBufferIDsMap = renderChunk.chasmWallIndexBufferIDsMap;
	const auto iter = chasmWallIndexBufferIDsMap.find(voxel);
	if (iter == chasmWallIndexBufferIDsMap.end())
	{
		chasmWallIndexBufferIDsMap.emplace(voxel, indexBufferID);
	}
	else
	{
		iter->second = indexBufferID;
	}
}

void RenderChunkManager::loadVoxelChasmWalls(RenderChunk &renderChunk, const VoxelChunk &voxelChunk)
{
	for (WEInt z = 0; z < Chunk::DEPTH; z++)
	{
		for (int y = 0; y < voxelChunk.getHeight(); y++)
		{
			for (SNInt x = 0; x < Chunk::WIDTH; x++)
			{
				this->loadVoxelChasmWall(renderChunk, voxelChunk, x, y, z);
			}
		}
	}
}

void RenderChunkManager::loadEntityTextures(const EntityChunk &entityChunk, const EntityChunkManager &entityChunkManager,
	TextureManager &textureManager, Renderer &renderer)
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

		if (animIter == this->entityAnims.end())
		{
			const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityDefID);
			const EntityAnimationDefinition &animDef = entityDef.getAnimDef();
			Buffer<ScopedObjectTextureRef> textureRefs = sgTexture::MakeEntityAnimationTextures(animDef, textureManager, renderer);

			LoadedEntityAnimation loadedEntityAnim;
			loadedEntityAnim.init(entityDefID, std::move(textureRefs));
			this->entityAnims.emplace_back(std::move(loadedEntityAnim));
		}

		if (entityInst.isCitizen())
		{
			const EntityPaletteIndicesInstanceID paletteIndicesInstID = entityInst.paletteIndicesInstID;
			const auto paletteIndicesIter = this->entityPaletteIndicesTextureRefs.find(paletteIndicesInstID);
			if (paletteIndicesIter == this->entityPaletteIndicesTextureRefs.end())
			{
				const PaletteIndices &paletteIndices = entityChunkManager.getEntityPaletteIndices(paletteIndicesInstID);
				ScopedObjectTextureRef paletteIndicesTextureRef = sgTexture::MakeEntityPaletteIndicesTextureRef(paletteIndices, renderer);
				this->entityPaletteIndicesTextureRefs.emplace(paletteIndicesInstID, std::move(paletteIndicesTextureRef));
			}
		}
	}
}

void RenderChunkManager::addVoxelDrawCall(const Double3 &position, const Double3 &preScaleTranslation, const Matrix4d &rotationMatrix,
	const Matrix4d &scaleMatrix, VertexBufferID vertexBufferID, AttributeBufferID normalBufferID, AttributeBufferID texCoordBufferID,
	IndexBufferID indexBufferID, ObjectTextureID textureID0, const std::optional<ObjectTextureID> &textureID1,
	TextureSamplingType textureSamplingType0, TextureSamplingType textureSamplingType1, RenderLightingType lightingType,
	double meshLightPercent, BufferView<const RenderLightID> lightIDs, VertexShaderType vertexShaderType, PixelShaderType pixelShaderType,
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
	drawCall.textureSamplingType0 = textureSamplingType0;
	drawCall.textureSamplingType1 = textureSamplingType1;
	drawCall.lightingType = lightingType;
	drawCall.lightPercent = meshLightPercent;

	DebugAssert(std::size(drawCall.lightIDs) >= lightIDs.getCount());
	std::copy(lightIDs.begin(), lightIDs.end(), std::begin(drawCall.lightIDs));
	drawCall.lightIdCount = lightIDs.getCount();

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
					BufferView<const VoxelFadeAnimationInstance> fadeAnimInsts = voxelChunk.getFadeAnimInsts();
					fadeAnimInst = &fadeAnimInsts[fadeAnimInstIndex];
				}

				const RenderVoxelLightIdList &voxelLightIdList = renderChunk.voxelLightIdLists.get(x, y, z);

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

						RenderLightingType lightingType = RenderLightingType::PerPixel;
						double meshLightPercent = 0.0;
						if (isFading)
						{
							lightingType = RenderLightingType::PerMesh;
							meshLightPercent = std::clamp(1.0 - fadeAnimInst->percentFaded, 0.0, 1.0);
						}

						std::vector<RenderDrawCall> *drawCallsPtr = nullptr;
						if (isChasm)
						{
							const ChasmDefinition &chasmDef = voxelChunk.getChasmDef(chasmDefID);
							if (chasmDef.isEmissive)
							{
								lightingType = RenderLightingType::PerMesh;
								meshLightPercent = 1.0;
							}

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

						const PixelShaderType pixelShaderType = PixelShaderType::Opaque;
						const double pixelShaderParam0 = 0.0;
						this->addVoxelDrawCall(worldPos, preScaleTranslation, rotationMatrix, scaleMatrix, renderMeshDef.vertexBufferID,
							renderMeshDef.normalBufferID, renderMeshDef.texCoordBufferID, opaqueIndexBufferID, textureID, std::nullopt,
							textureSamplingType, textureSamplingType, lightingType, meshLightPercent, voxelLightIdList.getLightIDs(),
							VertexShaderType::Voxel, pixelShaderType, pixelShaderParam0, *drawCallsPtr);
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
								BufferView<const VoxelDoorAnimationInstance> doorAnimInsts = voxelChunk.getDoorAnimInsts();
								const VoxelDoorAnimationInstance &doorAnimInst = doorAnimInsts[doorAnimInstIndex];
								doorAnimPercent = doorAnimInst.percentOpen;
							}

							int doorVisInstIndex;
							if (!voxelChunk.tryGetDoorVisibilityInstIndex(x, y, z, &doorVisInstIndex))
							{
								DebugLogError("Expected door visibility instance at (" + std::to_string(x) + ", " +
									std::to_string(y) + ", " + std::to_string(z) + ") in chunk (" + chunkPos.toString() + ").");
								continue;
							}

							BufferView<const VoxelDoorVisibilityInstance> doorVisInsts = voxelChunk.getDoorVisibilityInsts();
							const VoxelDoorVisibilityInstance &doorVisInst = doorVisInsts[doorVisInstIndex];
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
									const TextureSamplingType textureSamplingType = TextureSamplingType::Default;
									constexpr double meshLightPercent = 0.0;
									constexpr double pixelShaderParam0 = 0.0;
									this->addVoxelDrawCall(doorHingePosition, doorPreScaleTranslation, doorRotationMatrix, doorScaleMatrix,
										renderMeshDef.vertexBufferID, renderMeshDef.normalBufferID, renderMeshDef.texCoordBufferID,
										renderMeshDef.alphaTestedIndexBufferID, textureID, std::nullopt, textureSamplingType, textureSamplingType,
										RenderLightingType::PerPixel, meshLightPercent, voxelLightIdList.getLightIDs(), VertexShaderType::SwingingDoor,
										PixelShaderType::AlphaTested, pixelShaderParam0, renderChunk.doorDrawCalls);
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
									const TextureSamplingType textureSamplingType = TextureSamplingType::Default;
									constexpr double meshLightPercent = 0.0;
									const double pixelShaderParam0 = uMin;
									this->addVoxelDrawCall(doorHingePosition, doorPreScaleTranslation, doorRotationMatrix, doorScaleMatrix,
										renderMeshDef.vertexBufferID, renderMeshDef.normalBufferID, renderMeshDef.texCoordBufferID,
										renderMeshDef.alphaTestedIndexBufferID, textureID, std::nullopt, textureSamplingType, textureSamplingType,
										RenderLightingType::PerPixel, meshLightPercent, voxelLightIdList.getLightIDs(), VertexShaderType::SlidingDoor,
										PixelShaderType::AlphaTestedWithVariableTexCoordUMin, pixelShaderParam0, renderChunk.doorDrawCalls);
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
									const TextureSamplingType textureSamplingType = TextureSamplingType::Default;
									constexpr double meshLightPercent = 0.0;
									const double pixelShaderParam0 = vMin;
									this->addVoxelDrawCall(doorHingePosition, doorPreScaleTranslation, doorRotationMatrix, doorScaleMatrix,
										renderMeshDef.vertexBufferID, renderMeshDef.normalBufferID, renderMeshDef.texCoordBufferID,
										renderMeshDef.alphaTestedIndexBufferID, textureID, std::nullopt, textureSamplingType, textureSamplingType,
										RenderLightingType::PerPixel, meshLightPercent, voxelLightIdList.getLightIDs(), VertexShaderType::RaisingDoor,
										PixelShaderType::AlphaTestedWithVariableTexCoordVMin, pixelShaderParam0, renderChunk.doorDrawCalls);
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
							const TextureSamplingType textureSamplingType = TextureSamplingType::Default;

							RenderLightingType lightingType = RenderLightingType::PerPixel;
							double meshLightPercent = 0.0;
							std::vector<RenderDrawCall> *drawCallsPtr = &renderChunk.staticDrawCalls;
							if (isFading)
							{
								lightingType = RenderLightingType::PerMesh;
								meshLightPercent = std::clamp(1.0 - fadeAnimInst->percentFaded, 0.0, 1.0);
								drawCallsPtr = &renderChunk.fadingDrawCalls;
							}

							constexpr double pixelShaderParam0 = 0.0;
							this->addVoxelDrawCall(worldPos, preScaleTranslation, rotationMatrix, scaleMatrix, renderMeshDef.vertexBufferID,
								renderMeshDef.normalBufferID, renderMeshDef.texCoordBufferID, renderMeshDef.alphaTestedIndexBufferID,
								textureID, std::nullopt, textureSamplingType, textureSamplingType, lightingType, meshLightPercent,
								voxelLightIdList.getLightIDs(), VertexShaderType::Voxel, PixelShaderType::AlphaTested, pixelShaderParam0, *drawCallsPtr);
						}
					}
				}

				if (isChasm)
				{
					const auto chasmWallIter = renderChunk.chasmWallIndexBufferIDsMap.find(voxel);
					if (chasmWallIter != renderChunk.chasmWallIndexBufferIDsMap.end())
					{
						DebugAssert(voxelTraitsDef.type == ArenaTypes::VoxelType::Chasm);
						const ArenaTypes::ChasmType chasmType = voxelTraitsDef.chasm.type;
						const bool isAnimatingChasm = chasmType != ArenaTypes::ChasmType::Dry;
						const IndexBufferID chasmWallIndexBufferID = chasmWallIter->second;

						// Need to give two textures since chasm walls are multi-textured.
						ObjectTextureID textureID0 = this->getChasmFloorTextureID(chunkPos, chasmDefID, chasmAnimPercent);
						ObjectTextureID textureID1 = this->getChasmWallTextureID(chunkPos, chasmDefID);

						const Double3 preScaleTranslation = Double3::Zero;
						const Matrix4d rotationMatrix = Matrix4d::identity();
						const Matrix4d scaleMatrix = Matrix4d::identity();
						const TextureSamplingType textureSamplingType = isAnimatingChasm ? TextureSamplingType::ScreenSpaceRepeatY : TextureSamplingType::Default;

						double meshLightPercent = 0.0;
						RenderLightingType lightingType = RenderLightingType::PerPixel;
						if (RendererUtils::isChasmEmissive(chasmType))
						{
							meshLightPercent = 1.0;
							lightingType = RenderLightingType::PerMesh;
						}

						constexpr double pixelShaderParam0 = 0.0;
						this->addVoxelDrawCall(worldPos, preScaleTranslation, rotationMatrix, scaleMatrix, renderMeshDef.vertexBufferID,
							renderMeshDef.normalBufferID, renderMeshDef.texCoordBufferID, chasmWallIndexBufferID, textureID0, textureID1,
							textureSamplingType, textureSamplingType, lightingType, meshLightPercent, voxelLightIdList.getLightIDs(),
							VertexShaderType::Voxel, PixelShaderType::OpaqueWithAlphaTestLayer, pixelShaderParam0, renderChunk.chasmDrawCalls);
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
		BufferView<const RenderDrawCall> staticDrawCalls = chunkPtr->staticDrawCalls;
		BufferView<const RenderDrawCall> doorDrawCalls = chunkPtr->doorDrawCalls;
		BufferView<const RenderDrawCall> chasmDrawCalls = chunkPtr->chasmDrawCalls;
		BufferView<const RenderDrawCall> fadingDrawCalls = chunkPtr->fadingDrawCalls;
		this->voxelDrawCallsCache.insert(this->voxelDrawCallsCache.end(), staticDrawCalls.begin(), staticDrawCalls.end());
		this->voxelDrawCallsCache.insert(this->voxelDrawCallsCache.end(), doorDrawCalls.begin(), doorDrawCalls.end());
		this->voxelDrawCallsCache.insert(this->voxelDrawCallsCache.end(), chasmDrawCalls.begin(), chasmDrawCalls.end());
		this->voxelDrawCallsCache.insert(this->voxelDrawCallsCache.end(), fadingDrawCalls.begin(), fadingDrawCalls.end());
	}
}

void RenderChunkManager::addEntityDrawCall(const Double3 &position, const Matrix4d &rotationMatrix, const Matrix4d &scaleMatrix,
	ObjectTextureID textureID0, const std::optional<ObjectTextureID> &textureID1, BufferView<const RenderLightID> lightIDs,
	PixelShaderType pixelShaderType, std::vector<RenderDrawCall> &drawCalls)
{
	RenderDrawCall drawCall;
	drawCall.position = position;
	drawCall.preScaleTranslation = Double3::Zero;
	drawCall.rotation = rotationMatrix;
	drawCall.scale = scaleMatrix;
	drawCall.vertexBufferID = this->entityMeshDef.vertexBufferID;
	drawCall.normalBufferID = this->entityMeshDef.normalBufferID;
	drawCall.texCoordBufferID = this->entityMeshDef.texCoordBufferID;
	drawCall.indexBufferID = this->entityMeshDef.indexBufferID;
	drawCall.textureIDs[0] = textureID0;
	drawCall.textureIDs[1] = textureID1;
	drawCall.textureSamplingType0 = TextureSamplingType::Default;
	drawCall.textureSamplingType1 = TextureSamplingType::Default;
	drawCall.lightingType = RenderLightingType::PerPixel;
	drawCall.lightPercent = 0.0;

	DebugAssert(std::size(drawCall.lightIDs) >= lightIDs.getCount());
	std::copy(lightIDs.begin(), lightIDs.end(), std::begin(drawCall.lightIDs));
	drawCall.lightIdCount = lightIDs.getCount();

	drawCall.vertexShaderType = VertexShaderType::Entity;
	drawCall.pixelShaderType = pixelShaderType;
	drawCall.pixelShaderParam0 = 0.0;

	drawCalls.emplace_back(std::move(drawCall));
}

void RenderChunkManager::rebuildEntityChunkDrawCalls(RenderChunk &renderChunk, const EntityChunk &entityChunk,
	const CoordDouble2 &cameraCoordXZ, const Matrix4d &rotationMatrix, double ceilingScale,
	const VoxelChunkManager &voxelChunkManager, const EntityChunkManager &entityChunkManager)
{
	renderChunk.entityDrawCalls.clear();

	BufferView<const EntityInstanceID> entityIDs = entityChunk.entityIDs;
	const int entityCount = entityIDs.getCount();
	for (int i = 0; i < entityCount; i++)
	{
		const EntityInstanceID entityInstID = entityIDs[i];
		const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
		const CoordDouble2 &entityCoord = entityChunkManager.getEntityPosition(entityInst.positionID);
		const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityInst.defID);
		const EntityAnimationDefinition &animDef = entityDef.getAnimDef();

		EntityVisibilityState3D visState;
		entityChunkManager.getEntityVisibilityState3D(entityInstID, cameraCoordXZ, ceilingScale, voxelChunkManager, visState);
		const int linearizedKeyframeIndex = animDef.getLinearizedKeyframeIndex(visState.stateIndex, visState.angleIndex, visState.keyframeIndex);
		DebugAssertIndex(animDef.keyframes, linearizedKeyframeIndex);
		const EntityAnimationDefinitionKeyframe &keyframe = animDef.keyframes[linearizedKeyframeIndex];

		// Convert entity XYZ to world space.
		const Double3 worldPos = VoxelUtils::coordToWorldPoint(visState.flatPosition);
		const Matrix4d scaleMatrix = Matrix4d::scale(1.0, keyframe.height, keyframe.width);

		const ObjectTextureID textureID0 = this->getEntityTextureID(entityInstID, cameraCoordXZ, entityChunkManager);
		std::optional<ObjectTextureID> textureID1 = std::nullopt;
		PixelShaderType pixelShaderType = PixelShaderType::AlphaTested;

		const bool isCitizen = entityInst.isCitizen();
		const bool isGhost = EntityUtils::isGhost(entityDef);
		if (isCitizen)
		{
			const EntityPaletteIndicesInstanceID paletteIndicesInstID = entityInst.paletteIndicesInstID;
			const auto paletteIndicesIter = this->entityPaletteIndicesTextureRefs.find(paletteIndicesInstID);
			DebugAssertMsg(paletteIndicesIter != this->entityPaletteIndicesTextureRefs.end(), "Expected entity palette indices texture for ID " + std::to_string(paletteIndicesInstID) + ".");
			textureID1 = paletteIndicesIter->second.get();
			pixelShaderType = PixelShaderType::AlphaTestedWithPaletteIndexLookup;
		}
		else if (isGhost)
		{
			pixelShaderType = PixelShaderType::AlphaTestedWithLightLevelOpacity;
		}

		const VoxelDouble3 &entityLightPoint = visState.flatPosition.point; // Where the entity receives its light (can't use center due to some really tall entities reaching outside the chunk).
		const VoxelInt3 entityLightVoxel = VoxelUtils::pointToVoxel(entityLightPoint, ceilingScale);
		BufferView<const RenderLightID> lightIdsView; // Limitation of reusing lights per voxel: entity is unlit if they are outside the world.
		if (renderChunk.isValidVoxel(entityLightVoxel.x, entityLightVoxel.y, entityLightVoxel.z))
		{
			const RenderVoxelLightIdList &voxelLightIdList = renderChunk.voxelLightIdLists.get(entityLightVoxel.x, entityLightVoxel.y, entityLightVoxel.z);
			lightIdsView = voxelLightIdList.getLightIDs();
		}

		this->addEntityDrawCall(worldPos, rotationMatrix, scaleMatrix, textureID0, textureID1, lightIdsView, pixelShaderType, renderChunk.entityDrawCalls);
	}
}

void RenderChunkManager::rebuildEntityDrawCallsList()
{
	this->entityDrawCallsCache.clear();

	// @todo: eventually this should sort by distance from a CoordDouble2
	for (size_t i = 0; i < this->activeChunks.size(); i++)
	{
		const ChunkPtr &chunkPtr = this->activeChunks[i];
		BufferView<const RenderDrawCall> entityDrawCalls = chunkPtr->entityDrawCalls;
		this->entityDrawCallsCache.insert(this->entityDrawCallsCache.end(), entityDrawCalls.begin(), entityDrawCalls.end());
	}
}

void RenderChunkManager::updateActiveChunks(BufferView<const ChunkInt2> activeChunkPositions,
	BufferView<const ChunkInt2> newChunkPositions, BufferView<const ChunkInt2> freedChunkPositions,
	const VoxelChunkManager &voxelChunkManager, Renderer &renderer)
{
	for (const ChunkInt2 &chunkPos : freedChunkPositions)
	{
		const int chunkIndex = this->getChunkIndex(chunkPos);
		RenderChunk &renderChunk = this->getChunkAtIndex(chunkIndex);
		renderChunk.freeBuffers(renderer);
		this->recycleChunk(chunkIndex);
	}

	for (const ChunkInt2 &chunkPos : newChunkPositions)
	{
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);

		const int spawnIndex = this->spawnChunk();
		RenderChunk &renderChunk = this->getChunkAtIndex(spawnIndex);
		renderChunk.init(chunkPos, voxelChunk.getHeight());
	}

	// Free any unneeded chunks for memory savings in case the chunk distance was once large
	// and is now small. This is significant even for chunk distance 2->1, or 25->9 chunks.
	this->chunkPool.clear();
}

void RenderChunkManager::updateVoxels(BufferView<const ChunkInt2> activeChunkPositions,
	BufferView<const ChunkInt2> newChunkPositions, double ceilingScale, double chasmAnimPercent,
	const VoxelChunkManager &voxelChunkManager, TextureManager &textureManager, Renderer &renderer)
{
	for (const ChunkInt2 &chunkPos : newChunkPositions)
	{
		RenderChunk &renderChunk = this->getChunkAtPosition(chunkPos);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		this->loadVoxelTextures(voxelChunk, textureManager, renderer);
		this->loadVoxelMeshBuffers(renderChunk, voxelChunk, ceilingScale, renderer);
		this->loadVoxelChasmWalls(renderChunk, voxelChunk);
		this->rebuildVoxelChunkDrawCalls(renderChunk, voxelChunk, ceilingScale, chasmAnimPercent, true, false);
	}

	for (const ChunkInt2 &chunkPos : activeChunkPositions)
	{
		RenderChunk &renderChunk = this->getChunkAtPosition(chunkPos);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);

		BufferView<const VoxelInt3> dirtyChasmWallInstPositions = voxelChunk.getDirtyChasmWallInstPositions();
		for (const VoxelInt3 &chasmWallPos : dirtyChasmWallInstPositions)
		{
			this->loadVoxelChasmWall(renderChunk, voxelChunk, chasmWallPos.x, chasmWallPos.y, chasmWallPos.z);
		}

		BufferView<const VoxelInt3> dirtyMeshDefPositions = voxelChunk.getDirtyMeshDefPositions();
		BufferView<const VoxelInt3> dirtyFadeAnimInstPositions = voxelChunk.getDirtyFadeAnimInstPositions();
		BufferView<const VoxelInt3> dirtyLightPositions = renderChunk.dirtyLightPositions;
		bool updateStatics = dirtyMeshDefPositions.getCount() > 0;
		updateStatics |= dirtyFadeAnimInstPositions.getCount() > 0; // @temp fix for fading voxels being covered by their non-fading draw call
		updateStatics |= dirtyLightPositions.getCount() > 0; // @temp fix for player light movement, eventually other moving lights too
		this->rebuildVoxelChunkDrawCalls(renderChunk, voxelChunk, ceilingScale, chasmAnimPercent, updateStatics, true);
	}

	// @todo: only rebuild if needed; currently we assume that all scenes in the game have some kind of animating chasms/etc., which is inefficient
	//if ((freedChunkCount > 0) || (newChunkCount > 0))
	{
		this->rebuildVoxelDrawCallsList();
	}
}

void RenderChunkManager::updateEntities(BufferView<const ChunkInt2> activeChunkPositions,
	BufferView<const ChunkInt2> newChunkPositions, const CoordDouble2 &cameraCoordXZ, const VoxelDouble2 &cameraDirXZ,
	double ceilingScale, const VoxelChunkManager &voxelChunkManager, const EntityChunkManager &entityChunkManager,
	TextureManager &textureManager, Renderer &renderer)
{
	for (const EntityInstanceID entityInstID : entityChunkManager.getQueuedDestroyEntityIDs())
	{
		const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
		if (entityInst.isCitizen())
		{
			const EntityPaletteIndicesInstanceID paletteIndicesInstID = entityInst.paletteIndicesInstID;
			const auto paletteIndicesIter = this->entityPaletteIndicesTextureRefs.find(paletteIndicesInstID);
			if (paletteIndicesIter != this->entityPaletteIndicesTextureRefs.end())
			{
				this->entityPaletteIndicesTextureRefs.erase(paletteIndicesIter);
			}
		}
	}

	for (const ChunkInt2 &chunkPos : newChunkPositions)
	{
		RenderChunk &renderChunk = this->getChunkAtPosition(chunkPos);
		const EntityChunk &entityChunk = entityChunkManager.getChunkAtPosition(chunkPos);
		this->loadEntityTextures(entityChunk, entityChunkManager, textureManager, renderer);
	}

	const Radians rotationAngle = -MathUtils::fullAtan2(cameraDirXZ);
	const Matrix4d rotationMatrix = Matrix4d::yRotation(rotationAngle - Constants::HalfPi);
	for (const ChunkInt2 &chunkPos : activeChunkPositions)
	{
		RenderChunk &renderChunk = this->getChunkAtPosition(chunkPos);
		const EntityChunk &entityChunk = entityChunkManager.getChunkAtPosition(chunkPos);
		this->rebuildEntityChunkDrawCalls(renderChunk, entityChunk, cameraCoordXZ, rotationMatrix, ceilingScale,
			voxelChunkManager, entityChunkManager);
	}

	this->rebuildEntityDrawCallsList();

	// Update normals buffer.
	const VoxelDouble2 entityDir = -cameraDirXZ;
	constexpr int entityMeshVertexCount = 4;
	const std::array<double, entityMeshVertexCount * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX> entityNormals =
	{
		entityDir.x, 0.0, entityDir.y,
		entityDir.x, 0.0, entityDir.y,
		entityDir.x, 0.0, entityDir.y,
		entityDir.x, 0.0, entityDir.y
	};

	renderer.populateAttributeBuffer(this->entityMeshDef.normalBufferID, entityNormals);
}

void RenderChunkManager::updateLights(BufferView<const ChunkInt2> activeChunkPositions, BufferView<const ChunkInt2> newChunkPositions,
	const CoordDouble3 &cameraCoord, double ceilingScale, bool isFogActive, bool nightLightsAreActive, bool playerHasLight,
	const EntityChunkManager &entityChunkManager, Renderer &renderer)
{
	for (const EntityInstanceID entityInstID : entityChunkManager.getQueuedDestroyEntityIDs())
	{
		const auto iter = this->entityLights.find(entityInstID);
		if (iter != this->entityLights.end())
		{
			const Light &light = iter->second;
			renderer.freeLight(light.lightID);
			this->entityLights.erase(iter);
		}
	}

	for (const ChunkInt2 &chunkPos : newChunkPositions)
	{
		const EntityChunk &entityChunk = entityChunkManager.getChunkAtPosition(chunkPos);
		for (const EntityInstanceID entityInstID : entityChunk.entityIDs)
		{
			const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
			const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityInst.defID);
			const std::optional<double> entityLightRadius = EntityUtils::tryGetLightRadius(entityDef);
			const bool isLight = entityLightRadius.has_value();
			if (isLight)
			{
				RenderLightID lightID;
				if (!renderer.tryCreateLight(&lightID))
				{
					DebugLogError("Couldn't allocate render light ID in chunk (" + chunkPos.toString() + ").");
					continue;
				}

				const bool isLightEnabled = !EntityUtils::isStreetlight(entityDef) || nightLightsAreActive;

				Light light;
				light.init(lightID, isLightEnabled);
				this->entityLights.emplace(entityInstID, std::move(light));

				const CoordDouble2 &entityCoord = entityChunkManager.getEntityPosition(entityInst.positionID);
				const WorldDouble2 entityPos = VoxelUtils::coordToWorldPoint(entityCoord);

				double dummyAnimMaxWidth, animMaxHeight;
				EntityUtils::getAnimationMaxDims(entityDef.getAnimDef(), &dummyAnimMaxWidth, &animMaxHeight);

				const double entityPosY = ceilingScale + (animMaxHeight * 0.50);
				const WorldDouble3 entityPos3D(entityPos.x, entityPosY, entityPos.y);
				renderer.setLightPosition(lightID, entityPos3D);
				renderer.setLightRadius(lightID, ArenaRenderUtils::PLAYER_LIGHT_START_RADIUS, *entityLightRadius);
			}
		}
	}

	const WorldDouble3 prevPlayerLightPosition = renderer.getLightPosition(this->playerLightID);
	const WorldDouble3 playerLightPosition = VoxelUtils::coordToWorldPoint(cameraCoord);
	renderer.setLightPosition(this->playerLightID, playerLightPosition);

	double playerLightRadiusStart, playerLightRadiusEnd;
	if (isFogActive)
	{
		playerLightRadiusStart = ArenaRenderUtils::PLAYER_FOG_LIGHT_START_RADIUS;
		playerLightRadiusEnd = ArenaRenderUtils::PLAYER_FOG_LIGHT_END_RADIUS;
	}
	else
	{
		playerLightRadiusStart = ArenaRenderUtils::PLAYER_LIGHT_START_RADIUS;
		playerLightRadiusEnd = ArenaRenderUtils::PLAYER_LIGHT_END_RADIUS;
	}

	if (!playerHasLight)
	{
		playerLightRadiusStart = 0.0;
		playerLightRadiusEnd = 0.0;
	}

	renderer.setLightRadius(this->playerLightID, playerLightRadiusStart, playerLightRadiusEnd);

	// Clear all previous light references in voxels.
	// @todo: track touched, newly-touched, and no-longer-touched voxels each frame for moving lights so we don't have to iterate all voxels
	for (int i = 0; i < this->getChunkCount(); i++)
	{
		RenderChunk &renderChunk = this->getChunkAtIndex(i);
		for (RenderVoxelLightIdList &voxelLightIdList : renderChunk.voxelLightIdLists)
		{
			voxelLightIdList.clear();
		}
	}

	auto getLightMinAndMaxVoxels = [ceilingScale](const WorldDouble3 &lightPosition, double endRadius, WorldInt3 *outMin, WorldInt3 *outMax)
	{
		const WorldDouble3 lightPosMin = lightPosition - WorldDouble3(endRadius, endRadius, endRadius);
		const WorldDouble3 lightPosMax = lightPosition + WorldDouble3(endRadius, endRadius, endRadius);
		*outMin = VoxelUtils::pointToVoxel(lightPosMin, ceilingScale);
		*outMax = VoxelUtils::pointToVoxel(lightPosMax, ceilingScale);
	};

	auto populateTouchedVoxelLightIdLists = [this, &renderer, ceilingScale](RenderLightID lightID, const WorldDouble3 &lightPosition,
		const WorldInt3 &lightVoxelMin, const WorldInt3 &lightVoxelMax)
	{
		// Iterate over all voxels the light's bounding box touches.
		for (WEInt z = lightVoxelMin.z; z <= lightVoxelMax.z; z++)
		{
			for (int y = lightVoxelMin.y; y <= lightVoxelMax.y; y++)
			{
				for (SNInt x = lightVoxelMin.x; x <= lightVoxelMax.x; x++)
				{
					const CoordInt3 curLightCoord = VoxelUtils::worldVoxelToCoord(WorldInt3(x, y, z));
					RenderChunk *renderChunkPtr = this->tryGetChunkAtPosition(curLightCoord.chunk);
					if (renderChunkPtr != nullptr)
					{
						const VoxelInt3 &curLightVoxel = curLightCoord.voxel;
						if (renderChunkPtr->isValidVoxel(curLightVoxel.x, curLightVoxel.y, curLightVoxel.z))
						{
							RenderVoxelLightIdList &voxelLightIdList = renderChunkPtr->voxelLightIdLists.get(curLightVoxel.x, curLightVoxel.y, curLightVoxel.z);
							voxelLightIdList.tryAddLight(lightID);
						}
					}
				}
			}
		}
	};

	WorldInt3 playerLightVoxelMin, playerLightVoxelMax;
	getLightMinAndMaxVoxels(playerLightPosition, playerLightRadiusEnd, &playerLightVoxelMin, &playerLightVoxelMax);
	populateTouchedVoxelLightIdLists(this->playerLightID, playerLightPosition, playerLightVoxelMin, playerLightVoxelMax);

	// Populate each voxel's light ID list based on which lights touch them, preferring the nearest lights.
	// - @todo: this method doesn't implicitly allow sorting by distance because it doesn't check lights per voxel, it checks voxels per light.
	//   If sorting is desired then do it after this loop. It should also sort by intensity at the voxel center, not just distance to the light.
	for (const auto &pair : this->entityLights)
	{
		const Light &light = pair.second;
		if (light.enabled)
		{
			const RenderLightID lightID = light.lightID;
			const WorldDouble3 &lightPosition = renderer.getLightPosition(lightID);

			double dummyLightStartRadius, lightEndRadius;
			renderer.getLightRadii(lightID, &dummyLightStartRadius, &lightEndRadius);

			WorldInt3 lightVoxelMin, lightVoxelMax;
			getLightMinAndMaxVoxels(lightPosition, lightEndRadius, &lightVoxelMin, &lightVoxelMax);
			populateTouchedVoxelLightIdLists(lightID, lightPosition, lightVoxelMin, lightVoxelMax);
		}
	}

	WorldInt3 prevPlayerLightVoxelMin, prevPlayerLightVoxelMax;
	getLightMinAndMaxVoxels(prevPlayerLightPosition, playerLightRadiusEnd, &prevPlayerLightVoxelMin, &prevPlayerLightVoxelMax);

	// See which voxels affected by the player's light are getting their light references updated.
	// This is for dirty voxel draw calls mainly, not about setting light references (might change later).
	if ((prevPlayerLightVoxelMin != playerLightVoxelMin) || (prevPlayerLightVoxelMax != playerLightVoxelMax))
	{
		for (WEInt z = playerLightVoxelMin.z; z <= playerLightVoxelMax.z; z++)
		{
			for (int y = playerLightVoxelMin.y; y <= playerLightVoxelMax.y; y++)
			{
				for (SNInt x = playerLightVoxelMin.x; x <= playerLightVoxelMax.x; x++)
				{
					const bool isInPrevRange =
						(x >= prevPlayerLightVoxelMin.x) && (x <= prevPlayerLightVoxelMax.x) &&
						(y >= prevPlayerLightVoxelMin.y) && (y <= prevPlayerLightVoxelMax.y) &&
						(z >= prevPlayerLightVoxelMin.z) && (z <= prevPlayerLightVoxelMax.z);
					
					if (!isInPrevRange)
					{
						const CoordInt3 curLightCoord = VoxelUtils::worldVoxelToCoord(WorldInt3(x, y, z));
						RenderChunk *renderChunkPtr = this->tryGetChunkAtPosition(curLightCoord.chunk);
						if (renderChunkPtr != nullptr)
						{
							renderChunkPtr->addDirtyLightPosition(curLightCoord.voxel);
						}
					}
				}
			}
		}
	}
}

void RenderChunkManager::setNightLightsActive(bool enabled, const EntityChunkManager &entityChunkManager)
{
	// Update streetlight enabled states.
	for (ChunkPtr &chunkPtr : this->activeChunks)
	{
		const ChunkInt2 &chunkPos = chunkPtr->getPosition();
		const EntityChunk &entityChunk = entityChunkManager.getChunkAtPosition(chunkPos);
		for (const EntityInstanceID entityInstID : entityChunk.entityIDs)
		{
			const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
			const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityInst.defID);
			if (EntityUtils::isStreetlight(entityDef))
			{
				const auto lightIter = this->entityLights.find(entityInstID);
				DebugAssertMsg(lightIter != this->entityLights.end(), "Couldn't find light for streetlight entity \"" + std::to_string(entityInstID) + "\" in chunk (" + chunkPos.toString() + ").");

				Light &light = lightIter->second;
				light.enabled = enabled;
			}
		}
	}
}

void RenderChunkManager::cleanUp()
{
	for (ChunkPtr &chunkPtr : this->activeChunks)
	{
		chunkPtr->dirtyLightPositions.clear();
	}
}

void RenderChunkManager::unloadScene(Renderer &renderer)
{
	this->voxelTextures.clear();
	this->chasmFloorTextureLists.clear();
	this->chasmTextureKeys.clear();
	this->entityAnims.clear();
	this->entityPaletteIndicesTextureRefs.clear();

	// Free vertex/attribute/index buffer IDs.
	for (int i = static_cast<int>(this->activeChunks.size()) - 1; i >= 0; i--)
	{
		ChunkPtr &chunkPtr = this->activeChunks[i];
		chunkPtr->freeBuffers(renderer);
		this->recycleChunk(i);
	}

	for (auto &pair : this->entityLights)
	{
		Light &light = pair.second;
		if (light.lightID >= 0)
		{
			renderer.freeLight(light.lightID);
			light.lightID = -1;
		}
	}

	this->entityLights.clear();
	this->voxelDrawCallsCache.clear();
	this->entityDrawCallsCache.clear();
}
