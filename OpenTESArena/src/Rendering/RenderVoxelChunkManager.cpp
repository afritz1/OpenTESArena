#include <algorithm>
#include <array>
#include <numeric>
#include <optional>

#include "RenderLightChunkManager.h"
#include "RenderVoxelChunkManager.h"
#include "Renderer.h"
#include "RendererUtils.h"
#include "../Assets/TextureManager.h"
#include "../Voxels/DoorUtils.h"
#include "../Voxels/VoxelChunkManager.h"
#include "../Voxels/VoxelVisibilityChunk.h"
#include "../Voxels/VoxelVisibilityChunkManager.h"

#include "components/debug/Debug.h"

namespace
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
	void LoadVoxelDefTextures(const VoxelTextureDefinition &voxelTextureDef, std::vector<RenderVoxelChunkManager::LoadedTexture> &textures,
		TextureManager &textureManager, Renderer &renderer)
	{
		for (int i = 0; i < voxelTextureDef.textureCount; i++)
		{
			const TextureAsset &textureAsset = voxelTextureDef.getTextureAsset(i);
			const auto cacheIter = std::find_if(textures.begin(), textures.end(),
				[&textureAsset](const RenderVoxelChunkManager::LoadedTexture &loadedTexture)
			{
				return loadedTexture.textureAsset == textureAsset;
			});

			if (cacheIter == textures.end())
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
				RenderVoxelChunkManager::LoadedTexture newTexture;
				newTexture.init(textureAsset, std::move(voxelTextureRef));
				textures.emplace_back(std::move(newTexture));
			}
		}
	}

	bool LoadedChasmFloorComparer(const RenderVoxelChunkManager::LoadedChasmFloorTextureList &textureList, const ChasmDefinition &chasmDef)
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
		BufferView<const RenderVoxelChunkManager::LoadedTexture> textures,
		std::vector<RenderVoxelChunkManager::LoadedChasmFloorTextureList> &chasmFloorTextureLists,
		std::vector<RenderVoxelChunkManager::LoadedChasmTextureKey> &chasmTextureKeys,
		TextureManager &textureManager, Renderer &renderer)
	{
		const ChunkInt2 chunkPos = voxelChunk.getPosition();
		const ChasmDefinition &chasmDef = voxelChunk.getChasmDef(chasmDefID);

		// Check if this chasm already has a mapping (i.e. have we seen this chunk before?).
		const auto keyIter = std::find_if(chasmTextureKeys.begin(), chasmTextureKeys.end(),
			[chasmDefID, &chunkPos](const RenderVoxelChunkManager::LoadedChasmTextureKey &loadedKey)
		{
			return (loadedKey.chasmDefID == chasmDefID) && (loadedKey.chunkPos == chunkPos);
		});

		if (keyIter != chasmTextureKeys.end())
		{
			return;
		}

		// Check if any loaded chasm floors reference the same asset(s).
		const auto chasmFloorIter = std::find_if(chasmFloorTextureLists.begin(), chasmFloorTextureLists.end(),
			[&chasmDef](const RenderVoxelChunkManager::LoadedChasmFloorTextureList &textureList)
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
			RenderVoxelChunkManager::LoadedChasmFloorTextureList newTextureList;
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
		const auto chasmWallIter = std::find_if(textures.begin(), textures.end(),
			[&chasmDef](const RenderVoxelChunkManager::LoadedTexture &texture)
		{
			return texture.textureAsset == chasmDef.wallTextureAsset;
		});

		DebugAssert(chasmWallIter != textures.end());
		const int chasmWallIndex = static_cast<int>(std::distance(textures.begin(), chasmWallIter));

		DebugAssert(chasmFloorListIndex >= 0);
		DebugAssert(chasmWallIndex >= 0);

		RenderVoxelChunkManager::LoadedChasmTextureKey key;
		key.init(chunkPos, chasmDefID, chasmFloorListIndex, chasmWallIndex);
		chasmTextureKeys.emplace_back(std::move(key));
	}

	RenderTransform MakeDoorFaceRenderTransform(ArenaTypes::DoorType doorType, Radians faceBaseRadians, double animPercent, double ceilingScale)
	{
		RenderTransform renderTransform;

		switch (doorType)
		{
		case ArenaTypes::DoorType::Swinging:
		{
			const Radians rotationRadians = DoorUtils::getSwingingRotationRadians(faceBaseRadians, animPercent);
			renderTransform.preScaleTranslation = Double3::Zero;
			renderTransform.rotation = Matrix4d::yRotation(rotationRadians);
			renderTransform.scale = Matrix4d::identity();
			break;
		}
		case ArenaTypes::DoorType::Sliding:
		{
			const double uMin = DoorUtils::getAnimatedTexCoordPercent(animPercent);
			const double scaleAmount = DoorUtils::getAnimatedScaleAmount(uMin);
			renderTransform.preScaleTranslation = Double3::Zero;
			renderTransform.rotation = Matrix4d::yRotation(faceBaseRadians);
			renderTransform.scale = Matrix4d::scale(1.0, 1.0, scaleAmount);
			break;
		}
		case ArenaTypes::DoorType::Raising:
		{
			const double vMin = DoorUtils::getAnimatedTexCoordPercent(animPercent);
			const double scaleAmount = DoorUtils::getAnimatedScaleAmount(vMin);
			renderTransform.preScaleTranslation = Double3(1.0, -ceilingScale, 1.0);
			renderTransform.rotation = Matrix4d::yRotation(faceBaseRadians);
			renderTransform.scale = Matrix4d::scale(1.0, scaleAmount, 1.0);
			break;
		}
		default:
			DebugNotImplementedMsg(std::to_string(static_cast<int>(doorType)));
			break;
		}

		return renderTransform;
	}
}

