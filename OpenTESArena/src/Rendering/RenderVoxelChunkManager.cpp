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
#include "../Voxels/VoxelChunkManager.h"
#include "../Voxels/VoxelDoorUtils.h"
#include "../Voxels/VoxelFaceCombineChunkManager.h"
#include "../Voxels/VoxelFrustumCullingChunk.h"
#include "../Voxels/VoxelFrustumCullingChunkManager.h"

#include "components/debug/Debug.h"

namespace
{
	int GetVoxelRenderTransformIndex(SNInt x, int y, WEInt z, int chunkHeight)
	{
		return x + (y * Chunk::WIDTH) + (z * Chunk::WIDTH * chunkHeight);
	}

	// Loads the given voxel definition's textures into the voxel textures list if they haven't been loaded yet.
	void LoadVoxelDefTextures(const VoxelTextureDefinition &voxelTextureDef, std::vector<RenderVoxelLoadedTexture> &textures,
		TextureManager &textureManager, Renderer &renderer)
	{
		for (int i = 0; i < voxelTextureDef.textureCount; i++)
		{
			const TextureAsset &textureAsset = voxelTextureDef.getTextureAsset(i);
			const auto cacheIter = std::find_if(textures.begin(), textures.end(),
				[&textureAsset](const RenderVoxelLoadedTexture &loadedTexture)
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
				const ObjectTextureID voxelTextureID = renderer.createObjectTexture(textureBuilder);
				if (voxelTextureID < 0)
				{
					DebugLogWarning("Couldn't create voxel texture \"" + textureAsset.filename + "\".");
					continue;
				}

				ScopedObjectTextureRef voxelTextureRef(voxelTextureID, renderer);
				RenderVoxelLoadedTexture newTexture;
				newTexture.init(textureAsset, std::move(voxelTextureRef));
				textures.emplace_back(std::move(newTexture));
			}
		}
	}

