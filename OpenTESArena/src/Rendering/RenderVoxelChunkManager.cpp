#include <algorithm>
#include <array>
#include <numeric>
#include <optional>

#include "RenderCommandBuffer.h"
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
	constexpr int DefaultVoxelTextureInitInfoIndices[] = { 0, 1, 2, 3 };
	constexpr int RaisedVoxelTextureInitInfoIndices[] = { 1, 2, 0, -1 };

	int GetVoxelRenderTransformIndex(SNInt x, int y, WEInt z, int chunkHeight)
	{
		return x + (y * Chunk::WIDTH) + (z * Chunk::WIDTH * chunkHeight);
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

	WorldDouble3 MakeVoxelWorldPosition(const ChunkInt2 &chunkPos, const VoxelInt3 &voxel, double ceilingScale)
	{
		const WorldInt3 worldVoxel = VoxelUtils::chunkVoxelToWorldVoxel(chunkPos, voxel);
		return WorldDouble3(
			static_cast<SNDouble>(worldVoxel.x),
			static_cast<double>(worldVoxel.y) * ceilingScale,
			static_cast<WEDouble>(worldVoxel.z));
	}

	RenderTransform MakeDoorFaceRenderTransform(ArenaTypes::DoorType doorType, int doorFaceIndex, const WorldDouble3 &worldPosition, double animPercent)
	{
		const Radians faceBaseRadians = DoorUtils::BaseAngles[doorFaceIndex];
		const Double3 &hingeOffset = DoorUtils::SwingingHingeOffsets[doorFaceIndex];
		const Double3 hingePosition = worldPosition + hingeOffset;

		RenderTransform renderTransform;
		switch (doorType)
		{
		case ArenaTypes::DoorType::Swinging:
		{
			const Radians rotationRadians = DoorUtils::getSwingingRotationRadians(faceBaseRadians, animPercent);
			renderTransform.translation = Matrix4d::translation(hingePosition.x, hingePosition.y, hingePosition.z);
			renderTransform.rotation = Matrix4d::yRotation(rotationRadians);
			renderTransform.scale = Matrix4d::identity();
			break;
		}
		case ArenaTypes::DoorType::Sliding:
		{
			const double uMin = DoorUtils::getAnimatedTexCoordPercent(animPercent);
			const double scaleAmount = DoorUtils::getAnimatedScaleAmount(uMin);
			renderTransform.translation = Matrix4d::translation(hingePosition.x, hingePosition.y, hingePosition.z);
			renderTransform.rotation = Matrix4d::yRotation(faceBaseRadians);
			renderTransform.scale = Matrix4d::scale(1.0, 1.0, scaleAmount);
			break;
		}
		case ArenaTypes::DoorType::Raising:
		{
			const double vMin = DoorUtils::getAnimatedTexCoordPercent(animPercent);
			const double scaleAmount = DoorUtils::getAnimatedScaleAmount(vMin);
			renderTransform.translation = Matrix4d::translation(hingePosition.x, hingePosition.y, hingePosition.z);
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

	Double3 MakeRaisingDoorPreScaleTranslation(double ceilingScale)
	{
		return Double3(0.0, -ceilingScale, 0.0);
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
	this->raisingDoorPreScaleTranslationBufferID = -1;
	this->chasmWallIndexBufferIDs.fill(-1);
}

void RenderVoxelChunkManager::init(Renderer &renderer)
{
	// Populate pre-scale translation transform (for raising doors).
	if (!renderer.tryCreateUniformBuffer(1, sizeof(Double3), alignof(Double3), &this->raisingDoorPreScaleTranslationBufferID))
	{
		DebugLogError("Couldn't create uniform buffer for pre-scale translation.");
		return;
	}

	const Double3 preScaleTranslation = Double3::Zero; // Populated on scene change.
	renderer.populateUniformBuffer(this->raisingDoorPreScaleTranslationBufferID, preScaleTranslation);

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

	if (this->raisingDoorPreScaleTranslationBufferID >= 0)
	{
		renderer.freeUniformBuffer(this->raisingDoorPreScaleTranslationBufferID);
		this->raisingDoorPreScaleTranslationBufferID = -1;
	}

	for (IndexBufferID &indexBufferID : this->chasmWallIndexBufferIDs)
	{
		if (indexBufferID >= 0)
		{
			renderer.freeIndexBuffer(indexBufferID);
			indexBufferID = -1;
		}
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
	for (int shapeDefIndex = 0; shapeDefIndex < voxelChunk.getShapeDefCount(); shapeDefIndex++)
	{
		const VoxelChunk::VoxelShapeDefID voxelShapeDefID = static_cast<VoxelChunk::VoxelShapeDefID>(shapeDefIndex);
		const VoxelShapeDefinition &voxelShapeDef = voxelChunk.getShapeDef(voxelShapeDefID);
		const VoxelMeshDefinition &voxelMeshDef = voxelShapeDef.mesh;
		const bool isRenderMeshValid = !voxelMeshDef.isEmpty(); // Air has a shape for trigger voxels but no mesh

		RenderVoxelMeshInstance renderVoxelMeshInst;
		if (isRenderMeshValid)
		{
			constexpr int positionComponentsPerVertex = MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
			constexpr int normalComponentsPerVertex = MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
			constexpr int texCoordComponentsPerVertex = MeshUtils::TEX_COORDS_PER_VERTEX;

			const int vertexCount = voxelMeshDef.rendererVertexCount;
			if (!renderer.tryCreateVertexBuffer(vertexCount, positionComponentsPerVertex, &renderVoxelMeshInst.vertexBufferID))
			{
				DebugLogError("Couldn't create vertex buffer for voxel shape def ID " + std::to_string(voxelShapeDefID) +
					" in chunk (" + voxelChunk.getPosition().toString() + ").");
				continue;
			}

			if (!renderer.tryCreateAttributeBuffer(vertexCount, normalComponentsPerVertex, &renderVoxelMeshInst.normalBufferID))
			{
				DebugLogError("Couldn't create normal attribute buffer for voxel shape def ID " + std::to_string(voxelShapeDefID) +
					" in chunk (" + voxelChunk.getPosition().toString() + ").");
				renderVoxelMeshInst.freeBuffers(renderer);
				continue;
			}

			if (!renderer.tryCreateAttributeBuffer(vertexCount, texCoordComponentsPerVertex, &renderVoxelMeshInst.texCoordBufferID))
			{
				DebugLogError("Couldn't create tex coord attribute buffer for voxel shape def ID " + std::to_string(voxelShapeDefID) +
					" in chunk (" + voxelChunk.getPosition().toString() + ").");
				renderVoxelMeshInst.freeBuffers(renderer);
				continue;
			}

			ArenaMeshUtils::ShapeInitCache shapeInitCache;

			// Generate mesh geometry and indices for this voxel definition.
			voxelMeshDef.writeRendererGeometryBuffers(voxelShapeDef.scaleType, ceilingScale, shapeInitCache.verticesView, shapeInitCache.normalsView, shapeInitCache.texCoordsView);
			voxelMeshDef.writeRendererIndexBuffers(shapeInitCache.opaqueIndices0View, shapeInitCache.opaqueIndices1View,
				shapeInitCache.opaqueIndices2View, shapeInitCache.alphaTestedIndices0View);

			renderer.populateVertexBuffer(renderVoxelMeshInst.vertexBufferID,
				BufferView<const double>(shapeInitCache.vertices.data(), vertexCount * positionComponentsPerVertex));
			renderer.populateAttributeBuffer(renderVoxelMeshInst.normalBufferID,
				BufferView<const double>(shapeInitCache.normals.data(), vertexCount * normalComponentsPerVertex));
			renderer.populateAttributeBuffer(renderVoxelMeshInst.texCoordBufferID,
				BufferView<const double>(shapeInitCache.texCoords.data(), vertexCount * texCoordComponentsPerVertex));

			const int opaqueIndexBufferCount = voxelMeshDef.opaqueIndicesListCount;
			for (int bufferIndex = 0; bufferIndex < opaqueIndexBufferCount; bufferIndex++)
			{
				const int opaqueIndexCount = voxelMeshDef.getOpaqueIndicesList(bufferIndex).getCount();
				IndexBufferID &opaqueIndexBufferID = renderVoxelMeshInst.opaqueIndexBufferIDs[bufferIndex];
				if (!renderer.tryCreateIndexBuffer(opaqueIndexCount, &opaqueIndexBufferID))
				{
					DebugLogError("Couldn't create opaque index buffer for voxel shape def ID " + std::to_string(voxelShapeDefID) +
						" in chunk (" + voxelChunk.getPosition().toString() + ").");
					renderVoxelMeshInst.freeBuffers(renderer);
					continue;
				}

				renderVoxelMeshInst.opaqueIndexBufferIdCount++;

				const auto &indices = *shapeInitCache.opaqueIndicesPtrs[bufferIndex];
				renderer.populateIndexBuffer(opaqueIndexBufferID, BufferView<const int32_t>(indices.data(), opaqueIndexCount));
			}

			const bool hasAlphaTestedIndexBuffer = voxelMeshDef.alphaTestedIndicesListCount > 0;
			if (hasAlphaTestedIndexBuffer)
			{
				const int alphaTestedIndexCount = static_cast<int>(voxelMeshDef.alphaTestedIndices.size());
				if (!renderer.tryCreateIndexBuffer(alphaTestedIndexCount, &renderVoxelMeshInst.alphaTestedIndexBufferID))
				{
					DebugLogError("Couldn't create alpha-tested index buffer for voxel shape def ID " + std::to_string(voxelShapeDefID) +
						" in chunk (" + voxelChunk.getPosition().toString() + ").");
					renderVoxelMeshInst.freeBuffers(renderer);
					continue;
				}

				renderer.populateIndexBuffer(renderVoxelMeshInst.alphaTestedIndexBufferID,
					BufferView<const int32_t>(shapeInitCache.alphaTestedIndices0.data(), alphaTestedIndexCount));
			}
		}

		const RenderVoxelMeshInstID renderMeshInstID = renderChunk.addMeshInst(std::move(renderVoxelMeshInst));
		renderChunk.meshInstMappings.emplace(voxelShapeDefID, renderMeshInstID);
	}
}

void RenderVoxelChunkManager::loadChasmWall(RenderVoxelChunk &renderChunk, const VoxelChunk &voxelChunk, SNInt x, int y, WEInt z)
{
	const VoxelInt3 voxel(x, y, z);
	auto &chasmWallIndexBufferIDsMap = renderChunk.chasmWallIndexBufferIDsMap;

	int chasmWallInstIndex;
	if (voxelChunk.tryGetChasmWallInstIndex(x, y, z, &chasmWallInstIndex))
	{
		BufferView<const VoxelChasmWallInstance> chasmWallInsts = voxelChunk.getChasmWallInsts();
		const VoxelChasmWallInstance &chasmWallInst = chasmWallInsts[chasmWallInstIndex];
		DebugAssert(chasmWallInst.getFaceCount() > 0);

		const int chasmWallIndexBufferIndex = ArenaMeshUtils::GetChasmWallIndex(
			chasmWallInst.north, chasmWallInst.east, chasmWallInst.south, chasmWallInst.west);
		const IndexBufferID indexBufferID = this->chasmWallIndexBufferIDs[chasmWallIndexBufferIndex];

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
	else
	{
		// Clear index buffer mapping if this chasm wall was removed.
		const auto iter = chasmWallIndexBufferIDsMap.find(voxel);
		if (iter != chasmWallIndexBufferIDsMap.end())
		{
			chasmWallIndexBufferIDsMap.erase(iter);
		}
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

void RenderVoxelChunkManager::loadTransforms(RenderVoxelChunk &renderChunk, const VoxelChunk &voxelChunk, double ceilingScale, Renderer &renderer)
{
	const int chunkHeight = voxelChunk.getHeight();

	// Allocate one large uniform buffer that covers all voxels. Air is wasted and doors are double-allocated but this
	// is much faster than one buffer per voxel.
	const int chunkTransformsCount = Chunk::WIDTH * chunkHeight * Chunk::DEPTH;
	UniformBufferID chunkTransformsBufferID;
	if (!renderer.tryCreateUniformBuffer(chunkTransformsCount, sizeof(RenderTransform), alignof(RenderTransform), &chunkTransformsBufferID))
	{
		DebugLogError("Couldn't create uniform buffer for voxel transforms.");
		return;
	}

	renderChunk.transformBufferID = chunkTransformsBufferID;

	for (WEInt z = 0; z < Chunk::DEPTH; z++)
	{
		for (int y = 0; y < chunkHeight; y++)
		{
			for (SNInt x = 0; x < Chunk::WIDTH; x++)
			{
				const VoxelInt3 voxel(x, y, z);
				const WorldDouble3 worldPosition = MakeVoxelWorldPosition(voxelChunk.getPosition(), voxel, ceilingScale);

				VoxelChunk::DoorDefID doorDefID;
				if (voxelChunk.tryGetDoorDefID(x, y, z, &doorDefID))
				{
					// Door transform uniform buffers. These are separate because each voxel has a RenderTransform per door face.
					const DoorDefinition &doorDef = voxelChunk.getDoorDef(doorDefID);
					const ArenaTypes::DoorType doorType = doorDef.getType();
					DebugAssert(renderChunk.doorTransformBuffers.find(voxel) == renderChunk.doorTransformBuffers.end());

					constexpr int doorFaceCount = DoorUtils::FACE_COUNT;

					// Each door voxel has a uniform buffer, one render transform per face.
					UniformBufferID doorTransformBufferID;
					if (!renderer.tryCreateUniformBuffer(doorFaceCount, sizeof(RenderTransform), alignof(RenderTransform), &doorTransformBufferID))
					{
						DebugLogError("Couldn't create uniform buffer for door transform.");
						continue;
					}

					const double doorAnimPercent = DoorUtils::getAnimPercentOrZero(voxel.x, voxel.y, voxel.z, voxelChunk);

					// Initialize to default appearance. Dirty door animations trigger an update.
					for (int i = 0; i < doorFaceCount; i++)
					{
						const RenderTransform faceRenderTransform = MakeDoorFaceRenderTransform(doorType, i, worldPosition, doorAnimPercent);
						renderer.populateUniformAtIndex(doorTransformBufferID, i, faceRenderTransform);
					}

					renderChunk.doorTransformBuffers.emplace(voxel, doorTransformBufferID);
				}
				else
				{
					const int chunkTransformsBufferIndex = GetVoxelRenderTransformIndex(x, y, z, chunkHeight);

					RenderTransform renderTransform;
					renderTransform.translation = Matrix4d::translation(worldPosition.x, worldPosition.y, worldPosition.z);
					renderTransform.rotation = Matrix4d::identity();
					renderTransform.scale = Matrix4d::identity();
					renderer.populateUniformAtIndex(chunkTransformsBufferID, chunkTransformsBufferIndex, renderTransform);
				}
			}
		}
	}
}

void RenderVoxelChunkManager::updateChunkDrawCalls(RenderVoxelChunk &renderChunk, BufferView<const VoxelInt3> dirtyVoxelPositions,
	const VoxelChunk &voxelChunk, const RenderLightChunk &renderLightChunk, double ceilingScale, double chasmAnimPercent)
{
	const ChunkInt2 &chunkPos = renderChunk.getPosition();
	RenderVoxelDrawCallHeap &drawCallHeap = renderChunk.drawCallHeap;

	// Regenerate all draw calls in the given dirty voxels.
	for (const VoxelInt3 &voxel : dirtyVoxelPositions)
	{
		renderChunk.freeDrawCalls(voxel.x, voxel.y, voxel.z);

		const VoxelChunk::VoxelShapeDefID voxelShapeDefID = voxelChunk.getShapeDefID(voxel.x, voxel.y, voxel.z);
		const VoxelShapeDefinition &voxelShapeDef = voxelChunk.getShapeDef(voxelShapeDefID);
		const VoxelMeshDefinition &voxelMeshDef = voxelShapeDef.mesh;
		if (voxelMeshDef.isEmpty())
		{
			continue;
		}

		const VoxelChunk::VoxelTextureDefID voxelTextureDefID = voxelChunk.getTextureDefID(voxel.x, voxel.y, voxel.z);
		const VoxelChunk::VoxelTraitsDefID voxelTraitsDefID = voxelChunk.getTraitsDefID(voxel.x, voxel.y, voxel.z);
		const VoxelTextureDefinition &voxelTextureDef = voxelChunk.getTextureDef(voxelTextureDefID);
		const VoxelTraitsDefinition &voxelTraitsDef = voxelChunk.getTraitsDef(voxelTraitsDefID);
		const ArenaTypes::VoxelType voxelType = voxelTraitsDef.type;

		const auto meshInstIter = renderChunk.meshInstMappings.find(voxelShapeDefID);
		DebugAssert(meshInstIter != renderChunk.meshInstMappings.end());
		const RenderVoxelMeshInstID renderMeshInstID = meshInstIter->second;
		renderChunk.meshInstIDs.set(voxel.x, voxel.y, voxel.z, renderMeshInstID);
		DebugAssertIndex(renderChunk.meshInsts, renderMeshInstID);
		const RenderVoxelMeshInstance &renderMeshInst = renderChunk.meshInsts[renderMeshInstID];

		VoxelChunk::DoorDefID doorDefID;
		const bool isDoor = voxelChunk.tryGetDoorDefID(voxel.x, voxel.y, voxel.z, &doorDefID);
		const DoorDefinition *doorDef = isDoor ? &voxelChunk.getDoorDef(doorDefID) : nullptr;

		double doorAnimPercent = 0.0;
		if (isDoor)
		{
			int doorAnimInstIndex;
			if (voxelChunk.tryGetDoorAnimInstIndex(voxel.x, voxel.y, voxel.z, &doorAnimInstIndex))
			{
				BufferView<const VoxelDoorAnimationInstance> doorAnimInsts = voxelChunk.getDoorAnimInsts();
				const VoxelDoorAnimationInstance &doorAnimInst = doorAnimInsts[doorAnimInstIndex];
				doorAnimPercent = doorAnimInst.percentOpen;
			}
		}

		VoxelChunk::ChasmDefID chasmDefID;
		const bool isChasm = voxelChunk.tryGetChasmDefID(voxel.x, voxel.y, voxel.z, &chasmDefID);

		const ChasmDefinition *chasmDef = nullptr;
		bool isAnimatingChasm = false;
		bool isEmissiveChasm = false;
		bool hasChasmWall = false;
		IndexBufferID chasmWallIndexBufferID = -1;
		if (isChasm)
		{
			chasmDef = &voxelChunk.getChasmDef(chasmDefID);
			isAnimatingChasm = chasmDef->animType == ChasmDefinition::AnimationType::Animated;
			isEmissiveChasm = chasmDef->isEmissive;

			const auto chasmWallIndexBufferIdIter = renderChunk.chasmWallIndexBufferIDsMap.find(voxel);
			if (chasmWallIndexBufferIdIter != renderChunk.chasmWallIndexBufferIDsMap.end())
			{
				hasChasmWall = true;
				chasmWallIndexBufferID = chasmWallIndexBufferIdIter->second;
			}
		}

		const VoxelFadeAnimationInstance *fadeAnimInst = nullptr;
		bool isFading = false;
		int fadeAnimInstIndex;
		if (voxelChunk.tryGetFadeAnimInstIndex(voxel.x, voxel.y, voxel.z, &fadeAnimInstIndex))
		{
			BufferView<const VoxelFadeAnimationInstance> fadeAnimInsts = voxelChunk.getFadeAnimInsts();
			fadeAnimInst = &fadeAnimInsts[fadeAnimInstIndex];
			isFading = !fadeAnimInst->isDoneFading();
		}

		const RenderLightIdList &voxelLightIdList = renderLightChunk.lightIdLists.get(voxel.x, voxel.y, voxel.z);

		constexpr int maxTransformsPerVoxel = DoorUtils::FACE_COUNT;
		constexpr int maxDrawCallsPerVoxel = RenderVoxelMeshInstance::MAX_TEXTURES + 1; // Sum of all index buffers.

		struct DrawCallTransformInitInfo
		{
			UniformBufferID id;
			int index;
			UniformBufferID preScaleTranslationBufferID;
		};

		struct DrawCallMeshInitInfo
		{
			VertexBufferID vertexBufferID;
			UniformBufferID normalBufferID;
			UniformBufferID texCoordBufferID;
			IndexBufferID indexBufferID;
		};

		struct DrawCallTextureInitInfo
		{
			ObjectTextureID id0, id1;
			const ObjectTextureID *varyingId0, *varyingId1;
			TextureSamplingType samplingType0, samplingType1;
		};

		struct DrawCallShadingInitInfo
		{
			VertexShaderType vertexShaderType;
			PixelShaderType pixelShaderType;
			double pixelShaderParam0; // For specialized values like texture coordinate manipulation.
		};

		struct DrawCallLightingInitInfo
		{
			RenderLightingType type;
			double percent;
			RenderLightID ids[RenderLightIdList::MAX_LIGHTS];
			int idCount;
		};

		// Populate various init infos to be used for generating draw calls.
		DrawCallTransformInitInfo transformInitInfos[maxTransformsPerVoxel];
		int transformInitInfoCount = 0;

		if (isDoor)
		{
			const auto transformIter = renderChunk.doorTransformBuffers.find(voxel);
			DebugAssert(transformIter != renderChunk.doorTransformBuffers.end());

			const UniformBufferID preScaleTranslationBufferID = (doorDef->getType() == ArenaTypes::DoorType::Raising) ? this->raisingDoorPreScaleTranslationBufferID : -1;
			for (int i = 0; i < maxTransformsPerVoxel; i++)
			{
				DrawCallTransformInitInfo &doorTransformInitInfo = transformInitInfos[i];
				doorTransformInitInfo.id = transformIter->second;
				doorTransformInitInfo.index = i;
				doorTransformInitInfo.preScaleTranslationBufferID = preScaleTranslationBufferID;
			}

			transformInitInfoCount = maxTransformsPerVoxel;
		}
		else
		{
			DrawCallTransformInitInfo &transformInitInfo = transformInitInfos[0];
			transformInitInfo.id = renderChunk.transformBufferID;
			transformInitInfo.index = GetVoxelRenderTransformIndex(voxel.x, voxel.y, voxel.z, renderChunk.getHeight());
			transformInitInfo.preScaleTranslationBufferID = -1;
			transformInitInfoCount = 1;
		}

		DrawCallMeshInitInfo meshInitInfos[maxDrawCallsPerVoxel];
		int meshInitInfoCount = 0;
		if (isDoor)
		{
			DrawCallMeshInitInfo &doorMeshInitInfo = meshInitInfos[0];
			doorMeshInitInfo.vertexBufferID = renderMeshInst.vertexBufferID;
			doorMeshInitInfo.normalBufferID = renderMeshInst.normalBufferID;
			doorMeshInitInfo.texCoordBufferID = renderMeshInst.texCoordBufferID;
			doorMeshInitInfo.indexBufferID = renderMeshInst.alphaTestedIndexBufferID;
			meshInitInfoCount = 1;
		}
		else if (isChasm)
		{
			DrawCallMeshInitInfo &chasmFloorMeshInitInfo = meshInitInfos[0];
			chasmFloorMeshInitInfo.vertexBufferID = renderMeshInst.vertexBufferID;
			chasmFloorMeshInitInfo.normalBufferID = renderMeshInst.normalBufferID;
			chasmFloorMeshInitInfo.texCoordBufferID = renderMeshInst.texCoordBufferID;
			chasmFloorMeshInitInfo.indexBufferID = renderMeshInst.opaqueIndexBufferIDs[0];
			meshInitInfoCount = 1;

			if (hasChasmWall)
			{
				DrawCallMeshInitInfo &chasmWallMeshInitInfo = meshInitInfos[1];
				chasmWallMeshInitInfo.vertexBufferID = renderMeshInst.vertexBufferID;
				chasmWallMeshInitInfo.normalBufferID = renderMeshInst.normalBufferID;
				chasmWallMeshInitInfo.texCoordBufferID = renderMeshInst.texCoordBufferID;
				chasmWallMeshInitInfo.indexBufferID = chasmWallIndexBufferID;
				meshInitInfoCount = 2;
			}
		}
		else
		{
			for (int i = 0; i < renderMeshInst.opaqueIndexBufferIdCount; i++)
			{
				DrawCallMeshInitInfo &opaqueMeshInitInfo = meshInitInfos[i];
				opaqueMeshInitInfo.vertexBufferID = renderMeshInst.vertexBufferID;
				opaqueMeshInitInfo.normalBufferID = renderMeshInst.normalBufferID;
				opaqueMeshInitInfo.texCoordBufferID = renderMeshInst.texCoordBufferID;
				opaqueMeshInitInfo.indexBufferID = renderMeshInst.opaqueIndexBufferIDs[i];
			}

			meshInitInfoCount = renderMeshInst.opaqueIndexBufferIdCount;

			if (renderMeshInst.alphaTestedIndexBufferID >= 0)
			{
				DrawCallMeshInitInfo &alphaTestedMeshInitInfo = meshInitInfos[renderMeshInst.opaqueIndexBufferIdCount];
				alphaTestedMeshInitInfo.vertexBufferID = renderMeshInst.vertexBufferID;
				alphaTestedMeshInitInfo.normalBufferID = renderMeshInst.normalBufferID;
				alphaTestedMeshInitInfo.texCoordBufferID = renderMeshInst.texCoordBufferID;
				alphaTestedMeshInitInfo.indexBufferID = renderMeshInst.alphaTestedIndexBufferID;
				meshInitInfoCount++;
			}
		}

		DrawCallTextureInitInfo textureInitInfos[maxDrawCallsPerVoxel];
		BufferView<const int> textureInitInfoIndices = DefaultVoxelTextureInitInfoIndices;
		int textureInitInfoCount = 0;
		if (isDoor)
		{
			DrawCallTextureInitInfo &doorTextureInitInfo = textureInitInfos[0];
			doorTextureInitInfo.id0 = this->getTextureID(voxelTextureDef.getTextureAsset(0));
			doorTextureInitInfo.id1 = -1;
			doorTextureInitInfo.varyingId0 = nullptr;
			doorTextureInitInfo.varyingId1 = nullptr;
			doorTextureInitInfo.samplingType0 = TextureSamplingType::Default;
			doorTextureInitInfo.samplingType1 = TextureSamplingType::Default;
			textureInitInfoCount = 1;
		}
		else if (isChasm)
		{
			const auto chasmFloorTextureIter = renderChunk.activeChasmFloorTextureIDs.find(chasmDefID);
			DebugAssert(chasmFloorTextureIter != renderChunk.activeChasmFloorTextureIDs.end());
			const ObjectTextureID *chasmFloorTextureIdPtr = &chasmFloorTextureIter->second;

			const ObjectTextureID chasmWallTextureID = this->getChasmWallTextureID(chunkPos, chasmDefID);
			const TextureSamplingType chasmFloorTextureSamplingType = isAnimatingChasm ? TextureSamplingType::ScreenSpaceRepeatY : TextureSamplingType::Default;
			const TextureSamplingType chasmWallTextureSamplingType = TextureSamplingType::Default;

			DrawCallTextureInitInfo &chasmFloorTextureInitInfo = textureInitInfos[0];
			chasmFloorTextureInitInfo.id0 = -1;
			chasmFloorTextureInitInfo.id1 = -1;
			chasmFloorTextureInitInfo.varyingId0 = chasmFloorTextureIdPtr;
			chasmFloorTextureInitInfo.varyingId1 = nullptr;
			chasmFloorTextureInitInfo.samplingType0 = chasmFloorTextureSamplingType;
			chasmFloorTextureInitInfo.samplingType1 = TextureSamplingType::Default;

			DrawCallTextureInitInfo &chasmWallTextureInitInfo = textureInitInfos[1];
			chasmWallTextureInitInfo.id0 = -1;
			chasmWallTextureInitInfo.id1 = chasmWallTextureID;
			chasmWallTextureInitInfo.varyingId0 = chasmFloorTextureIdPtr;
			chasmWallTextureInitInfo.varyingId1 = nullptr;
			chasmWallTextureInitInfo.samplingType0 = chasmFloorTextureSamplingType;
			chasmWallTextureInitInfo.samplingType1 = chasmWallTextureSamplingType;

			textureInitInfoCount = 2;
		}
		else
		{
			for (int i = 0; i < voxelTextureDef.textureCount; i++)
			{
				const TextureAsset &textureAsset = voxelTextureDef.getTextureAsset(i);

				DrawCallTextureInitInfo &textureInitInfo = textureInitInfos[i];
				textureInitInfo.id0 = this->getTextureID(textureAsset);
				textureInitInfo.id1 = -1;
				textureInitInfo.varyingId0 = nullptr;
				textureInitInfo.varyingId1 = nullptr;
				textureInitInfo.samplingType0 = TextureSamplingType::Default;
				textureInitInfo.samplingType1 = TextureSamplingType::Default;
			}

			if (voxelType == ArenaTypes::VoxelType::Raised)
			{
				textureInitInfoIndices = RaisedVoxelTextureInitInfoIndices;
			}

			textureInitInfoCount = voxelTextureDef.textureCount;
		}

		DrawCallShadingInitInfo shadingInitInfos[maxDrawCallsPerVoxel];
		int shadingInitInfoCount = 0;
		if (isDoor)
		{
			DrawCallShadingInitInfo &doorShadingInitInfo = shadingInitInfos[0];

			const ArenaTypes::DoorType doorType = doorDef->getType();
			switch (doorType)
			{
			case ArenaTypes::DoorType::Swinging:
				doorShadingInitInfo.vertexShaderType = VertexShaderType::Basic;
				doorShadingInitInfo.pixelShaderType = PixelShaderType::AlphaTested;
				doorShadingInitInfo.pixelShaderParam0 = 0.0;
				break;
			case ArenaTypes::DoorType::Sliding:
				doorShadingInitInfo.vertexShaderType = VertexShaderType::Basic;
				doorShadingInitInfo.pixelShaderType = PixelShaderType::AlphaTestedWithVariableTexCoordUMin;
				doorShadingInitInfo.pixelShaderParam0 = DoorUtils::getAnimatedTexCoordPercent(doorAnimPercent);
				break;
			case ArenaTypes::DoorType::Raising:
				doorShadingInitInfo.vertexShaderType = VertexShaderType::RaisingDoor;
				doorShadingInitInfo.pixelShaderType = PixelShaderType::AlphaTestedWithVariableTexCoordVMin;
				doorShadingInitInfo.pixelShaderParam0 = DoorUtils::getAnimatedTexCoordPercent(doorAnimPercent);
				break;
			case ArenaTypes::DoorType::Splitting:
				doorShadingInitInfo.vertexShaderType = VertexShaderType::Basic;
				doorShadingInitInfo.pixelShaderType = PixelShaderType::AlphaTestedWithVariableTexCoordUMin; // @todo: some "half and half" pixel shader
				doorShadingInitInfo.pixelShaderParam0 = DoorUtils::getAnimatedTexCoordPercent(doorAnimPercent);
				break;
			default:
				DebugNotImplementedMsg(std::to_string(static_cast<int>(doorType)));
				break;
			}

			shadingInitInfoCount = 1;
		}
		else if (isChasm)
		{
			DrawCallShadingInitInfo &chasmFloorShadingInitInfo = shadingInitInfos[0];
			chasmFloorShadingInitInfo.vertexShaderType = VertexShaderType::Basic;
			chasmFloorShadingInitInfo.pixelShaderType = PixelShaderType::Opaque;
			chasmFloorShadingInitInfo.pixelShaderParam0 = 0.0;

			DrawCallShadingInitInfo &chasmWallShadingInitInfo = shadingInitInfos[1];
			chasmWallShadingInitInfo.vertexShaderType = VertexShaderType::Basic;
			chasmWallShadingInitInfo.pixelShaderType = PixelShaderType::OpaqueWithAlphaTestLayer;
			chasmWallShadingInitInfo.pixelShaderParam0 = 0.0;

			shadingInitInfoCount = 2;
		}
		else
		{
			for (int i = 0; i < renderMeshInst.opaqueIndexBufferIdCount; i++)
			{
				DrawCallShadingInitInfo &opaqueShadingInitInfo = shadingInitInfos[i];
				opaqueShadingInitInfo.vertexShaderType = VertexShaderType::Basic;
				opaqueShadingInitInfo.pixelShaderType = PixelShaderType::Opaque;
				opaqueShadingInitInfo.pixelShaderParam0 = 0.0;
			}

			shadingInitInfoCount = renderMeshInst.opaqueIndexBufferIdCount;

			if (renderMeshInst.alphaTestedIndexBufferID >= 0)
			{
				DrawCallShadingInitInfo &alphaTestedShadingInitInfo = shadingInitInfos[renderMeshInst.opaqueIndexBufferIdCount];
				alphaTestedShadingInitInfo.vertexShaderType = VertexShaderType::Basic;
				alphaTestedShadingInitInfo.pixelShaderType = PixelShaderType::AlphaTested;
				alphaTestedShadingInitInfo.pixelShaderParam0 = 0.0;
			}
		}

		DrawCallLightingInitInfo lightingInitInfo;
		if (isFading)
		{
			lightingInitInfo.type = RenderLightingType::PerMesh;
			lightingInitInfo.percent = std::clamp(1.0 - fadeAnimInst->percentFaded, 0.0, 1.0);
			lightingInitInfo.idCount = 0;
		}
		else if (isEmissiveChasm)
		{
			lightingInitInfo.type = RenderLightingType::PerMesh;
			lightingInitInfo.percent = 1.0;
			lightingInitInfo.idCount = 0;
		}
		else
		{
			lightingInitInfo.type = RenderLightingType::PerPixel;

			BufferView<const RenderLightID> voxelLightIDs = voxelLightIdList.getLightIDs();
			DebugAssert(std::size(lightingInitInfo.ids) >= voxelLightIDs.getCount());
			std::copy(voxelLightIDs.begin(), voxelLightIDs.end(), std::begin(lightingInitInfo.ids));
			lightingInitInfo.idCount = voxelLightIDs.getCount();
		}

		bool visibleDoorFaces[DoorUtils::FACE_COUNT];
		int drawCallCount = 0;
		if (isDoor)
		{
			int doorVisInstIndex;
			if (!voxelChunk.tryGetDoorVisibilityInstIndex(voxel.x, voxel.y, voxel.z, &doorVisInstIndex))
			{
				DebugLogError("Expected door visibility instance at (" + voxel.toString() + ") in chunk (" + chunkPos.toString() + ").");
				continue;
			}

			BufferView<const VoxelDoorVisibilityInstance> doorVisInsts = voxelChunk.getDoorVisibilityInsts();
			const VoxelDoorVisibilityInstance &doorVisInst = doorVisInsts[doorVisInstIndex];
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

			drawCallCount = static_cast<int>(std::count(std::begin(visibleDoorFaces), std::end(visibleDoorFaces), true));
			DebugAssert(drawCallCount <= VoxelDoorVisibilityInstance::MAX_FACE_COUNT);
		}
		else if (isChasm)
		{
			drawCallCount = hasChasmWall ? 2 : 1;
		}
		else
		{
			drawCallCount = meshInitInfoCount;
		}

		const RenderVoxelDrawCallRangeID drawCallRangeID = drawCallHeap.alloc(drawCallCount);
		renderChunk.drawCallRangeIDs.set(voxel.x, voxel.y, voxel.z, drawCallRangeID);
		BufferView<RenderDrawCall> drawCalls = drawCallHeap.get(drawCallRangeID);

		if (isDoor)
		{			
			DebugAssert(transformInitInfoCount == DoorUtils::FACE_COUNT);

			int doorDrawCallWriteIndex = 0;
			for (int i = 0; i < transformInitInfoCount; i++)
			{
				if (!visibleDoorFaces[i])
				{
					continue;
				}
				
				const DrawCallTransformInitInfo &doorTransformInitInfo = transformInitInfos[i];
				const DrawCallMeshInitInfo &doorMeshInitInfo = meshInitInfos[0];
				const DrawCallTextureInitInfo &doorTextureInitInfo = textureInitInfos[0];
				const DrawCallShadingInitInfo &doorShadingInitInfo = shadingInitInfos[0];
				
				RenderDrawCall &doorDrawCall = drawCalls[doorDrawCallWriteIndex];
				doorDrawCall.transformBufferID = doorTransformInitInfo.id;
				doorDrawCall.transformIndex = doorTransformInitInfo.index;
				doorDrawCall.preScaleTranslationBufferID = doorTransformInitInfo.preScaleTranslationBufferID;
				doorDrawCall.vertexBufferID = doorMeshInitInfo.vertexBufferID;
				doorDrawCall.normalBufferID = doorMeshInitInfo.normalBufferID;
				doorDrawCall.texCoordBufferID = doorMeshInitInfo.texCoordBufferID;
				doorDrawCall.indexBufferID = doorMeshInitInfo.indexBufferID;
				doorDrawCall.textureIDs[0] = doorTextureInitInfo.id0;
				doorDrawCall.textureIDs[1] = doorTextureInitInfo.id1;
				doorDrawCall.varyingTextures[0] = doorTextureInitInfo.varyingId0;
				doorDrawCall.varyingTextures[1] = doorTextureInitInfo.varyingId1;
				doorDrawCall.textureSamplingTypes[0] = doorTextureInitInfo.samplingType0;
				doorDrawCall.textureSamplingTypes[1] = doorTextureInitInfo.samplingType1;
				doorDrawCall.vertexShaderType = doorShadingInitInfo.vertexShaderType;
				doorDrawCall.pixelShaderType = doorShadingInitInfo.pixelShaderType;
				doorDrawCall.pixelShaderParam0 = doorShadingInitInfo.pixelShaderParam0;
				doorDrawCall.lightingType = lightingInitInfo.type;
				doorDrawCall.lightPercent = lightingInitInfo.percent;
				std::copy(std::begin(lightingInitInfo.ids), std::end(lightingInitInfo.ids), std::begin(doorDrawCall.lightIDs));
				doorDrawCall.lightIdCount = lightingInitInfo.idCount;
				doorDrawCall.enableDepthRead = true;
				doorDrawCall.enableDepthWrite = true;
				
				doorDrawCallWriteIndex++;
			}
		}
		else if (isChasm)
		{
			const DrawCallTransformInitInfo &chasmTransformInitInfo = transformInitInfos[0];

			for (int i = 0; i < drawCallCount; i++)
			{
				const DrawCallMeshInitInfo &chasmMeshInitInfo = meshInitInfos[i];
				const DrawCallTextureInitInfo &chasmTextureInitInfo = textureInitInfos[i];
				const DrawCallShadingInitInfo &chasmShadingInitInfo = shadingInitInfos[i];
				
				RenderDrawCall &chasmDrawCall = drawCalls[i];
				chasmDrawCall.transformBufferID = chasmTransformInitInfo.id;
				chasmDrawCall.transformIndex = chasmTransformInitInfo.index;
				chasmDrawCall.preScaleTranslationBufferID = chasmTransformInitInfo.preScaleTranslationBufferID;
				chasmDrawCall.vertexBufferID = chasmMeshInitInfo.vertexBufferID;
				chasmDrawCall.normalBufferID = chasmMeshInitInfo.normalBufferID;
				chasmDrawCall.texCoordBufferID = chasmMeshInitInfo.texCoordBufferID;
				chasmDrawCall.indexBufferID = chasmMeshInitInfo.indexBufferID;
				chasmDrawCall.textureIDs[0] = chasmTextureInitInfo.id0;
				chasmDrawCall.textureIDs[1] = chasmTextureInitInfo.id1;
				chasmDrawCall.varyingTextures[0] = chasmTextureInitInfo.varyingId0;
				chasmDrawCall.varyingTextures[1] = chasmTextureInitInfo.varyingId1;
				chasmDrawCall.textureSamplingTypes[0] = chasmTextureInitInfo.samplingType0;
				chasmDrawCall.textureSamplingTypes[1] = chasmTextureInitInfo.samplingType1;
				chasmDrawCall.vertexShaderType = chasmShadingInitInfo.vertexShaderType;
				chasmDrawCall.pixelShaderType = chasmShadingInitInfo.pixelShaderType;
				chasmDrawCall.pixelShaderParam0 = chasmShadingInitInfo.pixelShaderParam0;
				chasmDrawCall.lightingType = lightingInitInfo.type;
				chasmDrawCall.lightPercent = lightingInitInfo.percent;
				std::copy(std::begin(lightingInitInfo.ids), std::end(lightingInitInfo.ids), std::begin(chasmDrawCall.lightIDs));
				chasmDrawCall.lightIdCount = lightingInitInfo.idCount;
				chasmDrawCall.enableDepthRead = true;
				chasmDrawCall.enableDepthWrite = true;
			}
		}
		else
		{
			const DrawCallTransformInitInfo &transformInitInfo = transformInitInfos[0];

			for (int i = 0; i < drawCallCount; i++)
			{
				const DrawCallMeshInitInfo &meshInitInfo = meshInitInfos[i];
				const int textureInitInfoIndex = textureInitInfoIndices[i];
				const DrawCallTextureInitInfo &textureInitInfo = textureInitInfos[textureInitInfoIndex];
				const DrawCallShadingInitInfo &shadingInitInfo = shadingInitInfos[i];

				RenderDrawCall &drawCall = drawCalls[i];
				drawCall.transformBufferID = transformInitInfo.id;
				drawCall.transformIndex = transformInitInfo.index;
				drawCall.preScaleTranslationBufferID = transformInitInfo.preScaleTranslationBufferID;
				drawCall.vertexBufferID = meshInitInfo.vertexBufferID;
				drawCall.normalBufferID = meshInitInfo.normalBufferID;
				drawCall.texCoordBufferID = meshInitInfo.texCoordBufferID;
				drawCall.indexBufferID = meshInitInfo.indexBufferID;
				drawCall.textureIDs[0] = textureInitInfo.id0;
				drawCall.textureIDs[1] = textureInitInfo.id1;
				drawCall.varyingTextures[0] = textureInitInfo.varyingId0;
				drawCall.varyingTextures[1] = textureInitInfo.varyingId1;
				drawCall.textureSamplingTypes[0] = textureInitInfo.samplingType0;
				drawCall.textureSamplingTypes[1] = textureInitInfo.samplingType1;
				drawCall.vertexShaderType = shadingInitInfo.vertexShaderType;
				drawCall.pixelShaderType = shadingInitInfo.pixelShaderType;
				drawCall.pixelShaderParam0 = shadingInitInfo.pixelShaderParam0;
				drawCall.lightingType = lightingInitInfo.type;
				drawCall.lightPercent = lightingInitInfo.percent;
				std::copy(std::begin(lightingInitInfo.ids), std::end(lightingInitInfo.ids), std::begin(drawCall.lightIDs));
				drawCall.lightIdCount = lightingInitInfo.idCount;
				drawCall.enableDepthRead = true;
				drawCall.enableDepthWrite = true;
			}
		}
	}
}

void RenderVoxelChunkManager::rebuildDrawCallsList(const VoxelVisibilityChunkManager &voxelVisChunkManager)
{
	this->drawCallsCache.clear();

	// @todo: eventually this should sort by distance from a CoordDouble2
	for (int i = 0; i < static_cast<int>(this->activeChunks.size()); i++)
	{
		const ChunkPtr &chunkPtr = this->activeChunks[i];
		const RenderVoxelChunk &renderChunk = *chunkPtr;
		const VoxelVisibilityChunk &voxelVisChunk = voxelVisChunkManager.getChunkAtIndex(i);

		BufferView3D<const RenderVoxelDrawCallRangeID> rangeIDs = renderChunk.drawCallRangeIDs;
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
	}
}

void RenderVoxelChunkManager::populateCommandBuffer(RenderCommandBuffer &commandBuffer) const
{
	commandBuffer.addDrawCalls(this->drawCallsCache);
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
	// Update pre-scale transition used by all raising doors (ideally this would be once on scene change).
	const Double3 raisingDoorPreScaleTranslation = MakeRaisingDoorPreScaleTranslation(ceilingScale);
	renderer.populateUniformBuffer(this->raisingDoorPreScaleTranslationBufferID, raisingDoorPreScaleTranslation);

	for (const ChunkInt2 &chunkPos : newChunkPositions)
	{
		RenderVoxelChunk &renderChunk = this->getChunkAtPosition(chunkPos);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		const VoxelVisibilityChunk &voxelVisChunk = voxelVisChunkManager.getChunkAtPosition(chunkPos);
		this->loadMeshBuffers(renderChunk, voxelChunk, ceilingScale, renderer);
		this->loadTextures(voxelChunk, textureManager, renderer);
		this->loadChasmWalls(renderChunk, voxelChunk);
		this->loadTransforms(renderChunk, voxelChunk, ceilingScale, renderer);
	}

	for (const ChunkInt2 &chunkPos : activeChunkPositions)
	{
		RenderVoxelChunk &renderChunk = this->getChunkAtPosition(chunkPos);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		const VoxelVisibilityChunk &voxelVisChunk = voxelVisChunkManager.getChunkAtPosition(chunkPos);
		const RenderLightChunk &renderLightChunk = renderLightChunkManager.getChunkAtPosition(chunkPos);

		BufferView<const VoxelInt3> dirtyChasmWallInstVoxels = voxelChunk.getDirtyChasmWallInstPositions();
		for (const VoxelInt3 &chasmWallPos : dirtyChasmWallInstVoxels)
		{
			this->loadChasmWall(renderChunk, voxelChunk, chasmWallPos.x, chasmWallPos.y, chasmWallPos.z);
		}

		// Update chasm animation textures, shared by all chasms in this chunk.
		for (int i = 0; i < voxelChunk.getChasmDefCount(); i++)
		{
			const VoxelChunk::ChasmDefID chasmDefID = static_cast<VoxelChunk::ChasmDefID>(i);
			auto activeTextureIter = renderChunk.activeChasmFloorTextureIDs.find(chasmDefID);
			if (activeTextureIter == renderChunk.activeChasmFloorTextureIDs.end())
			{
				ObjectTextureID dummyChasmFloorTextureID = -1;
				activeTextureIter = renderChunk.activeChasmFloorTextureIDs.emplace(chasmDefID, dummyChasmFloorTextureID).first;
			}

			activeTextureIter->second = this->getChasmFloorTextureID(chunkPos, chasmDefID, chasmAnimPercent);
		}

		// Update door render transforms (rotation angle, etc.).
		BufferView<const VoxelInt3> dirtyDoorAnimInstVoxels = voxelChunk.getDirtyDoorAnimInstPositions();
		for (const VoxelInt3 &doorVoxel : dirtyDoorAnimInstVoxels)
		{
			VoxelChunk::DoorDefID doorDefID;
			if (!voxelChunk.tryGetDoorDefID(doorVoxel.x, doorVoxel.y, doorVoxel.z, &doorDefID))
			{
				DebugLogError("Expected door def ID at (" + doorVoxel.toString() + ").");
				continue;
			}

			const DoorDefinition &doorDef = voxelChunk.getDoorDef(doorDefID);
			const ArenaTypes::DoorType doorType = doorDef.getType();
			const WorldDouble3 worldPosition = MakeVoxelWorldPosition(voxelChunk.getPosition(), doorVoxel, ceilingScale);
			const double doorAnimPercent = DoorUtils::getAnimPercentOrZero(doorVoxel.x, doorVoxel.y, doorVoxel.z, voxelChunk);

			for (int i = 0; i < DoorUtils::FACE_COUNT; i++)
			{
				const RenderTransform faceRenderTransform = MakeDoorFaceRenderTransform(doorType, i, worldPosition, doorAnimPercent);

				const auto doorTransformIter = renderChunk.doorTransformBuffers.find(doorVoxel);
				DebugAssert(doorTransformIter != renderChunk.doorTransformBuffers.end());
				const UniformBufferID doorTransformBufferID = doorTransformIter->second;
				renderer.populateUniformAtIndex(doorTransformBufferID, i, faceRenderTransform);
			}
		}

		// Update draw calls of dirty voxels.
		// - @todo: there is some double/triple updating possible here, maybe optimize.
		BufferView<const VoxelInt3> dirtyShapeDefVoxels = voxelChunk.getDirtyShapeDefPositions();
		BufferView<const VoxelInt3> dirtyDoorVisInstVoxels = voxelChunk.getDirtyDoorVisInstPositions();
		BufferView<const VoxelInt3> dirtyFadeAnimInstVoxels = voxelChunk.getDirtyFadeAnimInstPositions();
		BufferView<const VoxelInt3> dirtyLightVoxels = renderLightChunk.dirtyVoxelPositions;
		this->updateChunkDrawCalls(renderChunk, dirtyShapeDefVoxels, voxelChunk, renderLightChunk, ceilingScale, chasmAnimPercent);
		this->updateChunkDrawCalls(renderChunk, dirtyDoorAnimInstVoxels, voxelChunk, renderLightChunk, ceilingScale, chasmAnimPercent);
		this->updateChunkDrawCalls(renderChunk, dirtyDoorVisInstVoxels, voxelChunk, renderLightChunk, ceilingScale, chasmAnimPercent);
		this->updateChunkDrawCalls(renderChunk, dirtyFadeAnimInstVoxels, voxelChunk, renderLightChunk, ceilingScale, chasmAnimPercent);
		this->updateChunkDrawCalls(renderChunk, dirtyChasmWallInstVoxels, voxelChunk, renderLightChunk, ceilingScale, chasmAnimPercent);
		this->updateChunkDrawCalls(renderChunk, dirtyLightVoxels, voxelChunk, renderLightChunk, ceilingScale, chasmAnimPercent);
	}

	this->rebuildDrawCallsList(voxelVisChunkManager);
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