void RenderVoxelChunkManager::LoadedTexture::init(const TextureAsset &textureAsset, ScopedObjectTextureRef &&objectTextureRef)
{
	this->textureAsset = textureAsset;
	this->objectTextureRef = std::move(objectTextureRef);
}

RenderVoxelChunkManager::LoadedChasmFloorTextureList::LoadedChasmFloorTextureList()
{
	this->animType = static_cast<ChasmDefinition::AnimationType>(-1);
	this->paletteIndex = 0;
}

void RenderVoxelChunkManager::LoadedChasmFloorTextureList::initColor(uint8_t paletteIndex,
	ScopedObjectTextureRef &&objectTextureRef)
{
	this->animType = ChasmDefinition::AnimationType::SolidColor;
	this->paletteIndex = paletteIndex;
	this->objectTextureRefs.emplace_back(std::move(objectTextureRef));
}

void RenderVoxelChunkManager::LoadedChasmFloorTextureList::initTextured(std::vector<TextureAsset> &&textureAssets,
	std::vector<ScopedObjectTextureRef> &&objectTextureRefs)
{
	this->animType = ChasmDefinition::AnimationType::Animated;
	this->textureAssets = std::move(textureAssets);
	this->objectTextureRefs = std::move(objectTextureRefs);
}

int RenderVoxelChunkManager::LoadedChasmFloorTextureList::getTextureIndex(double chasmAnimPercent) const
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

void RenderVoxelChunkManager::LoadedChasmTextureKey::init(const ChunkInt2 &chunkPos, VoxelChunk::ChasmDefID chasmDefID,
	int chasmFloorListIndex, int chasmWallIndex)
{
	this->chunkPos = chunkPos;
	this->chasmDefID = chasmDefID;
	this->chasmFloorListIndex = chasmFloorListIndex;
	this->chasmWallIndex = chasmWallIndex;
}

RenderVoxelChunkManager::RenderVoxelChunkManager()
{
	this->defaultTransformBufferID = -1;
	this->chasmWallIndexBufferIDs.fill(-1);
}

void RenderVoxelChunkManager::init(Renderer &renderer)
{
	// Populate default voxel transform.
	if (!renderer.tryCreateUniformBuffer(1, sizeof(RenderTransform), alignof(RenderTransform), &this->defaultTransformBufferID))
	{
		DebugLogError("Couldn't create uniform buffer for voxel default transform.");
		return;
	}

	RenderTransform voxelDefaultTransform;
	voxelDefaultTransform.preScaleTranslation = Double3::Zero;
	voxelDefaultTransform.rotation = Matrix4d::identity();
	voxelDefaultTransform.scale = Matrix4d::identity();
	renderer.populateUniformBuffer(this->defaultTransformBufferID, voxelDefaultTransform);

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
}

void RenderVoxelChunkManager::shutdown(Renderer &renderer)
{
	for (int i = static_cast<int>(this->activeChunks.size()) - 1; i >= 0; i--)
	{
		ChunkPtr &chunkPtr = this->activeChunks[i];
		chunkPtr->freeBuffers(renderer);
		this->recycleChunk(i);
	}

	if (this->defaultTransformBufferID >= 0)
	{
		renderer.freeUniformBuffer(this->defaultTransformBufferID);
		this->defaultTransformBufferID = -1;
	}

	for (IndexBufferID &indexBufferID : this->chasmWallIndexBufferIDs)
	{
		renderer.freeIndexBuffer(indexBufferID);
		indexBufferID = -1;
	}

	this->textures.clear();
	this->chasmFloorTextureLists.clear();
	this->chasmTextureKeys.clear();
	this->drawCallsCache.clear();
}

ObjectTextureID RenderVoxelChunkManager::getTextureID(const TextureAsset &textureAsset) const
{
	const auto iter = std::find_if(this->textures.begin(), this->textures.end(),
		[&textureAsset](const LoadedTexture &loadedTexture)
	{
		return loadedTexture.textureAsset == textureAsset;
	});

	DebugAssertMsg(iter != this->textures.end(), "No loaded voxel texture for \"" + textureAsset.filename + "\".");
	const ScopedObjectTextureRef &objectTextureRef = iter->objectTextureRef;
	return objectTextureRef.get();
}