	bool LoadedChasmFloorComparer(const RenderVoxelLoadedChasmFloorTexture &textureList, const VoxelChasmDefinition &chasmDef)
	{
		if (textureList.animType != chasmDef.animType)
		{
			return false;
		}

		if (textureList.animType == VoxelChasmAnimationType::SolidColor)
		{
			return textureList.paletteIndex == chasmDef.solidColor.paletteIndex;
		}
		else if (textureList.animType == VoxelChasmAnimationType::Animated)
		{
			const int textureAssetCount = static_cast<int>(textureList.textureAssets.size());
			const VoxelChasmAnimated &chasmDefAnimated = chasmDef.animated;

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

	void LoadChasmDefTextures(VoxelChasmDefID chasmDefID, const VoxelChunkManager &voxelChunkManager,
		Span<const RenderVoxelLoadedTexture> textures, std::vector<RenderVoxelLoadedChasmFloorTexture> &chasmFloorTextures,
		std::vector<RenderVoxelLoadedChasmTextureKey> &chasmTextureKeys, TextureManager &textureManager, Renderer &renderer)
	{
		const VoxelChasmDefinition &chasmDef = voxelChunkManager.getChasmDef(chasmDefID);

		// Check if this chasm already has a mapping (i.e. have we seen this chunk before?).
		const auto keyIter = std::find_if(chasmTextureKeys.begin(), chasmTextureKeys.end(),
			[chasmDefID](const RenderVoxelLoadedChasmTextureKey &loadedKey)
		{
			return loadedKey.chasmDefID == chasmDefID;
		});

		if (keyIter != chasmTextureKeys.end())
		{
			return;
		}

		// Check if any loaded chasm floors reference the same asset(s).
		const auto chasmFloorIter = std::find_if(chasmFloorTextures.begin(), chasmFloorTextures.end(),
			[&chasmDef](const RenderVoxelLoadedChasmFloorTexture &textureList)
		{
			return LoadedChasmFloorComparer(textureList, chasmDef);
		});

		int chasmFloorListIndex = -1;
		if (chasmFloorIter != chasmFloorTextures.end())
		{
			chasmFloorListIndex = static_cast<int>(std::distance(chasmFloorTextures.begin(), chasmFloorIter));
		}
		else
		{
			// Load the required textures and add a key for them.
			RenderVoxelLoadedChasmFloorTexture newFloorTexture;
			if (chasmDef.animType == VoxelChasmAnimationType::SolidColor)
			{
				// Dry chasms are a single color, no texture asset.
				const ObjectTextureID dryChasmTextureID = renderer.createObjectTexture(1, 1, 1);
				if (dryChasmTextureID < 0)
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

				newFloorTexture.initColor(paletteIndex, std::move(dryChasmTextureRef));
				chasmFloorTextures.emplace_back(std::move(newFloorTexture));
			}
			else if (chasmDef.animType == VoxelChasmAnimationType::Animated)
			{
				const TextureAsset &firstFrameTextureAsset = chasmDef.animated.textureAssets[0];
				const std::optional<TextureBuilderID> firstFrameTextureBuilderID = textureManager.tryGetTextureBuilderID(firstFrameTextureAsset);
				if (!firstFrameTextureBuilderID.has_value())
				{
					DebugLogWarning("Couldn't load first frame of chasm texture \"" + firstFrameTextureAsset.filename + "\".");
					return;
				}

				const TextureBuilder &firstFrameTextureBuilder = textureManager.getTextureBuilderHandle(*firstFrameTextureBuilderID);
				int newObjectTextureWidth = firstFrameTextureBuilder.getWidth();
				int newObjectTextureHeight = firstFrameTextureBuilder.getHeight() * chasmDef.animated.textureAssets.getCount();

				const int bytesPerTexel = 1;
				DebugAssert(firstFrameTextureBuilder.getBytesPerTexel() == bytesPerTexel);

				const ObjectTextureID chasmTextureID = renderer.createObjectTexture(newObjectTextureWidth, newObjectTextureHeight, bytesPerTexel);
				if (chasmTextureID < 0)
				{
					DebugLogWarningFormat("Couldn't create chasm texture sheet %s %dx%d.", firstFrameTextureAsset.filename.c_str(), newObjectTextureWidth, newObjectTextureHeight);
					return;
				}

				ScopedObjectTextureRef newObjectTextureRef(chasmTextureID, renderer);
				LockedTexture lockedTexture = renderer.lockObjectTexture(chasmTextureID);
				if (!lockedTexture.isValid())
				{
					DebugLogWarningFormat("Couldn't lock chasm texture %s for writing.", firstFrameTextureAsset.filename.c_str());
					return;
				}

				std::vector<TextureAsset> newTextureAssets;
				int newObjectTextureCurrentY = 0;
				for (const TextureAsset &textureAsset : chasmDef.animated.textureAssets)
				{
					const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAsset);
					if (!textureBuilderID.has_value())
					{
						DebugLogWarning("Couldn't load chasm texture builder \"" + textureAsset.filename + "\".");
						continue;
					}

					const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
					DebugAssert(textureBuilder.type == TextureBuilderType::Paletted);
					const Span2D<const uint8_t> textureBuilderTexels = textureBuilder.paletteTexture.texels;
					const int dstByteOffset = (newObjectTextureCurrentY * newObjectTextureWidth) * bytesPerTexel;
					uint8_t *dstTexels = static_cast<uint8_t*>(lockedTexture.texels);
					std::copy(textureBuilderTexels.begin(), textureBuilderTexels.end(), dstTexels + dstByteOffset);
					newObjectTextureCurrentY += firstFrameTextureBuilder.getHeight();

					newTextureAssets.emplace_back(textureAsset);
				}

				renderer.unlockObjectTexture(chasmTextureID);

				newFloorTexture.initTextured(std::move(newTextureAssets), std::move(newObjectTextureRef));
				chasmFloorTextures.emplace_back(std::move(newFloorTexture));
			}
			else
			{
				DebugNotImplementedMsg(std::to_string(static_cast<int>(chasmDef.animType)));
			}

			chasmFloorListIndex = static_cast<int>(chasmFloorTextures.size()) - 1;
		}

		// The chasm wall (if any) should already be loaded as a voxel texture during map gen.
		// @todo: support chasm walls adding to the voxel textures list (i.e. for destroyed voxels; the list would have to be non-const)
		const auto chasmWallIter = std::find_if(textures.begin(), textures.end(),
			[&chasmDef](const RenderVoxelLoadedTexture &texture)
		{
			return texture.textureAsset == chasmDef.wallTextureAsset;
		});

		DebugAssert(chasmWallIter != textures.end());
		const int chasmWallIndex = static_cast<int>(std::distance(textures.begin(), chasmWallIter));

		DebugAssert(chasmFloorListIndex >= 0);
		DebugAssert(chasmWallIndex >= 0);

		RenderVoxelLoadedChasmTextureKey key;
		key.init(chasmDefID, chasmFloorListIndex, chasmWallIndex);
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

	RenderTransform MakeDoorFaceRenderTransform(ArenaDoorType doorType, int doorFaceIndex, const WorldDouble3 &worldPosition, double animPercent)
	{
		const Radians faceBaseRadians = VoxelDoorUtils::BaseAngles[doorFaceIndex];
		const Double3 hingeOffset = VoxelDoorUtils::SwingingHingeOffsets[doorFaceIndex];
		const Double3 hingePosition = worldPosition + hingeOffset;

		RenderTransform renderTransform;
		switch (doorType)
		{
		case ArenaDoorType::Swinging:
		{
			const Radians rotationRadians = VoxelDoorUtils::getSwingingRotationRadians(faceBaseRadians, animPercent);
			renderTransform.translation = Matrix4d::translation(hingePosition.x, hingePosition.y, hingePosition.z);
			renderTransform.rotation = Matrix4d::yRotation(rotationRadians);
			renderTransform.scale = Matrix4d::identity();
			break;
		}
		case ArenaDoorType::Sliding:
		{
			const double uMin = VoxelDoorUtils::getAnimatedTexCoordPercent(animPercent);
			const double scaleAmount = VoxelDoorUtils::getAnimatedScaleAmount(uMin);
			renderTransform.translation = Matrix4d::translation(hingePosition.x, hingePosition.y, hingePosition.z);
			renderTransform.rotation = Matrix4d::yRotation(faceBaseRadians);
			renderTransform.scale = Matrix4d::scale(1.0, 1.0, scaleAmount);
			break;
		}
		case ArenaDoorType::Raising:
		{
			const double vMin = VoxelDoorUtils::getAnimatedTexCoordPercent(animPercent);
			const double scaleAmount = VoxelDoorUtils::getAnimatedScaleAmount(vMin);
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

void RenderVoxelLoadedTexture::init(const TextureAsset &textureAsset, ScopedObjectTextureRef &&objectTextureRef)
{
	this->textureAsset = textureAsset;
	this->objectTextureRef = std::move(objectTextureRef);
}

RenderVoxelLoadedChasmFloorTexture::RenderVoxelLoadedChasmFloorTexture()
{
	this->animType = static_cast<VoxelChasmAnimationType>(-1);
	this->paletteIndex = 0;
}

void RenderVoxelLoadedChasmFloorTexture::initColor(uint8_t paletteIndex, ScopedObjectTextureRef &&objectTextureRef)
{
	this->animType = VoxelChasmAnimationType::SolidColor;
	this->paletteIndex = paletteIndex;
	this->objectTextureRef = std::move(objectTextureRef);
}

void RenderVoxelLoadedChasmFloorTexture::initTextured(std::vector<TextureAsset> &&textureAssets, ScopedObjectTextureRef &&objectTextureRef)
{
	this->animType = VoxelChasmAnimationType::Animated;
	this->textureAssets = std::move(textureAssets);
	this->objectTextureRef = std::move(objectTextureRef);
}

void RenderVoxelLoadedChasmTextureKey::init(VoxelChasmDefID chasmDefID, int chasmFloorListIndex, int chasmWallIndex)
{
	this->chasmDefID = chasmDefID;
	this->chasmFloorListIndex = chasmFloorListIndex;
	this->chasmWallIndex = chasmWallIndex;
}

RenderVoxelCombinedFaceVertexBuffer::RenderVoxelCombinedFaceVertexBuffer()
{
	this->voxelWidth = 0;
	this->voxelHeight = 0;
	this->shapeDefID = -1;
	this->facing = static_cast<VoxelFacing3D>(-1);
	this->positionBufferID = -1;
	this->normalBufferID = -1;
	this->texCoordBufferID = -1;
}

RenderVoxelChunkManager::RenderVoxelChunkManager()
{
	this->raisingDoorPreScaleTranslationBufferID = -1;
	this->defaultQuadIndexBufferID = -1;
	this->chasmWallIndexBufferIDs.fill(-1);
}

void RenderVoxelChunkManager::init(Renderer &renderer)
{
	// Populate pre-scale translation transform (for raising doors).
	this->raisingDoorPreScaleTranslationBufferID = renderer.createUniformBuffer(1, sizeof(Double3), alignof(Double3));
	if (this->raisingDoorPreScaleTranslationBufferID < 0)
	{
		DebugLogError("Couldn't create uniform buffer for pre-scale translation.");
		return;
	}

	const Double3 preScaleTranslation = Double3::Zero; // Populated on scene change.
	renderer.populateUniformBuffer(this->raisingDoorPreScaleTranslationBufferID, preScaleTranslation);

	// Populate default quad indices for combined voxel faces.
	constexpr int indicesPerQuad = MeshUtils::INDICES_PER_QUAD;
	this->defaultQuadIndexBufferID = renderer.createIndexBuffer(indicesPerQuad);
	if (this->defaultQuadIndexBufferID < 0)
	{
		DebugLogError("Couldn't create default quad index buffer.");
		return;
	}

	renderer.populateIndexBuffer(this->defaultQuadIndexBufferID, MeshUtils::DefaultQuadVertexIndices);

	// Populate chasm wall index buffers.
	ArenaChasmWallIndexBuffer northIndices, eastIndices, southIndices, westIndices;
	ArenaMeshUtils::writeChasmWallRendererIndexBuffers(&northIndices, &eastIndices, &southIndices, &westIndices);

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

		const int indexCount = faceCount * indicesPerQuad;
		IndexBufferID &indexBufferID = this->chasmWallIndexBufferIDs[i];
		indexBufferID = renderer.createIndexBuffer(indexCount);
		if (indexBufferID < 0)
		{
			DebugLogError("Couldn't create chasm wall index buffer " + std::to_string(i) + ".");
			continue;
		}

		std::array<int32_t, ArenaMeshUtils::CHASM_WALL_TOTAL_COUNT * indicesPerQuad> totalIndicesBuffer;
		int writingIndex = 0;
		auto tryWriteIndices = [indicesPerQuad, &totalIndicesBuffer, &writingIndex](bool hasFace, const ArenaChasmWallIndexBuffer &faceIndices)
		{
			if (hasFace)
			{
				std::copy(faceIndices.begin(), faceIndices.end(), totalIndicesBuffer.begin() + writingIndex);
				writingIndex += indicesPerQuad;
			}
		};

		tryWriteIndices(hasNorth, northIndices);
		tryWriteIndices(hasEast, eastIndices);
		tryWriteIndices(hasSouth, southIndices);
		tryWriteIndices(hasWest, westIndices);

		renderer.populateIndexBuffer(indexBufferID, Span<const int32_t>(totalIndicesBuffer.data(), writingIndex));
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

	if (this->defaultQuadIndexBufferID >= 0)
	{
		renderer.freeIndexBuffer(this->defaultQuadIndexBufferID);
		this->defaultQuadIndexBufferID = -1;
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
	this->chasmFloorTextures.clear();
	this->chasmTextureKeys.clear();
	this->drawCallsCache.clear();
}

ObjectTextureID RenderVoxelChunkManager::getTextureID(const TextureAsset &textureAsset) const
{
	const auto iter = std::find_if(this->textures.begin(), this->textures.end(),
		[&textureAsset](const RenderVoxelLoadedTexture &loadedTexture)
	{
		return loadedTexture.textureAsset == textureAsset;
	});

	DebugAssertMsg(iter != this->textures.end(), "No loaded voxel texture for \"" + textureAsset.filename + "\".");
	const ScopedObjectTextureRef &objectTextureRef = iter->objectTextureRef;
	return objectTextureRef.get();
}

ObjectTextureID RenderVoxelChunkManager::getChasmFloorTextureID(VoxelChasmDefID chasmDefID) const
{
	const auto keyIter = std::find_if(this->chasmTextureKeys.begin(), this->chasmTextureKeys.end(),
		[chasmDefID](const RenderVoxelLoadedChasmTextureKey &key)
	{
		return key.chasmDefID == chasmDefID;
	});

	DebugAssertMsg(keyIter != this->chasmTextureKeys.end(), "No chasm texture key for chasm def ID \"" + std::to_string(chasmDefID) + "\".");

	const int floorListIndex = keyIter->chasmFloorListIndex;
	DebugAssertIndex(this->chasmFloorTextures, floorListIndex);
	const RenderVoxelLoadedChasmFloorTexture &textureList = this->chasmFloorTextures[floorListIndex];
	const ScopedObjectTextureRef &objectTextureRef = textureList.objectTextureRef;
	return objectTextureRef.get();
}

ObjectTextureID RenderVoxelChunkManager::getChasmWallTextureID(VoxelChasmDefID chasmDefID) const
{
	const auto keyIter = std::find_if(this->chasmTextureKeys.begin(), this->chasmTextureKeys.end(),
		[chasmDefID](const RenderVoxelLoadedChasmTextureKey &key)
	{
		return key.chasmDefID == chasmDefID;
	});

	DebugAssertMsg(keyIter != this->chasmTextureKeys.end(), "No chasm texture key for chasm def ID \"" + std::to_string(chasmDefID) + "\".");

	const int wallIndex = keyIter->chasmWallIndex;
	const RenderVoxelLoadedTexture &voxelTexture = this->textures[wallIndex];
	const ScopedObjectTextureRef &objectTextureRef = voxelTexture.objectTextureRef;
	return objectTextureRef.get();
}

void RenderVoxelChunkManager::loadChunkTextures(const VoxelChunk &voxelChunk, const VoxelChunkManager &voxelChunkManager, TextureManager &textureManager, Renderer &renderer)
{
	for (int i = 0; i < voxelChunk.getTextureDefCount(); i++)
	{
		const VoxelTextureDefinition &voxelTextureDef = voxelChunk.getTextureDef(i);
		LoadVoxelDefTextures(voxelTextureDef, this->textures, textureManager, renderer);
	}

	for (int i = 0; i < voxelChunkManager.getChasmDefCount(); i++)
	{
		const VoxelChasmDefID chasmDefID = static_cast<VoxelChasmDefID>(i);
		LoadChasmDefTextures(chasmDefID, voxelChunkManager, this->textures, this->chasmFloorTextures, this->chasmTextureKeys, textureManager, renderer);
	}
}

void RenderVoxelChunkManager::loadMeshBuffers(RenderVoxelChunk &renderChunk, const VoxelChunk &voxelChunk, double ceilingScale, Renderer &renderer)
{
	const ChunkInt2 chunkPos = voxelChunk.position;

	// Add render chunk voxel mesh instances and create mappings to them.
	for (int shapeDefIndex = 0; shapeDefIndex < voxelChunk.getShapeDefCount(); shapeDefIndex++)
	{
		const VoxelShapeDefID voxelShapeDefID = static_cast<VoxelShapeDefID>(shapeDefIndex);
		const VoxelShapeDefinition &voxelShapeDef = voxelChunk.getShapeDef(voxelShapeDefID);
		const VoxelMeshDefinition &voxelMeshDef = voxelShapeDef.mesh;
		const bool isRenderMeshValid = !voxelMeshDef.isEmpty(); // Air has a shape for trigger voxels but no mesh
		if (!isRenderMeshValid)
		{
			continue;
		}

		RenderVoxelMeshInstance renderVoxelMeshInst;
		constexpr int positionComponentsPerVertex = MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
		constexpr int normalComponentsPerVertex = MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
		constexpr int texCoordComponentsPerVertex = MeshUtils::TEX_COORD_COMPONENTS_PER_VERTEX;

		const int vertexCount = MeshUtils::getVertexCount(voxelMeshDef.rendererPositions, MeshUtils::POSITION_COMPONENTS_PER_VERTEX);
		renderVoxelMeshInst.positionBufferID = renderer.createVertexPositionBuffer(vertexCount, positionComponentsPerVertex);
		if (renderVoxelMeshInst.positionBufferID < 0)
		{
			DebugLogError("Couldn't create vertex position buffer for voxel shape def ID " + std::to_string(voxelShapeDefID) + " in chunk (" + chunkPos.toString() + ").");
			continue;
		}

		renderVoxelMeshInst.normalBufferID = renderer.createVertexAttributeBuffer(vertexCount, normalComponentsPerVertex);
		if (renderVoxelMeshInst.normalBufferID < 0)
		{
			DebugLogError("Couldn't create vertex normal attribute buffer for voxel shape def ID " + std::to_string(voxelShapeDefID) + " in chunk (" + chunkPos.toString() + ").");
			renderVoxelMeshInst.freeBuffers(renderer);
			continue;
		}

		renderVoxelMeshInst.texCoordBufferID = renderer.createVertexAttributeBuffer(vertexCount, texCoordComponentsPerVertex);
		if (renderVoxelMeshInst.texCoordBufferID < 0)
		{
			DebugLogError("Couldn't create vertex tex coord attribute buffer for voxel shape def ID " + std::to_string(voxelShapeDefID) + " in chunk (" + chunkPos.toString() + ").");
			renderVoxelMeshInst.freeBuffers(renderer);
			continue;
		}

		// Populate renderer mesh geometry and indices from this voxel definition.
		renderer.populateVertexPositionBuffer(renderVoxelMeshInst.positionBufferID, voxelMeshDef.rendererPositions);
		renderer.populateVertexAttributeBuffer(renderVoxelMeshInst.normalBufferID, voxelMeshDef.rendererNormals);
		renderer.populateVertexAttributeBuffer(renderVoxelMeshInst.texCoordBufferID, voxelMeshDef.rendererTexCoords);

		const int indexBufferCount = voxelMeshDef.indicesListCount;
		for (int indexBufferIndex = 0; indexBufferIndex < indexBufferCount; indexBufferIndex++)
		{
			Span<const int32_t> indices = voxelMeshDef.indicesLists[indexBufferIndex];
			const int indexCount = indices.getCount();

			DebugAssertIndex(renderVoxelMeshInst.indexBufferIDs, indexBufferIndex);
			IndexBufferID &indexBufferID = renderVoxelMeshInst.indexBufferIDs[indexBufferIndex];
			indexBufferID = renderer.createIndexBuffer(indexCount);
			if (indexBufferID < 0)
			{
				DebugLogErrorFormat("Couldn't create index buffer for voxel shape def ID %d in chunk (%s).", voxelShapeDefID, voxelChunk.position.toString().c_str());
				renderVoxelMeshInst.freeBuffers(renderer);
				continue;
			}

			renderVoxelMeshInst.indexBufferIdCount++;
			renderer.populateIndexBuffer(indexBufferID, indices);
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
		Span<const VoxelChasmWallInstance> chasmWallInsts = voxelChunk.getChasmWallInsts();
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
		for (int y = 0; y < voxelChunk.height; y++)
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
	const int chunkHeight = voxelChunk.height;

	// Allocate one large uniform buffer that covers all voxels. Air is wasted and doors are double-allocated but this
	// is much faster than one buffer per voxel.
	const int chunkTransformsCount = Chunk::WIDTH * chunkHeight * Chunk::DEPTH;
	const UniformBufferID chunkTransformsBufferID = renderer.createUniformBuffer(chunkTransformsCount, sizeof(RenderTransform), alignof(RenderTransform));
	if (chunkTransformsBufferID < 0)
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
				const WorldDouble3 worldPosition = MakeVoxelWorldPosition(voxelChunk.position, voxel, ceilingScale);

				VoxelDoorDefID doorDefID;
				if (voxelChunk.tryGetDoorDefID(x, y, z, &doorDefID))
				{
					// Door transform uniform buffers. These are separate because each voxel has a RenderTransform per door face.
					const VoxelDoorDefinition &doorDef = voxelChunk.getDoorDef(doorDefID);
					const ArenaDoorType doorType = doorDef.type;
					DebugAssert(renderChunk.doorTransformBuffers.find(voxel) == renderChunk.doorTransformBuffers.end());

					constexpr int doorFaceCount = VoxelDoorUtils::FACE_COUNT;

					// Each door voxel has a uniform buffer, one render transform per face.
					const UniformBufferID doorTransformBufferID = renderer.createUniformBuffer(doorFaceCount, sizeof(RenderTransform), alignof(RenderTransform));
					if (doorTransformBufferID < 0)
					{
						DebugLogError("Couldn't create uniform buffer for door transform.");
						continue;
					}

					const double doorAnimPercent = VoxelDoorUtils::getAnimPercentOrZero(voxel.x, voxel.y, voxel.z, voxelChunk);

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

void RenderVoxelChunkManager::updateChunkVoxelDrawCalls(RenderVoxelChunk &renderChunk, Span<const VoxelInt3> dirtyVoxelPositions,
	const VoxelChunk &voxelChunk, const RenderLightChunk &renderLightChunk, const VoxelChunkManager &voxelChunkManager,
	double ceilingScale, double chasmAnimPercent)
{
	const ChunkInt2 chunkPos = renderChunk.position;
	RenderVoxelDrawCallHeap &drawCallHeap = renderChunk.drawCallHeap;

	// Regenerate all draw calls in the given dirty voxels.
	for (const VoxelInt3 voxel : dirtyVoxelPositions)
	{
		renderChunk.freeDrawCalls(voxel.x, voxel.y, voxel.z);

		const VoxelShapeDefID voxelShapeDefID = voxelChunk.getShapeDefID(voxel.x, voxel.y, voxel.z);
		const VoxelShapeDefinition &voxelShapeDef = voxelChunk.getShapeDef(voxelShapeDefID);
		const VoxelMeshDefinition &voxelMeshDef = voxelShapeDef.mesh;
		if (voxelMeshDef.isEmpty())
		{
			continue;
		}

		// @todo: if voxel participates in face combining algorithm, then continue

		const VoxelTextureDefID voxelTextureDefID = voxelChunk.getTextureDefID(voxel.x, voxel.y, voxel.z);
		const VoxelShadingDefID voxelShadingDefID = voxelChunk.getShadingDefID(voxel.x, voxel.y, voxel.z);
		const VoxelTextureDefinition &voxelTextureDef = voxelChunk.getTextureDef(voxelTextureDefID);
		const VoxelShadingDefinition &voxelShadingDef = voxelChunk.getShadingDef(voxelShadingDefID);

		const auto meshInstIter = renderChunk.meshInstMappings.find(voxelShapeDefID);
		DebugAssert(meshInstIter != renderChunk.meshInstMappings.end());
		const RenderVoxelMeshInstID renderMeshInstID = meshInstIter->second;
		renderChunk.meshInstIDs.set(voxel.x, voxel.y, voxel.z, renderMeshInstID);
		DebugAssertIndex(renderChunk.meshInsts, renderMeshInstID);
		const RenderVoxelMeshInstance &renderMeshInst = renderChunk.meshInsts[renderMeshInstID];

		VoxelDoorDefID doorDefID;
		const bool isDoor = voxelChunk.tryGetDoorDefID(voxel.x, voxel.y, voxel.z, &doorDefID);
		const VoxelDoorDefinition *doorDef = isDoor ? &voxelChunk.getDoorDef(doorDefID) : nullptr;

		double doorAnimPercent = 0.0;
		if (isDoor)
		{
			int doorAnimInstIndex;
			if (voxelChunk.tryGetDoorAnimInstIndex(voxel.x, voxel.y, voxel.z, &doorAnimInstIndex))
			{
				Span<const VoxelDoorAnimationInstance> doorAnimInsts = voxelChunk.getDoorAnimInsts();
				const VoxelDoorAnimationInstance &doorAnimInst = doorAnimInsts[doorAnimInstIndex];
				doorAnimPercent = doorAnimInst.percentOpen;
			}
		}

		VoxelChasmDefID chasmDefID;
		const bool isChasm = voxelChunk.tryGetChasmDefID(voxel.x, voxel.y, voxel.z, &chasmDefID);

		const VoxelChasmDefinition *chasmDef = nullptr;
		bool isAnimatingChasm = false;
		bool isEmissiveChasm = false;
		bool hasChasmWall = false;
		IndexBufferID chasmWallIndexBufferID = -1;
		if (isChasm)
		{
			chasmDef = &voxelChunkManager.getChasmDef(chasmDefID);
			isAnimatingChasm = chasmDef->animType == VoxelChasmAnimationType::Animated;
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
			Span<const VoxelFadeAnimationInstance> fadeAnimInsts = voxelChunk.getFadeAnimInsts();
			fadeAnimInst = &fadeAnimInsts[fadeAnimInstIndex];
			isFading = !fadeAnimInst->isDoneFading();
		}

		const RenderLightIdList &voxelLightIdList = renderLightChunk.lightIdLists.get(voxel.x, voxel.y, voxel.z);

		constexpr int maxTransformsPerVoxel = VoxelDoorUtils::FACE_COUNT;
		constexpr int maxDrawCallsPerVoxel = RenderVoxelMeshInstance::MAX_DRAW_CALLS;

		struct DrawCallTransformInitInfo
		{
			UniformBufferID id;
			int index;
			UniformBufferID preScaleTranslationBufferID;
		};

		struct DrawCallMeshInitInfo
		{
			VertexPositionBufferID positionBufferID;
			UniformBufferID normalBufferID;
			UniformBufferID texCoordBufferID;
			IndexBufferID indexBufferID;
		};

		struct DrawCallTextureInitInfo
		{
			ObjectTextureID id0, id1;
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

			const UniformBufferID preScaleTranslationBufferID = (doorDef->type == ArenaDoorType::Raising) ? this->raisingDoorPreScaleTranslationBufferID : -1;
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
			transformInitInfo.index = GetVoxelRenderTransformIndex(voxel.x, voxel.y, voxel.z, renderChunk.height);
			transformInitInfo.preScaleTranslationBufferID = -1;
			transformInitInfoCount = 1;
		}

		DrawCallMeshInitInfo meshInitInfos[maxDrawCallsPerVoxel];
		int meshInitInfoCount = 0;
		if (isDoor)
		{
			DrawCallMeshInitInfo &doorMeshInitInfo = meshInitInfos[0];
			doorMeshInitInfo.positionBufferID = renderMeshInst.positionBufferID;
			doorMeshInitInfo.normalBufferID = renderMeshInst.normalBufferID;
			doorMeshInitInfo.texCoordBufferID = renderMeshInst.texCoordBufferID;
			doorMeshInitInfo.indexBufferID = renderMeshInst.indexBufferIDs[0];
			meshInitInfoCount = 1;
		}
		else if (isChasm)
		{
			DrawCallMeshInitInfo &chasmFloorMeshInitInfo = meshInitInfos[0];
			chasmFloorMeshInitInfo.positionBufferID = renderMeshInst.positionBufferID;
			chasmFloorMeshInitInfo.normalBufferID = renderMeshInst.normalBufferID;
			chasmFloorMeshInitInfo.texCoordBufferID = renderMeshInst.texCoordBufferID;
			chasmFloorMeshInitInfo.indexBufferID = renderMeshInst.indexBufferIDs[0];
			meshInitInfoCount = 1;

			if (hasChasmWall)
			{
				DrawCallMeshInitInfo &chasmWallMeshInitInfo = meshInitInfos[1];
				chasmWallMeshInitInfo.positionBufferID = renderMeshInst.positionBufferID;
				chasmWallMeshInitInfo.normalBufferID = renderMeshInst.normalBufferID;
				chasmWallMeshInitInfo.texCoordBufferID = renderMeshInst.texCoordBufferID;
				chasmWallMeshInitInfo.indexBufferID = chasmWallIndexBufferID;
				meshInitInfoCount = 2;
			}
		}
		else
		{
			for (int i = 0; i < renderMeshInst.indexBufferIdCount; i++)
			{
				DebugAssertIndex(meshInitInfos, i);
				DrawCallMeshInitInfo &meshInitInfo = meshInitInfos[i];
				meshInitInfo.positionBufferID = renderMeshInst.positionBufferID;
				meshInitInfo.normalBufferID = renderMeshInst.normalBufferID;
				meshInitInfo.texCoordBufferID = renderMeshInst.texCoordBufferID;
				meshInitInfo.indexBufferID = renderMeshInst.indexBufferIDs[i];
			}

			meshInitInfoCount = renderMeshInst.indexBufferIdCount;
		}

		DrawCallTextureInitInfo textureInitInfos[maxDrawCallsPerVoxel];
		int textureInitInfoCount = 0;
		if (isDoor)
		{
			DrawCallTextureInitInfo &doorTextureInitInfo = textureInitInfos[0];
			doorTextureInitInfo.id0 = this->getTextureID(voxelTextureDef.getTextureAsset(0));
			doorTextureInitInfo.id1 = -1;
			textureInitInfoCount = 1;
		}
		else if (isChasm)
		{
			const ObjectTextureID chasmFloorTextureID = this->getChasmFloorTextureID(chasmDefID);
			const ObjectTextureID chasmWallTextureID = this->getChasmWallTextureID(chasmDefID);

			DrawCallTextureInitInfo &chasmFloorTextureInitInfo = textureInitInfos[0];
			chasmFloorTextureInitInfo.id0 = chasmFloorTextureID;
			chasmFloorTextureInitInfo.id1 = -1;

			DrawCallTextureInitInfo &chasmWallTextureInitInfo = textureInitInfos[1];
			chasmWallTextureInitInfo.id0 = chasmFloorTextureID;
			chasmWallTextureInitInfo.id1 = chasmWallTextureID;

			textureInitInfoCount = 2;
		}
		else
		{
			for (int i = 0; i < voxelTextureDef.textureCount; i++)
			{
				const TextureAsset &textureAsset = voxelTextureDef.getTextureAsset(i);

				DebugAssertIndex(textureInitInfos, i);
				DrawCallTextureInitInfo &textureInitInfo = textureInitInfos[i];
				textureInitInfo.id0 = this->getTextureID(textureAsset);
				textureInitInfo.id1 = -1;
			}

			textureInitInfoCount = voxelTextureDef.textureCount;
		}

		DrawCallShadingInitInfo shadingInitInfos[maxDrawCallsPerVoxel];
		int shadingInitInfoCount = 0;
		if (isDoor)
		{
			DebugAssert(voxelShadingDef.pixelShaderCount == 1);

			DrawCallShadingInitInfo &doorShadingInitInfo = shadingInitInfos[0];
			doorShadingInitInfo.vertexShaderType = voxelShadingDef.vertexShaderType;
			doorShadingInitInfo.pixelShaderType = voxelShadingDef.pixelShaderTypes[0];

			const ArenaDoorType doorType = doorDef->type;
			switch (doorType)
			{
			case ArenaDoorType::Swinging:
				doorShadingInitInfo.pixelShaderParam0 = 0.0;
				break;
			case ArenaDoorType::Sliding:
				doorShadingInitInfo.pixelShaderParam0 = VoxelDoorUtils::getAnimatedTexCoordPercent(doorAnimPercent);
				break;
			case ArenaDoorType::Raising:
				doorShadingInitInfo.pixelShaderParam0 = VoxelDoorUtils::getAnimatedTexCoordPercent(doorAnimPercent);
				break;
			case ArenaDoorType::Splitting:
				doorShadingInitInfo.pixelShaderParam0 = VoxelDoorUtils::getAnimatedTexCoordPercent(doorAnimPercent);
				break;
			default:
				DebugNotImplementedMsg(std::to_string(static_cast<int>(doorType)));
				break;
			}

			shadingInitInfoCount = 1;
		}
		else if (isChasm)
		{
			DebugAssert(voxelShadingDef.pixelShaderCount == 2);

			DrawCallShadingInitInfo &chasmFloorShadingInitInfo = shadingInitInfos[0];
			chasmFloorShadingInitInfo.vertexShaderType = voxelShadingDef.vertexShaderType;
			chasmFloorShadingInitInfo.pixelShaderType = voxelShadingDef.pixelShaderTypes[0];
			chasmFloorShadingInitInfo.pixelShaderParam0 = 0.0;

			DrawCallShadingInitInfo &chasmWallShadingInitInfo = shadingInitInfos[1];
			chasmWallShadingInitInfo.vertexShaderType = voxelShadingDef.vertexShaderType;
			chasmWallShadingInitInfo.pixelShaderType = voxelShadingDef.pixelShaderTypes[1];
			chasmWallShadingInitInfo.pixelShaderParam0 = 0.0;

			shadingInitInfoCount = 2;
		}
		else
		{
			for (int i = 0; i < renderMeshInst.indexBufferIdCount; i++)
			{
				DebugAssertIndex(shadingInitInfos, i);
				DrawCallShadingInitInfo &shadingInitInfo = shadingInitInfos[i];
				shadingInitInfo.vertexShaderType = voxelShadingDef.vertexShaderType;

				DebugAssertIndex(voxelMeshDef.textureSlotIndices, i);
				const int textureSlotIndex = voxelMeshDef.textureSlotIndices[i];
				shadingInitInfo.pixelShaderType = voxelShadingDef.pixelShaderTypes[textureSlotIndex];

				shadingInitInfo.pixelShaderParam0 = 0.0;
			}

			shadingInitInfoCount = renderMeshInst.indexBufferIdCount;
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

			Span<const RenderLightID> voxelLightIDs = voxelLightIdList.getLightIDs();
			DebugAssert(std::size(lightingInitInfo.ids) >= voxelLightIDs.getCount());
			std::copy(voxelLightIDs.begin(), voxelLightIDs.end(), std::begin(lightingInitInfo.ids));
			lightingInitInfo.idCount = voxelLightIDs.getCount();
		}

		bool visibleDoorFaces[VoxelDoorUtils::FACE_COUNT];
		int drawCallCount = 0;
		if (isDoor)
		{
			int doorVisInstIndex;
			if (!voxelChunk.tryGetDoorVisibilityInstIndex(voxel.x, voxel.y, voxel.z, &doorVisInstIndex))
			{
				DebugLogError("Expected door visibility instance at (" + voxel.toString() + ") in chunk (" + chunkPos.toString() + ").");
				continue;
			}

			Span<const VoxelDoorVisibilityInstance> doorVisInsts = voxelChunk.getDoorVisibilityInsts();
			const VoxelDoorVisibilityInstance &doorVisInst = doorVisInsts[doorVisInstIndex];
			std::fill(std::begin(visibleDoorFaces), std::end(visibleDoorFaces), false);
			for (size_t i = 0; i < std::size(visibleDoorFaces); i++)
			{
				const VoxelFacing2D doorFacing = VoxelDoorUtils::Facings[i];
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

			// Handle "closets" with three walled sides, causing zero visible faces when camera is on one side
			// (happens on second level of northeasternmost province's map dungeon).
			if (drawCallCount == 0)
			{
				continue;
			}
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
		Span<RenderDrawCall> drawCalls = drawCallHeap.get(drawCallRangeID);

		if (isDoor)
		{
			DebugAssert(transformInitInfoCount == VoxelDoorUtils::FACE_COUNT);

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
				doorDrawCall.positionBufferID = doorMeshInitInfo.positionBufferID;
				doorDrawCall.normalBufferID = doorMeshInitInfo.normalBufferID;
				doorDrawCall.texCoordBufferID = doorMeshInitInfo.texCoordBufferID;
				doorDrawCall.indexBufferID = doorMeshInitInfo.indexBufferID;
				doorDrawCall.textureIDs[0] = doorTextureInitInfo.id0;
				doorDrawCall.textureIDs[1] = doorTextureInitInfo.id1;
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
				chasmDrawCall.positionBufferID = chasmMeshInitInfo.positionBufferID;
				chasmDrawCall.normalBufferID = chasmMeshInitInfo.normalBufferID;
				chasmDrawCall.texCoordBufferID = chasmMeshInitInfo.texCoordBufferID;
				chasmDrawCall.indexBufferID = chasmMeshInitInfo.indexBufferID;
				chasmDrawCall.textureIDs[0] = chasmTextureInitInfo.id0;
				chasmDrawCall.textureIDs[1] = chasmTextureInitInfo.id1;
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
				const DrawCallTextureInitInfo &textureInitInfo = textureInitInfos[i];
				const DrawCallShadingInitInfo &shadingInitInfo = shadingInitInfos[i];

				RenderDrawCall &drawCall = drawCalls[i];
				drawCall.transformBufferID = transformInitInfo.id;
				drawCall.transformIndex = transformInitInfo.index;
				drawCall.preScaleTranslationBufferID = transformInitInfo.preScaleTranslationBufferID;
				drawCall.positionBufferID = meshInitInfo.positionBufferID;
				drawCall.normalBufferID = meshInitInfo.normalBufferID;
				drawCall.texCoordBufferID = meshInitInfo.texCoordBufferID;
				drawCall.indexBufferID = meshInitInfo.indexBufferID;
				drawCall.textureIDs[0] = textureInitInfo.id0;
				drawCall.textureIDs[1] = textureInitInfo.id1;
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

void RenderVoxelChunkManager::updateChunkCombinedVoxelDrawCalls(RenderVoxelChunk &renderChunk, Span<const VoxelInt3> dirtyVoxelPositions,
	const VoxelChunk &voxelChunk, const VoxelFaceCombineChunk &faceCombineChunk, const RenderLightChunk &renderLightChunk,
	const VoxelChunkManager &voxelChunkManager, double ceilingScale, double chasmAnimPercent, Renderer &renderer)
{
	// can't rely on VoxelFaceCombineResultID meaning the same thing - it may have been freed and immediately renewed in the same update()

	const ChunkInt2 chunkPos = renderChunk.position;
	RenderVoxelDrawCallHeap &chunkDrawCallHeap = renderChunk.drawCallHeap;

	const RecyclablePool<VoxelFaceCombineResultID, VoxelFaceCombineResult> &combinedFacesPool = faceCombineChunk.combinedFacesPool;
	int combinedFaceCount = combinedFacesPool.getTotalCount();
	for (int combinedFaceIndex = 0; combinedFaceIndex < combinedFaceCount; combinedFaceIndex++)
	{
		const VoxelFaceCombineResultID faceCombineResultID = static_cast<VoxelFaceCombineResultID>(combinedFaceIndex);
		const VoxelFaceCombineResult *faceCombineResult = combinedFacesPool.tryGet(faceCombineResultID);
		if (faceCombineResult == nullptr)
		{
			continue;
		}

		const VoxelInt3 minVoxel = faceCombineResult->min;
		const VoxelInt3 maxVoxel = faceCombineResult->max;
		const VoxelFacing3D facing = faceCombineResult->facing;
		bool shouldAllocateDrawCall = false;

		UniformBufferID transformBufferID = -1;

		RenderVoxelCombinedFaceTransformKey transformKey;
		transformKey.minVoxel = minVoxel;
		transformKey.maxVoxel = maxVoxel;
		transformKey.facing = facing;

		std::unordered_map<RenderVoxelCombinedFaceTransformKey, UniformBufferID> &chunkCombinedFaceTransforms = renderChunk.combinedFaceTransforms;
		const auto transformIter = chunkCombinedFaceTransforms.find(transformKey);
		if (transformIter != chunkCombinedFaceTransforms.end())
		{
			transformBufferID = transformIter->second;
		}
		else
		{
			// Create and reuse for any identical mesh at this spot in this chunk.
			transformBufferID = renderer.createUniformBuffer(1, sizeof(RenderTransform), alignof(RenderTransform));
			if (transformBufferID < 0)
			{
				DebugLogErrorFormat("Couldn't allocate combined face transform buffer starting at (%s) in chunk (%s).", minVoxel.toString().c_str(), chunkPos.toString().c_str());
			}

			const WorldInt3 meshMinVoxel = VoxelUtils::chunkVoxelToWorldVoxel(chunkPos, minVoxel);
			const WorldDouble3 meshPosition(
				static_cast<SNDouble>(meshMinVoxel.x),
				static_cast<double>(meshMinVoxel.y) * ceilingScale,
				static_cast<WEDouble>(meshMinVoxel.z));

			RenderTransform transform;
			transform.translation = Matrix4d::translation(meshPosition.x, meshPosition.y, meshPosition.z);
			transform.rotation = Matrix4d::identity();
			transform.scale = Matrix4d::identity();
			renderer.populateUniformBuffer(transformBufferID, transform);

			chunkCombinedFaceTransforms.emplace(transformKey, transformBufferID);

			// This mesh instance needs a draw call since its transform is newly made.
			shouldAllocateDrawCall = true;
		}

		if (!shouldAllocateDrawCall)
		{
			continue;
		}

		// Find model space vertex buffer matching this voxel span (also need scale type in case of chasms etc).
		int quadVoxelWidth;
		int quadVoxelHeight;
		MeshUtils::getVoxelFaceDimensions(minVoxel, maxVoxel, facing, &quadVoxelWidth, &quadVoxelHeight);

		const VoxelShapeDefID shapeDefID = voxelChunk.getShapeDefID(minVoxel.x, minVoxel.y, minVoxel.z);
		const VoxelShapeDefinition &shapeDef = voxelChunk.getShapeDef(shapeDefID);
		const VoxelMeshDefinition &meshDef = shapeDef.mesh;
		const VoxelShapeScaleType scaleType = shapeDef.scaleType;

		DebugAssert(shapeDef.allowsAdjacentFaceCombining);

		const auto vertexBufferIter = std::find_if(this->combinedFaceVertexBuffers.begin(), this->combinedFaceVertexBuffers.end(),
			[quadVoxelWidth, quadVoxelHeight, shapeDefID, facing](const RenderVoxelCombinedFaceVertexBuffer &curEntry)
		{
			return (curEntry.voxelWidth == quadVoxelWidth) && (curEntry.voxelHeight == quadVoxelHeight) && (curEntry.shapeDefID == shapeDefID) && (curEntry.facing == facing);
		});

		RenderVoxelCombinedFaceVertexBuffer *combinedFaceVertexBuffer = nullptr;
		if (vertexBufferIter != this->combinedFaceVertexBuffers.end())
		{
			combinedFaceVertexBuffer = &(*vertexBufferIter);
		}
		else
		{
			this->combinedFaceVertexBuffers.emplace_back(std::move(RenderVoxelCombinedFaceVertexBuffer()));
			combinedFaceVertexBuffer = &this->combinedFaceVertexBuffers.back();

			const int faceIndexBufferIndex = meshDef.findIndexBufferIndexWithFacing(facing);
			DebugAssert(faceIndexBufferIndex >= 0);
			Span<const int32_t> faceIndicesList = meshDef.indicesLists[faceIndexBufferIndex];

			constexpr int quadVertexCount = MeshUtils::VERTICES_PER_QUAD;
			int32_t faceVertexIndices[quadVertexCount];
			std::fill(std::begin(faceVertexIndices), std::end(faceVertexIndices), -1);
			MeshUtils::writeFirstFourUniqueIndices(faceIndicesList, faceVertexIndices);

			const Double3 quadVoxelDimsReal(
				1.0 + static_cast<SNDouble>(maxVoxel.x - minVoxel.x),
				1.0 + static_cast<double>(maxVoxel.y - minVoxel.y),
				1.0 + static_cast<WEDouble>(maxVoxel.z - minVoxel.z));

			// Transform the first voxel's vertices for this combined face.
			double quadVertexPositions[quadVertexCount * MeshUtils::POSITION_COMPONENTS_PER_VERTEX];
			double quadVertexNormals[quadVertexCount * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX];
			double quadVertexTexCoords[quadVertexCount * MeshUtils::TEX_COORD_COMPONENTS_PER_VERTEX];
			for (int vertexIndex = 0; vertexIndex < quadVertexCount; vertexIndex++)
			{
				const int32_t sourceVertexIndex = faceVertexIndices[vertexIndex];

				const int sourcePositionComponentIndex = sourceVertexIndex * MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
				const int destinationPositionComponentIndex = vertexIndex * MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
				const double sourcePositionX = meshDef.rendererPositions[sourcePositionComponentIndex];
				const double sourcePositionY = meshDef.rendererPositions[sourcePositionComponentIndex + 1];
				const double sourcePositionZ = meshDef.rendererPositions[sourcePositionComponentIndex + 2];
				const double sourcePositionScaledX = sourcePositionX * quadVoxelDimsReal.x;
				const double sourcePositionScaledY = MeshUtils::getScaledVertexY(sourcePositionY * quadVoxelDimsReal.y, scaleType, ceilingScale);
				const double sourcePositionScaledZ = sourcePositionZ * quadVoxelDimsReal.z;
				quadVertexPositions[destinationPositionComponentIndex] = sourcePositionScaledX;
				quadVertexPositions[destinationPositionComponentIndex + 1] = sourcePositionScaledY;
				quadVertexPositions[destinationPositionComponentIndex + 2] = sourcePositionScaledZ;

				const int sourceNormalComponentIndex = sourceVertexIndex * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
				const int destinationNormalComponentIndex = vertexIndex * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
				quadVertexNormals[destinationNormalComponentIndex] = meshDef.rendererNormals[sourceNormalComponentIndex];
				quadVertexNormals[destinationNormalComponentIndex + 1] = meshDef.rendererNormals[sourceNormalComponentIndex + 1];
				quadVertexNormals[destinationNormalComponentIndex + 2] = meshDef.rendererNormals[sourceNormalComponentIndex + 2];

				const int sourceTexCoordComponentIndex = sourceVertexIndex * MeshUtils::TEX_COORD_COMPONENTS_PER_VERTEX;
				const int destinationTexCoordComponentIndex = vertexIndex * MeshUtils::TEX_COORD_COMPONENTS_PER_VERTEX;
				quadVertexTexCoords[destinationTexCoordComponentIndex] = meshDef.rendererTexCoords[sourceTexCoordComponentIndex];
				quadVertexTexCoords[destinationTexCoordComponentIndex + 1] = meshDef.rendererTexCoords[sourceTexCoordComponentIndex + 1];
			}

			combinedFaceVertexBuffer->voxelWidth = quadVoxelWidth;
			combinedFaceVertexBuffer->voxelHeight = quadVoxelHeight;
			combinedFaceVertexBuffer->shapeDefID = shapeDefID;
			combinedFaceVertexBuffer->facing = facing;

			combinedFaceVertexBuffer->positionBufferID = renderer.createVertexPositionBuffer(quadVertexCount, MeshUtils::POSITION_COMPONENTS_PER_VERTEX);
			if (combinedFaceVertexBuffer->positionBufferID < 0)
			{
				DebugLogErrorFormat("Couldn't allocate combined face vertex position buffer starting at (%s) in chunk (%s).", minVoxel.toString().c_str(), chunkPos.toString().c_str());
			}

			combinedFaceVertexBuffer->normalBufferID = renderer.createVertexAttributeBuffer(quadVertexCount, MeshUtils::NORMAL_COMPONENTS_PER_VERTEX);
			if (combinedFaceVertexBuffer->normalBufferID < 0)
			{
				DebugLogErrorFormat("Couldn't allocate combined face vertex normal buffer starting at (%s) in chunk (%s).", minVoxel.toString().c_str(), chunkPos.toString().c_str());
			}

			combinedFaceVertexBuffer->texCoordBufferID = renderer.createVertexAttributeBuffer(quadVertexCount, MeshUtils::TEX_COORD_COMPONENTS_PER_VERTEX);
			if (combinedFaceVertexBuffer->texCoordBufferID < 0)
			{
				DebugLogErrorFormat("Couldn't allocate combined face vertex tex coords buffer starting at (%s) in chunk (%s).", minVoxel.toString().c_str(), chunkPos.toString().c_str());
			}

			renderer.populateVertexPositionBuffer(combinedFaceVertexBuffer->positionBufferID, quadVertexPositions);
			renderer.populateVertexAttributeBuffer(combinedFaceVertexBuffer->normalBufferID, quadVertexNormals);
			renderer.populateVertexAttributeBuffer(combinedFaceVertexBuffer->texCoordBufferID, quadVertexTexCoords);
		}

		VoxelChasmDefID chasmDefID;
		bool isChasm = voxelChunk.tryGetChasmDefID(minVoxel.x, minVoxel.y, minVoxel.z, &chasmDefID);

		// Use the texture/shading values of the first voxel.
		const int textureSlotIndex = meshDef.findTextureSlotIndexWithFacing(facing);
		ObjectTextureID textureID0 = -1;
		ObjectTextureID textureID1 = -1;
		if (isChasm)
		{
			const bool isChasmFloor = facing == VoxelFacing3D::NegativeY;

			textureID0 = this->getChasmFloorTextureID(chasmDefID);
			if (!isChasmFloor)
			{
				textureID1 = this->getChasmWallTextureID(chasmDefID);
			}
		}
		else
		{
			const VoxelTextureDefID textureDefID = voxelChunk.getTextureDefID(minVoxel.x, minVoxel.y, minVoxel.z);
			const VoxelTextureDefinition &textureDef = voxelChunk.getTextureDef(textureDefID);
			const TextureAsset &textureAsset = textureDef.getTextureAsset(textureSlotIndex);
			textureID0 = this->getTextureID(textureAsset);
		}

		const VoxelShadingDefID shadingDefID = voxelChunk.getShadingDefID(minVoxel.x, minVoxel.y, minVoxel.z);
		const VoxelShadingDefinition &shadingDef = voxelChunk.getShadingDef(shadingDefID);
		DebugAssertIndex(shadingDef.pixelShaderTypes, textureSlotIndex);
		DebugAssert(textureSlotIndex < shadingDef.pixelShaderCount);
		const PixelShaderType pixelShaderType = shadingDef.pixelShaderTypes[textureSlotIndex];

		// @todo solve lights per mesh in RenderLightChunk :O as a temporary fix, could use lights in minVoxel
		const RenderLightingType dummyLightingType = RenderLightingType::PerMesh;
		constexpr double dummyLightIntensity = 1.0;

		constexpr int drawCallCount = 1;
		const RenderVoxelDrawCallRangeID drawCallRangeID = chunkDrawCallHeap.alloc(drawCallCount);
		renderChunk.combinedFaceDrawCallRangeIDs.emplace_back(drawCallRangeID);
		Span<RenderDrawCall> drawCalls = chunkDrawCallHeap.get(drawCallRangeID);

		RenderDrawCall &drawCall = drawCalls[0];
		drawCall.transformBufferID = transformBufferID;
		drawCall.transformIndex = 0;
		drawCall.preScaleTranslationBufferID = -1;
		drawCall.positionBufferID = combinedFaceVertexBuffer->positionBufferID;
		drawCall.normalBufferID = combinedFaceVertexBuffer->normalBufferID;
		drawCall.texCoordBufferID = combinedFaceVertexBuffer->texCoordBufferID;
		drawCall.indexBufferID = this->defaultQuadIndexBufferID;
		drawCall.textureIDs[0] = textureID0;
		drawCall.textureIDs[1] = textureID1;
		drawCall.vertexShaderType = shadingDef.vertexShaderType;
		drawCall.pixelShaderType = pixelShaderType;
		drawCall.pixelShaderParam0 = 0.0;
		drawCall.lightingType = dummyLightingType;
		drawCall.lightPercent = dummyLightIntensity;
		//std::copy(std::begin(lightingInitInfo.ids), std::end(lightingInitInfo.ids), std::begin(drawCall.lightIDs));
		drawCall.lightIdCount = 0;
		drawCall.enableDepthRead = true;
		drawCall.enableDepthWrite = true;
	}
}

void RenderVoxelChunkManager::rebuildDrawCallsList(const VoxelFrustumCullingChunkManager &voxelFrustumCullingChunkManager)
{
	this->drawCallsCache.clear();

	for (int i = 0; i < static_cast<int>(this->activeChunks.size()); i++)
	{
		const ChunkPtr &chunkPtr = this->activeChunks[i];
		const RenderVoxelChunk &renderChunk = *chunkPtr;
		const VoxelFrustumCullingChunk &voxelFrustumCullingChunk = voxelFrustumCullingChunkManager.getChunkAtIndex(i);
		const VisibilityType rootVisibilityType = voxelFrustumCullingChunk.getRootVisibilityType();
		const bool anyVisibleLeafNodes = rootVisibilityType != VisibilityType::Outside;
		if (!anyVisibleLeafNodes)
		{
			continue;
		}

		Span<const RenderVoxelDrawCallRangeID> combinedFaceDrawCallRangeIDs = renderChunk.combinedFaceDrawCallRangeIDs;
		for (const RenderVoxelDrawCallRangeID rangeID : combinedFaceDrawCallRangeIDs)
		{
			// @todo check visibility types of all voxel columns for this combined face, probably need a struct w/ VoxelFaceCombineResultID and this rangeID

			const Span<const RenderDrawCall> drawCalls = renderChunk.drawCallHeap.get(rangeID);
			this->drawCallsCache.insert(this->drawCallsCache.end(), drawCalls.begin(), drawCalls.end());
		}

		/*Span3D<const RenderVoxelDrawCallRangeID> rangeIDs = renderChunk.drawCallRangeIDs;
		for (WEInt z = 0; z < rangeIDs.getDepth(); z++)
		{
			for (SNInt x = 0; x < rangeIDs.getWidth(); x++)
			{
				const int visibilityLeafNodeIndex = x + (z * rangeIDs.getWidth());
				DebugAssertIndex(voxelFrustumCullingChunk.leafNodeFrustumTests, visibilityLeafNodeIndex);
				const bool isVoxelColumnVisible = voxelFrustumCullingChunk.leafNodeFrustumTests[visibilityLeafNodeIndex];
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
		}*/
	}
}

void RenderVoxelChunkManager::populateCommandBuffer(RenderCommandBuffer &commandBuffer) const
{
	commandBuffer.addDrawCalls(this->drawCallsCache);
}

void RenderVoxelChunkManager::updateActiveChunks(Span<const ChunkInt2> newChunkPositions, Span<const ChunkInt2> freedChunkPositions,
	const VoxelChunkManager &voxelChunkManager, Renderer &renderer)
{
	for (const ChunkInt2 chunkPos : freedChunkPositions)
	{
		const int chunkIndex = this->getChunkIndex(chunkPos);
		RenderVoxelChunk &renderChunk = this->getChunkAtIndex(chunkIndex);
		renderChunk.freeBuffers(renderer);
		this->recycleChunk(chunkIndex);
	}

	for (const ChunkInt2 chunkPos : newChunkPositions)
	{
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);

		const int spawnIndex = this->spawnChunk();
		RenderVoxelChunk &renderChunk = this->getChunkAtIndex(spawnIndex);
		renderChunk.init(chunkPos, voxelChunk.height);
	}

	// Free any unneeded chunks for memory savings in case the chunk distance was once large
	// and is now small. This is significant even for chunk distance 2->1, or 25->9 chunks.
	this->chunkPool.clear();
}

void RenderVoxelChunkManager::update(Span<const ChunkInt2> activeChunkPositions, Span<const ChunkInt2> newChunkPositions,
	double ceilingScale, double chasmAnimPercent, const VoxelChunkManager &voxelChunkManager, const VoxelFaceCombineChunkManager &voxelFaceCombineChunkManager,
	const VoxelFrustumCullingChunkManager &voxelFrustumCullingChunkManager, const RenderLightChunkManager &renderLightChunkManager,
	TextureManager &textureManager, Renderer &renderer)
{
	// Update pre-scale transition used by all raising doors (ideally this would be once on scene change).
	const Double3 raisingDoorPreScaleTranslation = MakeRaisingDoorPreScaleTranslation(ceilingScale);
	renderer.populateUniformBuffer(this->raisingDoorPreScaleTranslationBufferID, raisingDoorPreScaleTranslation);

	for (const ChunkInt2 chunkPos : newChunkPositions)
	{
		RenderVoxelChunk &renderChunk = this->getChunkAtPosition(chunkPos);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		const VoxelFrustumCullingChunk &voxelFrustumCullingChunk = voxelFrustumCullingChunkManager.getChunkAtPosition(chunkPos);
		this->loadMeshBuffers(renderChunk, voxelChunk, ceilingScale, renderer);
		this->loadChunkTextures(voxelChunk, voxelChunkManager, textureManager, renderer);
		this->loadChasmWalls(renderChunk, voxelChunk);
		this->loadTransforms(renderChunk, voxelChunk, ceilingScale, renderer);
	}

	for (const ChunkInt2 chunkPos : activeChunkPositions)
	{
		RenderVoxelChunk &renderChunk = this->getChunkAtPosition(chunkPos);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		const VoxelFrustumCullingChunk &voxelFrustumCullingChunk = voxelFrustumCullingChunkManager.getChunkAtPosition(chunkPos);
		const RenderLightChunk &renderLightChunk = renderLightChunkManager.getChunkAtPosition(chunkPos);

		Span<const VoxelInt3> dirtyChasmWallInstVoxels = voxelChunk.getDirtyChasmWallInstPositions();
		for (const VoxelInt3 chasmWallPos : dirtyChasmWallInstVoxels)
		{
			this->loadChasmWall(renderChunk, voxelChunk, chasmWallPos.x, chasmWallPos.y, chasmWallPos.z);
		}

		// Update door render transforms (rotation angle, etc.).
		Span<const VoxelInt3> dirtyDoorAnimInstVoxels = voxelChunk.getDirtyDoorAnimInstPositions();
		for (const VoxelInt3 doorVoxel : dirtyDoorAnimInstVoxels)
		{
			VoxelDoorDefID doorDefID;
			if (!voxelChunk.tryGetDoorDefID(doorVoxel.x, doorVoxel.y, doorVoxel.z, &doorDefID))
			{
				DebugLogError("Expected door def ID at (" + doorVoxel.toString() + ").");
				continue;
			}

			const VoxelDoorDefinition &doorDef = voxelChunk.getDoorDef(doorDefID);
			const ArenaDoorType doorType = doorDef.type;
			const WorldDouble3 worldPosition = MakeVoxelWorldPosition(voxelChunk.position, doorVoxel, ceilingScale);
			const double doorAnimPercent = VoxelDoorUtils::getAnimPercentOrZero(doorVoxel.x, doorVoxel.y, doorVoxel.z, voxelChunk);

			for (int i = 0; i < VoxelDoorUtils::FACE_COUNT; i++)
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
		Span<const VoxelInt3> dirtyShapeDefVoxels = voxelChunk.getDirtyShapeDefPositions();
		Span<const VoxelInt3> dirtyDoorVisInstVoxels = voxelChunk.getDirtyDoorVisInstPositions();
		Span<const VoxelInt3> dirtyFadeAnimInstVoxels = voxelChunk.getDirtyFadeAnimInstPositions();
		Span<const VoxelInt3> dirtyLightVoxels = renderLightChunk.dirtyVoxelPositions;
		//this->updateChunkVoxelDrawCalls(renderChunk, dirtyShapeDefVoxels, voxelChunk, renderLightChunk, voxelChunkManager, ceilingScale, chasmAnimPercent);
		//this->updateChunkVoxelDrawCalls(renderChunk, dirtyDoorAnimInstVoxels, voxelChunk, renderLightChunk, voxelChunkManager, ceilingScale, chasmAnimPercent);
		//this->updateChunkVoxelDrawCalls(renderChunk, dirtyDoorVisInstVoxels, voxelChunk, renderLightChunk, voxelChunkManager, ceilingScale, chasmAnimPercent);
		//this->updateChunkVoxelDrawCalls(renderChunk, dirtyFadeAnimInstVoxels, voxelChunk, renderLightChunk, voxelChunkManager, ceilingScale, chasmAnimPercent);
		//this->updateChunkVoxelDrawCalls(renderChunk, dirtyChasmWallInstVoxels, voxelChunk, renderLightChunk, voxelChunkManager, ceilingScale, chasmAnimPercent);
		//this->updateChunkVoxelDrawCalls(renderChunk, dirtyLightVoxels, voxelChunk, renderLightChunk, voxelChunkManager, ceilingScale, chasmAnimPercent);

		const VoxelFaceCombineChunk &faceCombineChunk = voxelFaceCombineChunkManager.getChunkAtPosition(chunkPos);
		this->updateChunkCombinedVoxelDrawCalls(renderChunk, dirtyShapeDefVoxels, voxelChunk, faceCombineChunk, renderLightChunk, voxelChunkManager, ceilingScale, chasmAnimPercent, renderer);
	}

	this->rebuildDrawCallsList(voxelFrustumCullingChunkManager);
}

void RenderVoxelChunkManager::endFrame()
{

}

void RenderVoxelChunkManager::unloadScene(Renderer &renderer)
{
	this->textures.clear();
	this->chasmFloorTextures.clear();
	this->chasmTextureKeys.clear();

	// Free vertex/attribute/index buffer IDs.
	for (int i = static_cast<int>(this->activeChunks.size()) - 1; i >= 0; i--)
	{
		ChunkPtr &chunkPtr = this->activeChunks[i];
		chunkPtr->freeBuffers(renderer);
		this->recycleChunk(i);
	}

	for (RenderVoxelCombinedFaceVertexBuffer &buffer : this->combinedFaceVertexBuffers)
	{
		if (buffer.positionBufferID >= 0)
		{
			renderer.freeVertexPositionBuffer(buffer.positionBufferID);
		}

		if (buffer.normalBufferID >= 0)
		{
			renderer.freeVertexAttributeBuffer(buffer.normalBufferID);
		}

		if (buffer.texCoordBufferID >= 0)
		{
			renderer.freeVertexAttributeBuffer(buffer.texCoordBufferID);
		}
	}

	this->combinedFaceVertexBuffers.clear();
	this->drawCallsCache.clear();
}