ObjectTextureID RenderVoxelChunkManager::getChasmFloorTextureID(const ChunkInt2 &chunkPos, VoxelChunk::ChasmDefID chasmDefID,
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

ObjectTextureID RenderVoxelChunkManager::getChasmWallTextureID(const ChunkInt2 &chunkPos, VoxelChunk::ChasmDefID chasmDefID) const
{
	const auto keyIter = std::find_if(this->chasmTextureKeys.begin(), this->chasmTextureKeys.end(),
		[&chunkPos, chasmDefID](const LoadedChasmTextureKey &key)
	{
		return (key.chunkPos == chunkPos) && (key.chasmDefID == chasmDefID);
	});

	DebugAssertMsg(keyIter != this->chasmTextureKeys.end(), "No chasm texture key for chasm def ID \"" +
		std::to_string(chasmDefID) + "\" in chunk (" + chunkPos.toString() + ").");

	const int wallIndex = keyIter->chasmWallIndex;
	const LoadedTexture &voxelTexture = this->textures[wallIndex];
	const ScopedObjectTextureRef &objectTextureRef = voxelTexture.objectTextureRef;
	return objectTextureRef.get();
}

BufferView<const RenderDrawCall> RenderVoxelChunkManager::getDrawCalls() const
{
	return BufferView<const RenderDrawCall>(this->drawCallsCache);
}

void RenderVoxelChunkManager::loadTextures(const VoxelChunk &voxelChunk, TextureManager &textureManager, Renderer &renderer)
{
	for (int i = 0; i < voxelChunk.getTextureDefCount(); i++)
	{
		const VoxelTextureDefinition &voxelTextureDef = voxelChunk.getTextureDef(i);
		LoadVoxelDefTextures(voxelTextureDef, this->textures, textureManager, renderer);
	}

	for (int i = 0; i < voxelChunk.getChasmDefCount(); i++)
	{
		const VoxelChunk::ChasmDefID chasmDefID = static_cast<VoxelChunk::ChasmDefID>(i);
		LoadChasmDefTextures(chasmDefID, voxelChunk, this->textures, this->chasmFloorTextureLists, this->chasmTextureKeys, textureManager, renderer);
	}
}

void RenderVoxelChunkManager::loadMeshBuffers(RenderVoxelChunk &renderChunk, const VoxelChunk &voxelChunk, double ceilingScale, Renderer &renderer)
{
	const ChunkInt2 &chunkPos = voxelChunk.getPosition();

	// Add render chunk voxel mesh instances and create mappings to them.
	for (int meshDefIndex = 0; meshDefIndex < voxelChunk.getMeshDefCount(); meshDefIndex++)
	{
		const VoxelChunk::VoxelMeshDefID voxelMeshDefID = static_cast<VoxelChunk::VoxelMeshDefID>(meshDefIndex);
		const VoxelMeshDefinition &voxelMeshDef = voxelChunk.getMeshDef(voxelMeshDefID);

		RenderVoxelMeshInstance renderVoxelMeshInst;
		if (!voxelMeshDef.isEmpty()) // Only attempt to create buffers for non-air voxels.
		{
			constexpr int positionComponentsPerVertex = MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
			constexpr int normalComponentsPerVertex = MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
			constexpr int texCoordComponentsPerVertex = MeshUtils::TEX_COORDS_PER_VERTEX;

			const int vertexCount = voxelMeshDef.rendererVertexCount;
			if (!renderer.tryCreateVertexBuffer(vertexCount, positionComponentsPerVertex, &renderVoxelMeshInst.vertexBufferID))
			{
				DebugLogError("Couldn't create vertex buffer for voxel mesh ID " + std::to_string(voxelMeshDefID) +
					" in chunk (" + voxelChunk.getPosition().toString() + ").");
				continue;
			}

			if (!renderer.tryCreateAttributeBuffer(vertexCount, normalComponentsPerVertex, &renderVoxelMeshInst.normalBufferID))
			{
				DebugLogError("Couldn't create normal attribute buffer for voxel mesh ID " + std::to_string(voxelMeshDefID) +
					" in chunk (" + voxelChunk.getPosition().toString() + ").");
				renderVoxelMeshInst.freeBuffers(renderer);
				continue;
			}

			if (!renderer.tryCreateAttributeBuffer(vertexCount, texCoordComponentsPerVertex, &renderVoxelMeshInst.texCoordBufferID))
			{
				DebugLogError("Couldn't create tex coord attribute buffer for voxel mesh ID " + std::to_string(voxelMeshDefID) +
					" in chunk (" + voxelChunk.getPosition().toString() + ").");
				renderVoxelMeshInst.freeBuffers(renderer);
				continue;
			}

			ArenaMeshUtils::RenderMeshInitCache meshInitCache;

			// Generate mesh geometry and indices for this voxel definition.
			voxelMeshDef.writeRendererGeometryBuffers(ceilingScale, meshInitCache.verticesView, meshInitCache.normalsView, meshInitCache.texCoordsView);
			voxelMeshDef.writeRendererIndexBuffers(meshInitCache.opaqueIndices0View, meshInitCache.opaqueIndices1View,
				meshInitCache.opaqueIndices2View, meshInitCache.alphaTestedIndices0View);

			renderer.populateVertexBuffer(renderVoxelMeshInst.vertexBufferID,
				BufferView<const double>(meshInitCache.vertices.data(), vertexCount * positionComponentsPerVertex));
			renderer.populateAttributeBuffer(renderVoxelMeshInst.normalBufferID,
				BufferView<const double>(meshInitCache.normals.data(), vertexCount * normalComponentsPerVertex));
			renderer.populateAttributeBuffer(renderVoxelMeshInst.texCoordBufferID,
				BufferView<const double>(meshInitCache.texCoords.data(), vertexCount * texCoordComponentsPerVertex));

			const int opaqueIndexBufferCount = voxelMeshDef.opaqueIndicesListCount;
			for (int bufferIndex = 0; bufferIndex < opaqueIndexBufferCount; bufferIndex++)
			{
				const int opaqueIndexCount = voxelMeshDef.getOpaqueIndicesList(bufferIndex).getCount();
				IndexBufferID &opaqueIndexBufferID = renderVoxelMeshInst.opaqueIndexBufferIDs[bufferIndex];
				if (!renderer.tryCreateIndexBuffer(opaqueIndexCount, &opaqueIndexBufferID))
				{
					DebugLogError("Couldn't create opaque index buffer for voxel mesh ID " +
						std::to_string(voxelMeshDefID) + " in chunk (" + voxelChunk.getPosition().toString() + ").");
					renderVoxelMeshInst.freeBuffers(renderer);
					continue;
				}

				renderVoxelMeshInst.opaqueIndexBufferIdCount++;

				const auto &indices = *meshInitCache.opaqueIndicesPtrs[bufferIndex];
				renderer.populateIndexBuffer(opaqueIndexBufferID,
					BufferView<const int32_t>(indices.data(), opaqueIndexCount));
			}

			const bool hasAlphaTestedIndexBuffer = voxelMeshDef.alphaTestedIndicesListCount > 0;
			if (hasAlphaTestedIndexBuffer)
			{
				const int alphaTestedIndexCount = static_cast<int>(voxelMeshDef.alphaTestedIndices.size());
				if (!renderer.tryCreateIndexBuffer(alphaTestedIndexCount, &renderVoxelMeshInst.alphaTestedIndexBufferID))
				{
					DebugLogError("Couldn't create alpha-tested index buffer for voxel mesh ID " +
						std::to_string(voxelMeshDefID) + " in chunk (" + voxelChunk.getPosition().toString() + ").");
					renderVoxelMeshInst.freeBuffers(renderer);
					continue;
				}

				renderer.populateIndexBuffer(renderVoxelMeshInst.alphaTestedIndexBufferID,
					BufferView<const int32_t>(meshInitCache.alphaTestedIndices0.data(), alphaTestedIndexCount));
			}
		}

		const RenderVoxelMeshInstID renderMeshInstID = renderChunk.addMeshInst(std::move(renderVoxelMeshInst));
		renderChunk.meshInstMappings.emplace(voxelMeshDefID, renderMeshInstID);
	}
}

void RenderVoxelChunkManager::loadChasmWall(RenderVoxelChunk &renderChunk, const VoxelChunk &voxelChunk, SNInt x, int y, WEInt z)
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

void RenderVoxelChunkManager::loadChasmWalls(RenderVoxelChunk &renderChunk, const VoxelChunk &voxelChunk)
{
	for (WEInt z = 0; z < Chunk::DEPTH; z++)
	{
		for (int y = 0; y < voxelChunk.getHeight(); y++)
		{
			for (SNInt x = 0; x < Chunk::WIDTH; x++)
			{
				this->loadChasmWall(renderChunk, voxelChunk, x, y, z);
			}
		}
	}
}

void RenderVoxelChunkManager::loadDoorUniformBuffers(RenderVoxelChunk &renderChunk, const VoxelChunk &voxelChunk, double ceilingScale, Renderer &renderer)
{
	for (WEInt z = 0; z < Chunk::DEPTH; z++)
	{
		for (int y = 0; y < voxelChunk.getHeight(); y++)
		{
			for (SNInt x = 0; x < Chunk::WIDTH; x++)
			{
				VoxelChunk::DoorDefID doorDefID;
				if (!voxelChunk.tryGetDoorDefID(x, y, z, &doorDefID))
				{
					continue;
				}

				const DoorDefinition &doorDef = voxelChunk.getDoorDef(doorDefID);
				const ArenaTypes::DoorType doorType = doorDef.getType();
				const VoxelInt3 voxel(x, y, z);
				DebugAssert(renderChunk.doorTransformBuffers.find(voxel) == renderChunk.doorTransformBuffers.end());

				constexpr int faceCount = DoorUtils::FACE_COUNT;

				// Each door voxel has a uniform buffer, one render transform per face.
				UniformBufferID doorTransformBufferID;
				if (!renderer.tryCreateUniformBuffer(faceCount, sizeof(RenderTransform), alignof(RenderTransform), &doorTransformBufferID))
				{
					DebugLogError("Couldn't create uniform buffer for door transform.");
					continue;
				}

				const double doorAnimPercent = DoorUtils::getAnimPercentOrZero(voxel.x, voxel.y, voxel.z, voxelChunk);

				// Initialize to default appearance. Dirty door animations trigger an update.
				for (int i = 0; i < faceCount; i++)
				{
					const Radians faceBaseRadians = DoorUtils::BaseAngles[i];
					const RenderTransform faceRenderTransform = MakeDoorFaceRenderTransform(doorType, faceBaseRadians, doorAnimPercent, ceilingScale);
					renderer.populateUniformAtIndex(doorTransformBufferID, i, faceRenderTransform);
				}

				renderChunk.doorTransformBuffers.emplace(voxel, doorTransformBufferID);
			}
		}
	}
}

void RenderVoxelChunkManager::addDrawCall(RenderVoxelChunk &renderChunk, const VoxelInt3 &voxelPos, const Double3 &worldPosition,
	UniformBufferID transformBufferID, int transformIndex, VertexBufferID vertexBufferID, AttributeBufferID normalBufferID, AttributeBufferID texCoordBufferID,
	IndexBufferID indexBufferID, ObjectTextureID textureID0, const std::optional<ObjectTextureID> &textureID1, TextureSamplingType textureSamplingType0,
	TextureSamplingType textureSamplingType1, RenderLightingType lightingType, double meshLightPercent, BufferView<const RenderLightID> lightIDs,
	VertexShaderType vertexShaderType, PixelShaderType pixelShaderType, double pixelShaderParam0, BufferView3D<RenderVoxelDrawCallRangeID> drawCallRangeIDs)
{
	RenderVoxelDrawCallRangeID drawCallRangeID = drawCallRangeIDs.get(voxelPos.x, voxelPos.y, voxelPos.z);
	BufferView<RenderDrawCall> drawCallsView;
	if (drawCallRangeID >= 0)
	{
		drawCallRangeID = renderChunk.drawCallHeap.addDrawCall(drawCallRangeID);
		drawCallsView = renderChunk.drawCallHeap.get(drawCallRangeID);
	}
	else
	{
		drawCallRangeID = renderChunk.drawCallHeap.alloc(1);
		DebugAssertMsg(drawCallRangeID >= 0, "Couldn't allocate range ID for voxel (" + voxelPos.toString() + ") in chunk (" + renderChunk.getPosition().toString() + ").");
		drawCallRangeIDs.set(voxelPos.x, voxelPos.y, voxelPos.z, drawCallRangeID);

		drawCallsView = renderChunk.drawCallHeap.get(drawCallRangeID);
	}

	RenderDrawCall &drawCall = drawCallsView.get(drawCallsView.getCount() - 1);
	drawCall.position = worldPosition;
	drawCall.transformBufferID = transformBufferID;
	drawCall.transformIndex = transformIndex;
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
}

void RenderVoxelChunkManager::loadDrawCalls(RenderVoxelChunk &renderChunk, const VoxelChunk &voxelChunk,
	const VoxelVisibilityChunk &voxelVisChunk, const RenderLightChunk &renderLightChunk, double ceilingScale,
	double chasmAnimPercent, bool updateStatics, bool updateAnimating)
{
	const ChunkInt2 &chunkPos = renderChunk.getPosition();

	// Generate draw calls for each non-air voxel.
	for (WEInt z = 0; z < renderChunk.meshInstIDs.getDepth(); z++)
	{
		for (SNInt x = 0; x < renderChunk.meshInstIDs.getWidth(); x++)
		{
			const int visibilityLeafNodeIndex = x + (z * renderChunk.meshInstIDs.getWidth());
			DebugAssertIndex(voxelVisChunk.leafNodeFrustumTests, visibilityLeafNodeIndex);
			const bool isVoxelColumnVisible = voxelVisChunk.leafNodeFrustumTests[visibilityLeafNodeIndex];
			if (!isVoxelColumnVisible)
			{
				continue;
			}

			for (int y = 0; y < renderChunk.meshInstIDs.getHeight(); y++)
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

				const auto meshInstIter = renderChunk.meshInstMappings.find(voxelMeshDefID);
				DebugAssert(meshInstIter != renderChunk.meshInstMappings.end());
				const RenderVoxelMeshInstID renderMeshInstID = meshInstIter->second;
				renderChunk.meshInstIDs.set(x, y, z, renderMeshInstID);

				const RenderVoxelMeshInstance &renderMeshInst = renderChunk.meshInsts[renderMeshInstID];

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

				const VoxelFadeAnimationInstance *fadeAnimInst = nullptr;
				bool isFading = false;
				int fadeAnimInstIndex;
				if (voxelChunk.tryGetFadeAnimInstIndex(x, y, z, &fadeAnimInstIndex))
				{
					BufferView<const VoxelFadeAnimationInstance> fadeAnimInsts = voxelChunk.getFadeAnimInsts();
					fadeAnimInst = &fadeAnimInsts[fadeAnimInstIndex];
					isFading = !fadeAnimInst->isDoneFading();
				}

				const RenderVoxelLightIdList &voxelLightIdList = renderLightChunk.voxelLightIdLists.get(x, y, z);

				const bool canAnimate = isDoor || isChasm || isFading;
				if ((!canAnimate && updateStatics) || (canAnimate && updateAnimating))
				{
					for (int bufferIndex = 0; bufferIndex < renderMeshInst.opaqueIndexBufferIdCount; bufferIndex++)
					{
						ObjectTextureID textureID = -1;

						if (!isChasm)
						{
							const int textureAssetIndex = GetVoxelOpaqueTextureAssetIndex(voxelType, bufferIndex);
							const auto voxelTextureIter = std::find_if(this->textures.begin(), this->textures.end(),
								[&voxelTextureDef, textureAssetIndex](const RenderVoxelChunkManager::LoadedTexture &loadedTexture)
							{
								return loadedTexture.textureAsset == voxelTextureDef.getTextureAsset(textureAssetIndex);
							});

							if (voxelTextureIter != this->textures.end())
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

						const IndexBufferID opaqueIndexBufferID = renderMeshInst.opaqueIndexBufferIDs[bufferIndex];
						const UniformBufferID transformBufferID = this->defaultTransformBufferID;
						const int transformIndex = 0;
						const TextureSamplingType textureSamplingType = !isChasm ? TextureSamplingType::Default : TextureSamplingType::ScreenSpaceRepeatY;

						RenderLightingType lightingType = RenderLightingType::PerPixel;
						double meshLightPercent = 0.0;
						if (isFading)
						{
							lightingType = RenderLightingType::PerMesh;
							meshLightPercent = std::clamp(1.0 - fadeAnimInst->percentFaded, 0.0, 1.0);
						}

						BufferView3D<RenderVoxelDrawCallRangeID> drawCallRangeIDsView;
						if (isChasm)
						{
							const ChasmDefinition &chasmDef = voxelChunk.getChasmDef(chasmDefID);
							if (chasmDef.isEmissive)
							{
								lightingType = RenderLightingType::PerMesh;
								meshLightPercent = 1.0;
							}

							drawCallRangeIDsView = renderChunk.chasmDrawCallRangeIDs;
						}
						else if (isFading)
						{
							drawCallRangeIDsView = renderChunk.fadingDrawCallRangeIDs;
						}
						else
						{
							drawCallRangeIDsView = renderChunk.staticDrawCallRangeIDs;
						}

						const PixelShaderType pixelShaderType = PixelShaderType::Opaque;
						const double pixelShaderParam0 = 0.0;
						this->addDrawCall(renderChunk, voxel, worldPos, transformBufferID, transformIndex, renderMeshInst.vertexBufferID,
							renderMeshInst.normalBufferID, renderMeshInst.texCoordBufferID, opaqueIndexBufferID, textureID, std::nullopt,
							textureSamplingType, textureSamplingType, lightingType, meshLightPercent, voxelLightIdList.getLightIDs(),
							VertexShaderType::Voxel, pixelShaderType, pixelShaderParam0, drawCallRangeIDsView);
					}
				}

				if (renderMeshInst.alphaTestedIndexBufferID >= 0)
				{
					if (updateStatics || (updateAnimating && isDoor))
					{
						DebugAssert(!isChasm);
						ObjectTextureID textureID = -1;

						const int textureAssetIndex = GetVoxelAlphaTestedTextureAssetIndex(voxelType);
						const auto voxelTextureIter = std::find_if(this->textures.begin(), this->textures.end(),
							[&voxelTextureDef, textureAssetIndex](const RenderVoxelChunkManager::LoadedTexture &loadedTexture)
						{
							return loadedTexture.textureAsset == voxelTextureDef.getTextureAsset(textureAssetIndex);
						});

						if (voxelTextureIter != this->textures.end())
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

							const auto doorTransformIter = renderChunk.doorTransformBuffers.find(voxel);
							DebugAssert(doorTransformIter != renderChunk.doorTransformBuffers.end());
							UniformBufferID doorTransformBufferID = doorTransformIter->second;

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
								for (int i = 0; i < DoorUtils::FACE_COUNT; i++)
								{
									if (!visibleDoorFaces[i])
									{
										continue;
									}

									const Double3 &doorHingeOffset = DoorUtils::SwingingHingeOffsets[i];
									const Double3 doorHingePosition = worldPos + doorHingeOffset;
									const int doorTransformIndex = i;
									const TextureSamplingType textureSamplingType = TextureSamplingType::Default;
									constexpr double meshLightPercent = 0.0;
									constexpr double pixelShaderParam0 = 0.0;
									this->addDrawCall(renderChunk, voxel, doorHingePosition, doorTransformBufferID, doorTransformIndex, renderMeshInst.vertexBufferID,
										renderMeshInst.normalBufferID, renderMeshInst.texCoordBufferID, renderMeshInst.alphaTestedIndexBufferID, textureID,
										std::nullopt, textureSamplingType, textureSamplingType, RenderLightingType::PerPixel, meshLightPercent,
										voxelLightIdList.getLightIDs(), VertexShaderType::SwingingDoor, PixelShaderType::AlphaTested, pixelShaderParam0,
										renderChunk.doorDrawCallRangeIDs);
								}

								break;
							}
							case ArenaTypes::DoorType::Sliding:
							{
								const double uMin = DoorUtils::getAnimatedTexCoordPercent(doorAnimPercent);

								for (int i = 0; i < DoorUtils::FACE_COUNT; i++)
								{
									if (!visibleDoorFaces[i])
									{
										continue;
									}

									const Double3 &doorHingeOffset = DoorUtils::SwingingHingeOffsets[i];
									const Double3 doorHingePosition = worldPos + doorHingeOffset;
									const int doorTransformIndex = i;
									const TextureSamplingType textureSamplingType = TextureSamplingType::Default;
									constexpr double meshLightPercent = 0.0;
									const double pixelShaderParam0 = uMin;
									this->addDrawCall(renderChunk, voxel, doorHingePosition, doorTransformBufferID, doorTransformIndex, renderMeshInst.vertexBufferID,
										renderMeshInst.normalBufferID, renderMeshInst.texCoordBufferID, renderMeshInst.alphaTestedIndexBufferID, textureID,
										std::nullopt, textureSamplingType, textureSamplingType, RenderLightingType::PerPixel, meshLightPercent,
										voxelLightIdList.getLightIDs(), VertexShaderType::SlidingDoor, PixelShaderType::AlphaTestedWithVariableTexCoordUMin,
										pixelShaderParam0, renderChunk.doorDrawCallRangeIDs);
								}

								break;
							}
							case ArenaTypes::DoorType::Raising:
							{
								const double vMin = DoorUtils::getAnimatedTexCoordPercent(doorAnimPercent);

								for (int i = 0; i < DoorUtils::FACE_COUNT; i++)
								{
									if (!visibleDoorFaces[i])
									{
										continue;
									}

									const Double3 &doorHingeOffset = DoorUtils::SwingingHingeOffsets[i];
									const Double3 doorHingePosition = worldPos + doorHingeOffset;
									const int doorTransformIndex = i;
									const TextureSamplingType textureSamplingType = TextureSamplingType::Default;
									constexpr double meshLightPercent = 0.0;
									const double pixelShaderParam0 = vMin;
									this->addDrawCall(renderChunk, voxel, doorHingePosition, doorTransformBufferID, doorTransformIndex, renderMeshInst.vertexBufferID,
										renderMeshInst.normalBufferID, renderMeshInst.texCoordBufferID, renderMeshInst.alphaTestedIndexBufferID, textureID,
										std::nullopt, textureSamplingType, textureSamplingType, RenderLightingType::PerPixel, meshLightPercent,
										voxelLightIdList.getLightIDs(), VertexShaderType::RaisingDoor, PixelShaderType::AlphaTestedWithVariableTexCoordVMin,
										pixelShaderParam0, renderChunk.doorDrawCallRangeIDs);
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
							const UniformBufferID transformBufferID = this->defaultTransformBufferID;
							const int transformIndex = 0;
							const TextureSamplingType textureSamplingType = TextureSamplingType::Default;

							RenderLightingType lightingType = RenderLightingType::PerPixel;
							double meshLightPercent = 0.0;
							BufferView3D<RenderVoxelDrawCallRangeID> drawCallRangeIDsView = renderChunk.staticDrawCallRangeIDs;
							if (isFading)
							{
								lightingType = RenderLightingType::PerMesh;
								meshLightPercent = std::clamp(1.0 - fadeAnimInst->percentFaded, 0.0, 1.0);
								drawCallRangeIDsView = renderChunk.fadingDrawCallRangeIDs;
							}

							constexpr double pixelShaderParam0 = 0.0;
							this->addDrawCall(renderChunk, voxel, worldPos, transformBufferID, transformIndex, renderMeshInst.vertexBufferID,
								renderMeshInst.normalBufferID, renderMeshInst.texCoordBufferID, renderMeshInst.alphaTestedIndexBufferID,
								textureID, std::nullopt, textureSamplingType, textureSamplingType, lightingType, meshLightPercent,
								voxelLightIdList.getLightIDs(), VertexShaderType::Voxel, PixelShaderType::AlphaTested, pixelShaderParam0, drawCallRangeIDsView);
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

						const UniformBufferID transformBufferID = this->defaultTransformBufferID;
						const int transformIndex = 0;
						const TextureSamplingType textureSamplingType = isAnimatingChasm ? TextureSamplingType::ScreenSpaceRepeatY : TextureSamplingType::Default;

						double meshLightPercent = 0.0;
						RenderLightingType lightingType = RenderLightingType::PerPixel;
						if (RendererUtils::isChasmEmissive(chasmType))
						{
							meshLightPercent = 1.0;
							lightingType = RenderLightingType::PerMesh;
						}

						constexpr double pixelShaderParam0 = 0.0;
						this->addDrawCall(renderChunk, voxel, worldPos, transformBufferID, transformIndex, renderMeshInst.vertexBufferID,
							renderMeshInst.normalBufferID, renderMeshInst.texCoordBufferID, chasmWallIndexBufferID, textureID0, textureID1,
							textureSamplingType, textureSamplingType, lightingType, meshLightPercent, voxelLightIdList.getLightIDs(),
							VertexShaderType::Voxel, PixelShaderType::OpaqueWithAlphaTestLayer, pixelShaderParam0, renderChunk.chasmDrawCallRangeIDs);
					}
				}
			}
		}
	}
}

void RenderVoxelChunkManager::rebuildChunkDrawCalls(RenderVoxelChunk &renderChunk, const VoxelChunk &voxelChunk,
	const VoxelVisibilityChunk &voxelVisChunk, const RenderLightChunk &renderLightChunk, double ceilingScale,
	double chasmAnimPercent, bool updateStatics, bool updateAnimating)
{
	if (updateStatics)
	{
		renderChunk.freeStaticDrawCalls();
	}

	if (updateAnimating)
	{
		renderChunk.freeAnimatingDrawCalls();
	}

	this->loadDrawCalls(renderChunk, voxelChunk, voxelVisChunk, renderLightChunk, ceilingScale, chasmAnimPercent, updateStatics, updateAnimating);
}

void RenderVoxelChunkManager::rebuildDrawCallsList(const VoxelVisibilityChunkManager &voxelVisChunkManager)
{
	this->drawCallsCache.clear();

	auto addValidDrawCalls = [this](const RenderVoxelChunk &renderChunk, const VoxelVisibilityChunk &voxelVisChunk, BufferView3D<const RenderVoxelDrawCallRangeID> rangeIDs)
	{
		for (WEInt z = 0; z < rangeIDs.getDepth(); z++)
		{
			for (SNInt x = 0; x < rangeIDs.getWidth(); x++)
			{
				const int visibilityLeafNodeIndex = x + (z * rangeIDs.getWidth());
				DebugAssertIndex(voxelVisChunk.leafNodeFrustumTests, visibilityLeafNodeIndex);
				const bool isVoxelColumnVisible = voxelVisChunk.leafNodeFrustumTests[visibilityLeafNodeIndex];
				if (isVoxelColumnVisible)
				{
					for (int y = 0; y < rangeIDs.getHeight(); y++)
					{
						const RenderVoxelDrawCallRangeID rangeID = rangeIDs.get(x, y, z);
						if (rangeID >= 0)
						{
							const BufferView<const RenderDrawCall> drawCalls = renderChunk.drawCallHeap.get(rangeID);
							this->drawCallsCache.insert(this->drawCallsCache.end(), drawCalls.begin(), drawCalls.end());
						}
					}
				}
			}
		}
	};

	// @todo: eventually this should sort by distance from a CoordDouble2
	for (int i = 0; i < static_cast<int>(this->activeChunks.size()); i++)
	{
		const ChunkPtr &chunkPtr = this->activeChunks[i];
		const RenderVoxelChunk &renderChunk = *chunkPtr;
		const VoxelVisibilityChunk &voxelVisChunk = voxelVisChunkManager.getChunkAtIndex(i);
		addValidDrawCalls(renderChunk, voxelVisChunk, renderChunk.staticDrawCallRangeIDs);
		addValidDrawCalls(renderChunk, voxelVisChunk, renderChunk.doorDrawCallRangeIDs);
		addValidDrawCalls(renderChunk, voxelVisChunk, renderChunk.chasmDrawCallRangeIDs);
		addValidDrawCalls(renderChunk, voxelVisChunk, renderChunk.fadingDrawCallRangeIDs);
	}
}

void RenderVoxelChunkManager::updateActiveChunks(BufferView<const ChunkInt2> newChunkPositions, BufferView<const ChunkInt2> freedChunkPositions,
	const VoxelChunkManager &voxelChunkManager, Renderer &renderer)
{
	for (const ChunkInt2 &chunkPos : freedChunkPositions)
	{
		const int chunkIndex = this->getChunkIndex(chunkPos);
		RenderVoxelChunk &renderChunk = this->getChunkAtIndex(chunkIndex);
		renderChunk.freeBuffers(renderer);
		this->recycleChunk(chunkIndex);
	}

	for (const ChunkInt2 &chunkPos : newChunkPositions)
	{
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);

		const int spawnIndex = this->spawnChunk();
		RenderVoxelChunk &renderChunk = this->getChunkAtIndex(spawnIndex);
		renderChunk.init(chunkPos, voxelChunk.getHeight());
	}

	// Free any unneeded chunks for memory savings in case the chunk distance was once large
	// and is now small. This is significant even for chunk distance 2->1, or 25->9 chunks.
	this->chunkPool.clear();
}

void RenderVoxelChunkManager::update(BufferView<const ChunkInt2> activeChunkPositions, BufferView<const ChunkInt2> newChunkPositions,
	double ceilingScale, double chasmAnimPercent, const VoxelChunkManager &voxelChunkManager,
	const VoxelVisibilityChunkManager &voxelVisChunkManager, const RenderLightChunkManager &renderLightChunkManager,
	TextureManager &textureManager, Renderer &renderer)
{
	for (const ChunkInt2 &chunkPos : newChunkPositions)
	{
		RenderVoxelChunk &renderChunk = this->getChunkAtPosition(chunkPos);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		const VoxelVisibilityChunk &voxelVisChunk = voxelVisChunkManager.getChunkAtPosition(chunkPos);
		const RenderLightChunk &renderLightChunk = renderLightChunkManager.getChunkAtPosition(chunkPos);
		this->loadMeshBuffers(renderChunk, voxelChunk, ceilingScale, renderer);
		this->loadTextures(voxelChunk, textureManager, renderer);
		this->loadChasmWalls(renderChunk, voxelChunk);
		this->loadDoorUniformBuffers(renderChunk, voxelChunk, ceilingScale, renderer);
		this->rebuildChunkDrawCalls(renderChunk, voxelChunk, voxelVisChunk, renderLightChunk, ceilingScale, chasmAnimPercent, true, false);
	}

	for (const ChunkInt2 &chunkPos : activeChunkPositions)
	{
		RenderVoxelChunk &renderChunk = this->getChunkAtPosition(chunkPos);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		const VoxelVisibilityChunk &voxelVisChunk = voxelVisChunkManager.getChunkAtPosition(chunkPos);
		const RenderLightChunk &renderLightChunk = renderLightChunkManager.getChunkAtPosition(chunkPos);

		BufferView<const VoxelInt3> dirtyChasmWallInstPositions = voxelChunk.getDirtyChasmWallInstPositions();
		for (const VoxelInt3 &chasmWallPos : dirtyChasmWallInstPositions)
		{
			this->loadChasmWall(renderChunk, voxelChunk, chasmWallPos.x, chasmWallPos.y, chasmWallPos.z);
		}

		// Update door render transforms (rotation angle, etc.).
		BufferView<const VoxelInt3> dirtyDoorAnimPositions = voxelChunk.getDirtyDoorAnimInstPositions();
		for (const VoxelInt3 &doorVoxel : dirtyDoorAnimPositions)
		{
			VoxelChunk::DoorDefID doorDefID;
			if (!voxelChunk.tryGetDoorDefID(doorVoxel.x, doorVoxel.y, doorVoxel.z, &doorDefID))
			{
				DebugLogError("Expected door def ID at (" + doorVoxel.toString() + ").");
				continue;
			}

			const DoorDefinition &doorDef = voxelChunk.getDoorDef(doorDefID);
			const ArenaTypes::DoorType doorType = doorDef.getType();
			const double doorAnimPercent = DoorUtils::getAnimPercentOrZero(doorVoxel.x, doorVoxel.y, doorVoxel.z, voxelChunk);

			for (int i = 0; i < DoorUtils::FACE_COUNT; i++)
			{
				const Radians faceBaseRadians = DoorUtils::BaseAngles[i];
				const RenderTransform faceRenderTransform = MakeDoorFaceRenderTransform(doorType, faceBaseRadians, doorAnimPercent, ceilingScale);

				const auto doorTransformIter = renderChunk.doorTransformBuffers.find(doorVoxel);
				DebugAssert(doorTransformIter != renderChunk.doorTransformBuffers.end());
				const UniformBufferID doorTransformBufferID = doorTransformIter->second;
				renderer.populateUniformAtIndex(doorTransformBufferID, i, faceRenderTransform);
			}
		}

		BufferView<const VoxelInt3> dirtyMeshDefPositions = voxelChunk.getDirtyMeshDefPositions();
		BufferView<const VoxelInt3> dirtyFadeAnimInstPositions = voxelChunk.getDirtyFadeAnimInstPositions();
		BufferView<const VoxelInt3> dirtyLightPositions = renderLightChunk.dirtyLightPositions;
		bool updateStatics = dirtyMeshDefPositions.getCount() > 0;
		updateStatics |= dirtyFadeAnimInstPositions.getCount() > 0; // @temp fix for fading voxels being covered by their non-fading draw call
		updateStatics |= dirtyLightPositions.getCount() > 0; // @temp fix for player light movement, eventually other moving lights too
		updateStatics |= true; // @temp fix for draw calls not regenerating in chunks the player is not in (probably because nothing static in them is being marked dirty)
		this->rebuildChunkDrawCalls(renderChunk, voxelChunk, voxelVisChunk, renderLightChunk, ceilingScale, chasmAnimPercent, updateStatics, true);
	}

	// @todo: only rebuild if needed; currently we assume that all scenes in the game have some kind of animating chasms/etc., which is inefficient
	//if ((freedChunkCount > 0) || (newChunkCount > 0))
	{
		this->rebuildDrawCallsList(voxelVisChunkManager);
	}
}

void RenderVoxelChunkManager::cleanUp()
{
	
}

void RenderVoxelChunkManager::unloadScene(Renderer &renderer)
{
	this->textures.clear();
	this->chasmFloorTextureLists.clear();
	this->chasmTextureKeys.clear();

	// Free vertex/attribute/index buffer IDs.
	for (int i = static_cast<int>(this->activeChunks.size()) - 1; i >= 0; i--)
	{
		ChunkPtr &chunkPtr = this->activeChunks[i];
		chunkPtr->freeBuffers(renderer);
		this->recycleChunk(i);
	}

	this->drawCallsCache.clear();
}
