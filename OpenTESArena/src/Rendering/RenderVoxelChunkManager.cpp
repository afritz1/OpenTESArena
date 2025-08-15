#include <algorithm>
#include <numeric>
#include <optional>

#include "RenderCommand.h"
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
	};

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
					DebugLogWarningFormat("Couldn't load voxel texture \"%s\".", textureAsset.filename.c_str());
					continue;
				}

				const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
				const ObjectTextureID textureID = renderer.createObjectTexture(textureBuilder.width, textureBuilder.height, textureBuilder.bytesPerTexel);
				if (textureID < 0)
				{
					DebugLogWarningFormat("Couldn't create voxel texture \"%s\".", textureAsset.filename.c_str());
					continue;
				}

				if (!renderer.populateObjectTexture(textureID, textureBuilder.bytes))
				{
					DebugLogWarningFormat("Couldn't populate voxel texture \"%s\".", textureAsset.filename.c_str());
				}

				ScopedObjectTextureRef voxelTextureRef(textureID, renderer);
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
				const uint8_t paletteIndex = chasmDef.solidColor.paletteIndex;
				Span<const uint8_t> srcTexel(&paletteIndex, 1);
				if (!renderer.populateObjectTexture8Bit(dryChasmTextureID, srcTexel))
				{
					DebugLogWarning("Couldn't populate dry chasm texture for writing.");
					return;
				}

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
				int newObjectTextureWidth = firstFrameTextureBuilder.width;
				int newObjectTextureHeight = firstFrameTextureBuilder.height * chasmDef.animated.textureAssets.getCount();

				constexpr int bytesPerTexel = 1;
				DebugAssert(firstFrameTextureBuilder.bytesPerTexel == bytesPerTexel);

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
					Span2D<const uint8_t> textureBuilderTexels = textureBuilder.getTexels8();
					const int dstByteOffset = (newObjectTextureCurrentY * newObjectTextureWidth) * bytesPerTexel;
					uint8_t *dstTexels = lockedTexture.getTexels8().begin();
					std::copy(textureBuilderTexels.begin(), textureBuilderTexels.end(), dstTexels + dstByteOffset);
					newObjectTextureCurrentY += firstFrameTextureBuilder.height;

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
}

void RenderVoxelChunkManager::init(Renderer &renderer)
{
	// Populate pre-scale translation transform (for raising doors).
	this->raisingDoorPreScaleTranslationBufferID = renderer.createUniformBufferVector3s(1);
	if (this->raisingDoorPreScaleTranslationBufferID < 0)
	{
		DebugLogError("Couldn't create uniform buffer for pre-scale translation.");
		return;
	}

	const Double3 preScaleTranslation = Double3::Zero; // Populated on scene change.
	renderer.populateUniformBufferVector3s(this->raisingDoorPreScaleTranslationBufferID, Span<const Double3>(&preScaleTranslation, 1));

	// Populate default quad indices for combined voxel faces.
	constexpr int indicesPerQuad = MeshUtils::INDICES_PER_QUAD;
	this->defaultQuadIndexBufferID = renderer.createIndexBuffer(indicesPerQuad);
	if (this->defaultQuadIndexBufferID < 0)
	{
		DebugLogError("Couldn't create default quad index buffer.");
		return;
	}

	renderer.populateIndexBuffer(this->defaultQuadIndexBufferID, MeshUtils::DefaultQuadVertexIndices);
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
	for (const VoxelTextureDefinition &voxelTextureDef : voxelChunk.textureDefs)
	{
		LoadVoxelDefTextures(voxelTextureDef, this->textures, textureManager, renderer);
	}

	for (int i = 0; i < voxelChunkManager.getChasmDefCount(); i++)
	{
		const VoxelChasmDefID chasmDefID = static_cast<VoxelChasmDefID>(i);
		LoadChasmDefTextures(chasmDefID, voxelChunkManager, this->textures, this->chasmFloorTextures, this->chasmTextureKeys, textureManager, renderer);
	}
}

void RenderVoxelChunkManager::loadChunkNonCombinedVoxelMeshBuffers(RenderVoxelChunk &renderChunk, const VoxelChunk &voxelChunk, double ceilingScale, Renderer &renderer)
{
	const ChunkInt2 chunkPos = voxelChunk.position;

	// Add render chunk voxel mesh instances and create mappings to them.
	Span<const VoxelShapeDefinition> shapeDefs = voxelChunk.shapeDefs;
	for (int shapeDefIndex = 0; shapeDefIndex < shapeDefs.getCount(); shapeDefIndex++)
	{
		const VoxelShapeDefID voxelShapeDefID = static_cast<VoxelShapeDefID>(shapeDefIndex);
		const VoxelShapeDefinition &voxelShapeDef = shapeDefs[voxelShapeDefID];
		if (voxelShapeDef.allowsAdjacentFaceCombining)
		{
			// Let combined face draw call generation create vertex buffers instead.
			continue;
		}

		const VoxelMeshDefinition &voxelMeshDef = voxelShapeDef.mesh;
		const bool isRenderMeshValid = !voxelMeshDef.isEmpty(); // Air has a shape for trigger voxels but no mesh
		if (!isRenderMeshValid)
		{
			continue;
		}

		RenderMeshInstance renderMeshInst;
		constexpr int positionComponentsPerVertex = MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
		constexpr int normalComponentsPerVertex = MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
		constexpr int texCoordComponentsPerVertex = MeshUtils::TEX_COORD_COMPONENTS_PER_VERTEX;

		const int vertexCount = MeshUtils::getVertexCount(voxelMeshDef.rendererPositions, MeshUtils::POSITION_COMPONENTS_PER_VERTEX);
		renderMeshInst.positionBufferID = renderer.createVertexPositionBuffer(vertexCount, positionComponentsPerVertex);
		if (renderMeshInst.positionBufferID < 0)
		{
			DebugLogError("Couldn't create vertex position buffer for voxel shape def ID " + std::to_string(voxelShapeDefID) + " in chunk (" + chunkPos.toString() + ").");
			continue;
		}

		renderMeshInst.normalBufferID = renderer.createVertexAttributeBuffer(vertexCount, normalComponentsPerVertex);
		if (renderMeshInst.normalBufferID < 0)
		{
			DebugLogError("Couldn't create vertex normal attribute buffer for voxel shape def ID " + std::to_string(voxelShapeDefID) + " in chunk (" + chunkPos.toString() + ").");
			renderMeshInst.freeBuffers(renderer);
			continue;
		}

		renderMeshInst.texCoordBufferID = renderer.createVertexAttributeBuffer(vertexCount, texCoordComponentsPerVertex);
		if (renderMeshInst.texCoordBufferID < 0)
		{
			DebugLogError("Couldn't create vertex tex coord attribute buffer for voxel shape def ID " + std::to_string(voxelShapeDefID) + " in chunk (" + chunkPos.toString() + ").");
			renderMeshInst.freeBuffers(renderer);
			continue;
		}

		// Populate renderer mesh geometry and indices from this voxel definition.
		renderer.populateVertexPositionBuffer(renderMeshInst.positionBufferID, voxelMeshDef.rendererPositions);
		renderer.populateVertexAttributeBuffer(renderMeshInst.normalBufferID, voxelMeshDef.rendererNormals);
		renderer.populateVertexAttributeBuffer(renderMeshInst.texCoordBufferID, voxelMeshDef.rendererTexCoords);

		// No longer supporting index buffer per face in one voxel -- this is just for doors and diagonals now which select one index buffer.
		DebugAssert(voxelMeshDef.indicesListCount >= 1);
		Span<const int32_t> indices = voxelMeshDef.indicesLists[0];
		const int indexCount = indices.getCount();

		renderMeshInst.indexBufferID = renderer.createIndexBuffer(indexCount);
		if (renderMeshInst.indexBufferID < 0)
		{
			DebugLogErrorFormat("Couldn't create index buffer for voxel shape def ID %d in chunk (%s).", voxelShapeDefID, voxelChunk.position.toString().c_str());
			renderMeshInst.freeBuffers(renderer);
			continue;
		}

		renderer.populateIndexBuffer(renderMeshInst.indexBufferID, indices);

		const RenderMeshInstID renderMeshInstID = renderChunk.addMeshInst(std::move(renderMeshInst));
		renderChunk.meshInstMappings.emplace(voxelShapeDefID, renderMeshInstID);
	}
}

void RenderVoxelChunkManager::loadTransforms(RenderVoxelChunk &renderChunk, const VoxelChunk &voxelChunk, double ceilingScale, Renderer &renderer)
{
	const int chunkHeight = voxelChunk.height;

	// Allocate one large uniform buffer that covers all voxels. Air is wasted and doors are double-allocated but this
	// is much faster than one buffer per voxel.
	const int chunkTransformsCount = Chunk::WIDTH * chunkHeight * Chunk::DEPTH;
	const UniformBufferID chunkTransformsBufferID = renderer.createUniformBufferRenderTransforms(chunkTransformsCount);
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
					const VoxelDoorDefinition &doorDef = voxelChunk.doorDefs[doorDefID];
					const ArenaDoorType doorType = doorDef.type;
					DebugAssert(renderChunk.doorTransformBuffers.find(voxel) == renderChunk.doorTransformBuffers.end());

					constexpr int doorFaceCount = VoxelDoorUtils::FACE_COUNT;

					// Each door voxel has a uniform buffer, one render transform per face.
					const UniformBufferID doorTransformBufferID = renderer.createUniformBufferRenderTransforms(doorFaceCount);
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
						renderer.populateUniformBufferIndexRenderTransform(doorTransformBufferID, i, faceRenderTransform);
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
					renderer.populateUniformBufferIndexRenderTransform(chunkTransformsBufferID, chunkTransformsBufferIndex, renderTransform);
				}
			}
		}
	}
}

void RenderVoxelChunkManager::updateChunkCombinedVoxelDrawCalls(RenderVoxelChunk &renderChunk, Span<const VoxelInt3> dirtyVoxelPositions,
	const VoxelChunk &voxelChunk, const VoxelFaceCombineChunk &faceCombineChunk, const VoxelChunkManager &voxelChunkManager,
	double ceilingScale, double chasmAnimPercent, Renderer &renderer)
{
	const ChunkInt2 chunkPos = renderChunk.position;
	std::unordered_map<VoxelFaceCombineResultID, RenderVoxelCombinedFaceDrawCallEntry> &combinedFaceDrawCallEntries = renderChunk.combinedFaceDrawCallEntries;
	RenderVoxelDrawCallHeap &chunkDrawCallHeap = renderChunk.drawCallHeap;

	// @todo the VoxelFaceCombineResultID should've been freed from this pool when the floor became a chasm, it's trying to use the old Top face of the floor for the chasm mesh indicesList lookup

	const RecyclablePool<VoxelFaceCombineResultID, VoxelFaceCombineResult> &combinedFacesPool = faceCombineChunk.combinedFacesPool;
	for (const VoxelFaceCombineResultID faceCombineResultID : combinedFacesPool.keys)
	{
		bool shouldAllocateDrawCall = !combinedFaceDrawCallEntries.contains(faceCombineResultID);
		if (!shouldAllocateDrawCall)
		{
			continue;
		}

		const VoxelFaceCombineResult &faceCombineResult = combinedFacesPool.get(faceCombineResultID);
		const VoxelInt3 minVoxel = faceCombineResult.min;
		const VoxelInt3 maxVoxel = faceCombineResult.max;
		const VoxelFacing3D facing = faceCombineResult.facing;

		// Create and populate transform buffer for this combined face.
		UniformBufferID transformBufferID = renderer.createUniformBufferRenderTransforms(1);
		if (transformBufferID < 0)
		{
			DebugLogErrorFormat("Couldn't allocate combined face transform buffer starting at (%s) in chunk (%s).", minVoxel.toString().c_str(), chunkPos.toString().c_str());
		}

		const WorldInt3 worldMinVoxel = VoxelUtils::chunkVoxelToWorldVoxel(chunkPos, minVoxel);
		const WorldDouble3 meshPosition(
			static_cast<SNDouble>(worldMinVoxel.x),
			static_cast<double>(worldMinVoxel.y) * ceilingScale,
			static_cast<WEDouble>(worldMinVoxel.z));

		RenderTransform transform;
		transform.translation = Matrix4d::translation(meshPosition.x, meshPosition.y, meshPosition.z);
		transform.rotation = Matrix4d::identity();
		transform.scale = Matrix4d::identity();
		renderer.populateUniformBufferRenderTransforms(transformBufferID, Span<const RenderTransform>(&transform, 1));

		const VoxelTraitsDefID traitsDefID = voxelChunk.traitsDefIDs.get(minVoxel.x, minVoxel.y, minVoxel.z);
		const VoxelTraitsDefinition &traitsDef = voxelChunk.traitsDefs[traitsDefID];
		const ArenaVoxelType voxelType = traitsDef.type;

		// Find model space vertex buffer matching this voxel span (also need scale type in case of chasms etc).
		int quadVoxelWidth;
		int quadVoxelHeight;
		MeshUtils::getVoxelFaceDimensions(minVoxel, maxVoxel, facing, voxelType, &quadVoxelWidth, &quadVoxelHeight);

		const VoxelShapeDefID shapeDefID = voxelChunk.shapeDefIDs.get(minVoxel.x, minVoxel.y, minVoxel.z);
		const VoxelShapeDefinition &shapeDef = voxelChunk.shapeDefs[shapeDefID];
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
				const double sourcePositionScaledY = sourcePositionY * quadVoxelDimsReal.y;
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
				const double sourceTexCoordU = meshDef.rendererTexCoords[sourceTexCoordComponentIndex];
				const double sourceTexCoordV = meshDef.rendererTexCoords[sourceTexCoordComponentIndex + 1];
				quadVertexTexCoords[destinationTexCoordComponentIndex] = sourceTexCoordU * (static_cast<double>(quadVoxelWidth) * Constants::JustBelowOne); // Keep between [0, 1)
				quadVertexTexCoords[destinationTexCoordComponentIndex + 1] = sourceTexCoordV * (static_cast<double>(quadVoxelHeight) * Constants::JustBelowOne);
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
		const VoxelChasmDefinition *chasmDef = nullptr;

		// Use the texture/shading values of the first voxel.
		const int textureSlotIndex = meshDef.findTextureSlotIndexWithFacing(facing);
		ObjectTextureID textureID0 = -1;
		ObjectTextureID textureID1 = -1;
		if (isChasm)
		{
			chasmDef = &voxelChunkManager.getChasmDef(chasmDefID);

			const bool isChasmFloor = facing == VoxelFacing3D::NegativeY;

			textureID0 = this->getChasmFloorTextureID(chasmDefID);
			if (!isChasmFloor)
			{
				textureID1 = this->getChasmWallTextureID(chasmDefID);
			}
		}
		else
		{
			const VoxelTextureDefID textureDefID = voxelChunk.textureDefIDs.get(minVoxel.x, minVoxel.y, minVoxel.z);
			const VoxelTextureDefinition &textureDef = voxelChunk.textureDefs[textureDefID];
			const TextureAsset &textureAsset = textureDef.getTextureAsset(textureSlotIndex);
			textureID0 = this->getTextureID(textureAsset);
		}

		const VoxelShadingDefID shadingDefID = voxelChunk.shadingDefIDs.get(minVoxel.x, minVoxel.y, minVoxel.z);
		const VoxelShadingDefinition &shadingDef = voxelChunk.shadingDefs[shadingDefID];
		DebugAssertIndex(shadingDef.pixelShaderTypes, textureSlotIndex);
		DebugAssert(textureSlotIndex < shadingDef.pixelShaderCount);
		const PixelShaderType pixelShaderType = shadingDef.pixelShaderTypes[textureSlotIndex];
		const double pixelShaderParam0 = 0.0;

		DrawCallLightingInitInfo lightingInitInfo;
		lightingInitInfo.type = RenderLightingType::PerPixel;
		lightingInitInfo.percent = 1.0;

		int fadeAnimInstIndex;
		if (voxelChunk.tryGetFadeAnimInstIndex(minVoxel.x, minVoxel.y, minVoxel.z, &fadeAnimInstIndex))
		{
			const VoxelFadeAnimationInstance &fadeAnimInst = voxelChunk.fadeAnimInsts[fadeAnimInstIndex];
			if (!fadeAnimInst.isDoneFading())
			{
				lightingInitInfo.type = RenderLightingType::PerMesh;
				lightingInitInfo.percent = std::clamp(1.0 - fadeAnimInst.percentFaded, 0.0, 1.0);
			}
		}
		else if (isChasm)
		{
			if (chasmDef->isEmissive)
			{
				lightingInitInfo.type = RenderLightingType::PerMesh;
				lightingInitInfo.percent = 1.0;
			}
		}

		constexpr int drawCallCount = 1;
		const RenderVoxelDrawCallRangeID drawCallRangeID = chunkDrawCallHeap.alloc(drawCallCount);

		RenderVoxelCombinedFaceDrawCallEntry combinedFaceDrawCallEntry;
		combinedFaceDrawCallEntry.rangeID = drawCallRangeID;
		combinedFaceDrawCallEntry.transformBufferID = transformBufferID;
		combinedFaceDrawCallEntry.min = minVoxel;
		combinedFaceDrawCallEntry.max = maxVoxel;
		combinedFaceDrawCallEntries.emplace(faceCombineResultID, std::move(combinedFaceDrawCallEntry));

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
		drawCall.pixelShaderParam0 = pixelShaderParam0;
		drawCall.lightingType = lightingInitInfo.type;
		drawCall.lightPercent = lightingInitInfo.percent;
		drawCall.enableBackFaceCulling = !shapeDef.allowsBackFaces;
		drawCall.enableDepthRead = true;
		drawCall.enableDepthWrite = true;
	}
}

void RenderVoxelChunkManager::updateChunkDiagonalVoxelDrawCalls(RenderVoxelChunk &renderChunk, Span<const VoxelInt3> dirtyVoxelPositions,
	const VoxelChunk &voxelChunk, const VoxelChunkManager &voxelChunkManager, double ceilingScale)
{
	// Diagonals are treated separately since they can't use the face combining algorithm.
	const ChunkInt2 chunkPos = renderChunk.position;
	RenderVoxelDrawCallHeap &drawCallHeap = renderChunk.drawCallHeap;

	// Regenerate all draw calls in the given dirty voxels.
	for (const VoxelInt3 voxel : dirtyVoxelPositions)
	{
		const VoxelTraitsDefID voxelTraitsDefID = voxelChunk.traitsDefIDs.get(voxel.x, voxel.y, voxel.z);
		const VoxelTraitsDefinition &voxelTraitsDef = voxelChunk.traitsDefs[voxelTraitsDefID];
		if (voxelTraitsDef.type != ArenaVoxelType::Diagonal)
		{
			continue;
		}

		renderChunk.freeDrawCalls(voxel.x, voxel.y, voxel.z);

		const VoxelShapeDefID voxelShapeDefID = voxelChunk.shapeDefIDs.get(voxel.x, voxel.y, voxel.z);
		const VoxelTextureDefID voxelTextureDefID = voxelChunk.textureDefIDs.get(voxel.x, voxel.y, voxel.z);
		const VoxelShadingDefID voxelShadingDefID = voxelChunk.shadingDefIDs.get(voxel.x, voxel.y, voxel.z);
		const VoxelShapeDefinition &voxelShapeDef = voxelChunk.shapeDefs[voxelShapeDefID];
		const VoxelMeshDefinition &voxelMeshDef = voxelShapeDef.mesh;
		const VoxelTextureDefinition &voxelTextureDef = voxelChunk.textureDefs[voxelTextureDefID];
		const VoxelShadingDefinition &voxelShadingDef = voxelChunk.shadingDefs[voxelShadingDefID];

		const auto meshInstIter = renderChunk.meshInstMappings.find(voxelShapeDefID);
		DebugAssert(meshInstIter != renderChunk.meshInstMappings.end());
		const RenderMeshInstID renderMeshInstID = meshInstIter->second;
		DebugAssertIndex(renderChunk.meshInsts, renderMeshInstID);
		const RenderMeshInstance &renderMeshInst = renderChunk.meshInsts[renderMeshInstID];

		const VoxelFadeAnimationInstance *fadeAnimInst = nullptr;
		bool isFading = false;
		int fadeAnimInstIndex;
		if (voxelChunk.tryGetFadeAnimInstIndex(voxel.x, voxel.y, voxel.z, &fadeAnimInstIndex))
		{
			fadeAnimInst = &voxelChunk.fadeAnimInsts[fadeAnimInstIndex];
			isFading = !fadeAnimInst->isDoneFading();
		}

		// Populate various init infos to be used for generating draw calls.
		DrawCallTransformInitInfo transformInitInfo;
		transformInitInfo.id = renderChunk.transformBufferID;
		transformInitInfo.index = GetVoxelRenderTransformIndex(voxel.x, voxel.y, voxel.z, renderChunk.height);
		transformInitInfo.preScaleTranslationBufferID = -1;

		DrawCallMeshInitInfo meshInitInfo;
		meshInitInfo.positionBufferID = renderMeshInst.positionBufferID;
		meshInitInfo.normalBufferID = renderMeshInst.normalBufferID;
		meshInitInfo.texCoordBufferID = renderMeshInst.texCoordBufferID;
		meshInitInfo.indexBufferID = renderMeshInst.indexBufferID;

		const TextureAsset &textureAsset = voxelTextureDef.getTextureAsset(0);
		DrawCallTextureInitInfo textureInitInfo;
		textureInitInfo.id0 = this->getTextureID(textureAsset);
		textureInitInfo.id1 = -1;

		DrawCallShadingInitInfo shadingInitInfo;
		shadingInitInfo.vertexShaderType = voxelShadingDef.vertexShaderType;

		const int textureSlotIndex = voxelMeshDef.textureSlotIndices[0];
		DebugAssertIndex(voxelShadingDef.pixelShaderTypes, textureSlotIndex);
		shadingInitInfo.pixelShaderType = voxelShadingDef.pixelShaderTypes[textureSlotIndex];
		shadingInitInfo.pixelShaderParam0 = 0.0;

		DrawCallLightingInitInfo lightingInitInfo;
		lightingInitInfo.type = RenderLightingType::PerPixel;
		lightingInitInfo.percent = 0.0;
		if (isFading)
		{
			lightingInitInfo.type = RenderLightingType::PerMesh;
			lightingInitInfo.percent = std::clamp(1.0 - fadeAnimInst->percentFaded, 0.0, 1.0);
		}

		DebugAssert(renderChunk.drawCallRangeIDs.get(voxel.x, voxel.y, voxel.z) == -1);
		const RenderVoxelDrawCallRangeID drawCallRangeID = drawCallHeap.alloc(1);
		renderChunk.drawCallRangeIDs.set(voxel.x, voxel.y, voxel.z, drawCallRangeID);

		RenderDrawCall &drawCall = drawCallHeap.get(drawCallRangeID)[0];
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
		drawCall.enableBackFaceCulling = !voxelShapeDef.allowsBackFaces;
		drawCall.enableDepthRead = true;
		drawCall.enableDepthWrite = true;
	}
}

void RenderVoxelChunkManager::updateChunkDoorVoxelDrawCalls(RenderVoxelChunk &renderChunk, Span<const VoxelInt3> dirtyVoxelPositions,
	const VoxelChunk &voxelChunk, const VoxelChunkManager &voxelChunkManager, double ceilingScale)
{
	const ChunkInt2 chunkPos = renderChunk.position;
	RenderVoxelDrawCallHeap &drawCallHeap = renderChunk.drawCallHeap;

	// Regenerate all draw calls in the given dirty voxels.
	for (const VoxelInt3 voxel : dirtyVoxelPositions)
	{
		VoxelDoorDefID doorDefID;
		if (!voxelChunk.tryGetDoorDefID(voxel.x, voxel.y, voxel.z, &doorDefID))
		{
			continue;
		}

		renderChunk.freeDrawCalls(voxel.x, voxel.y, voxel.z);

		const VoxelDoorDefinition &doorDef = voxelChunk.doorDefs[doorDefID];
		const VoxelShapeDefID voxelShapeDefID = voxelChunk.shapeDefIDs.get(voxel.x, voxel.y, voxel.z);
		const VoxelTextureDefID voxelTextureDefID = voxelChunk.textureDefIDs.get(voxel.x, voxel.y, voxel.z);
		const VoxelShadingDefID voxelShadingDefID = voxelChunk.shadingDefIDs.get(voxel.x, voxel.y, voxel.z);
		const VoxelShapeDefinition &voxelShapeDef = voxelChunk.shapeDefs[voxelShapeDefID];
		const VoxelMeshDefinition &voxelMeshDef = voxelShapeDef.mesh;
		const VoxelTextureDefinition &voxelTextureDef = voxelChunk.textureDefs[voxelTextureDefID];
		const VoxelShadingDefinition &voxelShadingDef = voxelChunk.shadingDefs[voxelShadingDefID];

		const auto meshInstIter = renderChunk.meshInstMappings.find(voxelShapeDefID);
		DebugAssert(meshInstIter != renderChunk.meshInstMappings.end());
		const RenderMeshInstID renderMeshInstID = meshInstIter->second;
		DebugAssertIndex(renderChunk.meshInsts, renderMeshInstID);
		const RenderMeshInstance &renderMeshInst = renderChunk.meshInsts[renderMeshInstID];

		double doorAnimPercent = 0.0;
		int doorAnimInstIndex;
		if (voxelChunk.tryGetDoorAnimInstIndex(voxel.x, voxel.y, voxel.z, &doorAnimInstIndex))
		{
			const VoxelDoorAnimationInstance &doorAnimInst = voxelChunk.doorAnimInsts[doorAnimInstIndex];
			doorAnimPercent = doorAnimInst.percentOpen;
		}

		// Populate various init infos to be used for generating draw calls.
		constexpr int doorTransformCount = VoxelDoorUtils::FACE_COUNT;
		DrawCallTransformInitInfo transformInitInfos[doorTransformCount];

		const auto transformIter = renderChunk.doorTransformBuffers.find(voxel);
		DebugAssert(transformIter != renderChunk.doorTransformBuffers.end());

		const UniformBufferID preScaleTranslationBufferID = (doorDef.type == ArenaDoorType::Raising) ? this->raisingDoorPreScaleTranslationBufferID : -1;
		for (int i = 0; i < doorTransformCount; i++)
		{
			DrawCallTransformInitInfo &doorTransformInitInfo = transformInitInfos[i];
			doorTransformInitInfo.id = transformIter->second;
			doorTransformInitInfo.index = i;
			doorTransformInitInfo.preScaleTranslationBufferID = preScaleTranslationBufferID;
		}

		DrawCallMeshInitInfo meshInitInfo;
		meshInitInfo.positionBufferID = renderMeshInst.positionBufferID;
		meshInitInfo.normalBufferID = renderMeshInst.normalBufferID;
		meshInitInfo.texCoordBufferID = renderMeshInst.texCoordBufferID;
		meshInitInfo.indexBufferID = renderMeshInst.indexBufferID;

		DrawCallTextureInitInfo textureInitInfo;
		textureInitInfo.id0 = this->getTextureID(voxelTextureDef.getTextureAsset(0));
		textureInitInfo.id1 = -1;

		DrawCallShadingInitInfo shadingInitInfo;
		DebugAssert(voxelShadingDef.pixelShaderCount == 1);
		shadingInitInfo.vertexShaderType = voxelShadingDef.vertexShaderType;
		shadingInitInfo.pixelShaderType = voxelShadingDef.pixelShaderTypes[0];

		const ArenaDoorType doorType = doorDef.type;
		switch (doorType)
		{
		case ArenaDoorType::Swinging:
			shadingInitInfo.pixelShaderParam0 = 0.0;
			break;
		case ArenaDoorType::Sliding:
			shadingInitInfo.pixelShaderParam0 = VoxelDoorUtils::getAnimatedTexCoordPercent(doorAnimPercent);
			break;
		case ArenaDoorType::Raising:
			shadingInitInfo.pixelShaderParam0 = VoxelDoorUtils::getAnimatedTexCoordPercent(doorAnimPercent);
			break;
		case ArenaDoorType::Splitting:
			shadingInitInfo.pixelShaderParam0 = VoxelDoorUtils::getAnimatedTexCoordPercent(doorAnimPercent);
			break;
		default:
			DebugNotImplementedMsg(std::to_string(static_cast<int>(doorType)));
			break;
		}

		DrawCallLightingInitInfo lightingInitInfo;
		lightingInitInfo.type = RenderLightingType::PerPixel;
		lightingInitInfo.percent = 0.0;

		bool visibleDoorFaces[VoxelDoorUtils::FACE_COUNT];
		int drawCallCount = 0;
		int doorVisInstIndex;
		if (!voxelChunk.tryGetDoorVisibilityInstIndex(voxel.x, voxel.y, voxel.z, &doorVisInstIndex))
		{
			DebugLogError("Expected door visibility instance at (" + voxel.toString() + ") in chunk (" + chunkPos.toString() + ").");
			continue;
		}

		const VoxelDoorVisibilityInstance &doorVisInst = voxelChunk.doorVisInsts[doorVisInstIndex];
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

		DebugAssert(renderChunk.drawCallRangeIDs.get(voxel.x, voxel.y, voxel.z) == -1);
		const RenderVoxelDrawCallRangeID drawCallRangeID = drawCallHeap.alloc(drawCallCount);
		renderChunk.drawCallRangeIDs.set(voxel.x, voxel.y, voxel.z, drawCallRangeID);
		Span<RenderDrawCall> drawCalls = drawCallHeap.get(drawCallRangeID);

		DebugAssert(doorTransformCount == VoxelDoorUtils::FACE_COUNT);

		int doorDrawCallWriteIndex = 0;
		for (int i = 0; i < doorTransformCount; i++)
		{
			if (!visibleDoorFaces[i])
			{
				continue;
			}

			const DrawCallTransformInitInfo &doorTransformInitInfo = transformInitInfos[i];

			RenderDrawCall &doorDrawCall = drawCalls[doorDrawCallWriteIndex];
			doorDrawCall.transformBufferID = doorTransformInitInfo.id;
			doorDrawCall.transformIndex = doorTransformInitInfo.index;
			doorDrawCall.preScaleTranslationBufferID = doorTransformInitInfo.preScaleTranslationBufferID;
			doorDrawCall.positionBufferID = meshInitInfo.positionBufferID;
			doorDrawCall.normalBufferID = meshInitInfo.normalBufferID;
			doorDrawCall.texCoordBufferID = meshInitInfo.texCoordBufferID;
			doorDrawCall.indexBufferID = meshInitInfo.indexBufferID;
			doorDrawCall.textureIDs[0] = textureInitInfo.id0;
			doorDrawCall.textureIDs[1] = textureInitInfo.id1;
			doorDrawCall.vertexShaderType = shadingInitInfo.vertexShaderType;
			doorDrawCall.pixelShaderType = shadingInitInfo.pixelShaderType;
			doorDrawCall.pixelShaderParam0 = shadingInitInfo.pixelShaderParam0;
			doorDrawCall.lightingType = lightingInitInfo.type;
			doorDrawCall.lightPercent = lightingInitInfo.percent;
			doorDrawCall.enableBackFaceCulling = true;
			doorDrawCall.enableDepthRead = true;
			doorDrawCall.enableDepthWrite = true;

			doorDrawCallWriteIndex++;
		}
	}
}

void RenderVoxelChunkManager::clearChunkCombinedVoxelDrawCalls(RenderVoxelChunk &renderChunk, Span<const VoxelFaceCombineResultID> dirtyFaceCombineResultIDs, Renderer &renderer)
{
	std::unordered_map<VoxelFaceCombineResultID, RenderVoxelCombinedFaceDrawCallEntry> &drawCallEntriesPool = renderChunk.combinedFaceDrawCallEntries;

	for (const VoxelFaceCombineResultID faceCombineResultID : dirtyFaceCombineResultIDs)
	{
		DebugAssert(faceCombineResultID >= 0);

		const auto iter = drawCallEntriesPool.find(faceCombineResultID);
		if (iter == drawCallEntriesPool.end())
		{
			continue;
		}

		RenderVoxelCombinedFaceDrawCallEntry &drawCallEntry = iter->second;
		renderChunk.drawCallHeap.free(drawCallEntry.rangeID);
		renderer.freeUniformBuffer(drawCallEntry.transformBufferID);

		drawCallEntriesPool.erase(iter);
	}
}

void RenderVoxelChunkManager::clearChunkNonCombinedVoxelDrawCalls(RenderVoxelChunk &renderChunk, Span<const VoxelInt3> dirtyVoxelPositions)
{
	for (const VoxelInt3 &voxel : dirtyVoxelPositions)
	{
		renderChunk.freeDrawCalls(voxel.x, voxel.y, voxel.z);
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

		const RenderVoxelDrawCallHeap &drawCallHeap = renderChunk.drawCallHeap;

		// Add draw calls that are at least partially in the camera frustum.
		for (const std::pair<VoxelFaceCombineResultID, RenderVoxelCombinedFaceDrawCallEntry> &pair : renderChunk.combinedFaceDrawCallEntries)
		{
			const RenderVoxelCombinedFaceDrawCallEntry &combinedFaceDrawCallEntry = pair.second;

			bool isCombinedFaceVisible = false;
			for (WEInt z = combinedFaceDrawCallEntry.min.z; z <= combinedFaceDrawCallEntry.max.z; z++)
			{
				for (SNInt x = combinedFaceDrawCallEntry.min.x; x <= combinedFaceDrawCallEntry.max.x; x++)
				{
					const int visibilityLeafNodeIndex = x + (z * Chunk::WIDTH);
					DebugAssertIndex(voxelFrustumCullingChunk.leafNodeFrustumTests, visibilityLeafNodeIndex);
					const bool isVoxelColumnVisible = voxelFrustumCullingChunk.leafNodeFrustumTests[visibilityLeafNodeIndex];
					if (isVoxelColumnVisible)
					{
						isCombinedFaceVisible = true;
						break;
					}
				}

				if (isCombinedFaceVisible)
				{
					break;
				}
			}

			if (isCombinedFaceVisible)
			{
				const Span<const RenderDrawCall> combinedFaceDrawCalls = drawCallHeap.get(combinedFaceDrawCallEntry.rangeID);
				DebugAssert(combinedFaceDrawCalls.getCount() == 1);
				this->drawCallsCache.emplace_back(combinedFaceDrawCalls[0]);
			}
		}

		Span3D<const RenderVoxelDrawCallRangeID> nonCombinedRangeIDs = renderChunk.drawCallRangeIDs;
		for (WEInt z = 0; z < nonCombinedRangeIDs.getDepth(); z++)
		{
			for (SNInt x = 0; x < nonCombinedRangeIDs.getWidth(); x++)
			{
				const int visibilityLeafNodeIndex = x + (z * Chunk::WIDTH);
				DebugAssertIndex(voxelFrustumCullingChunk.leafNodeFrustumTests, visibilityLeafNodeIndex);
				const bool isVoxelColumnVisible = voxelFrustumCullingChunk.leafNodeFrustumTests[visibilityLeafNodeIndex];
				if (isVoxelColumnVisible)
				{
					for (int y = 0; y < nonCombinedRangeIDs.getHeight(); y++)
					{
						const RenderVoxelDrawCallRangeID rangeID = nonCombinedRangeIDs.get(x, y, z);
						if (rangeID >= 0)
						{
							const Span<const RenderDrawCall> nonCombinedDrawCalls = drawCallHeap.get(rangeID);
							this->drawCallsCache.insert(this->drawCallsCache.end(), nonCombinedDrawCalls.begin(), nonCombinedDrawCalls.end());
						}
					}
				}
			}
		}
	}
}

void RenderVoxelChunkManager::populateCommandList(RenderCommandList &commandList) const
{
	commandList.addDrawCalls(this->drawCallsCache);
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
	const VoxelFrustumCullingChunkManager &voxelFrustumCullingChunkManager, TextureManager &textureManager, Renderer &renderer)
{
	// Update pre-scale transition used by all raising doors (ideally this would be once on scene change).
	const Double3 raisingDoorPreScaleTranslation = MakeRaisingDoorPreScaleTranslation(ceilingScale);
	renderer.populateUniformBufferVector3s(this->raisingDoorPreScaleTranslationBufferID, Span<const Double3>(&raisingDoorPreScaleTranslation, 1));

	for (const ChunkInt2 chunkPos : newChunkPositions)
	{
		RenderVoxelChunk &renderChunk = this->getChunkAtPosition(chunkPos);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		const VoxelFrustumCullingChunk &voxelFrustumCullingChunk = voxelFrustumCullingChunkManager.getChunkAtPosition(chunkPos);
		this->loadChunkNonCombinedVoxelMeshBuffers(renderChunk, voxelChunk, ceilingScale, renderer);
		this->loadChunkTextures(voxelChunk, voxelChunkManager, textureManager, renderer);
		this->loadTransforms(renderChunk, voxelChunk, ceilingScale, renderer);
	}

	for (const ChunkInt2 chunkPos : activeChunkPositions)
	{
		RenderVoxelChunk &renderChunk = this->getChunkAtPosition(chunkPos);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		const VoxelFrustumCullingChunk &voxelFrustumCullingChunk = voxelFrustumCullingChunkManager.getChunkAtPosition(chunkPos);

		// Update door render transforms (rotation angle, etc.).
		Span<const VoxelInt3> dirtyDoorAnimInstVoxels = voxelChunk.dirtyDoorAnimInstPositions;
		for (const VoxelInt3 doorVoxel : dirtyDoorAnimInstVoxels)
		{
			VoxelDoorDefID doorDefID;
			if (!voxelChunk.tryGetDoorDefID(doorVoxel.x, doorVoxel.y, doorVoxel.z, &doorDefID))
			{
				DebugLogError("Expected door def ID at (" + doorVoxel.toString() + ").");
				continue;
			}

			const VoxelDoorDefinition &doorDef = voxelChunk.doorDefs[doorDefID];
			const ArenaDoorType doorType = doorDef.type;
			const WorldDouble3 worldPosition = MakeVoxelWorldPosition(voxelChunk.position, doorVoxel, ceilingScale);
			const double doorAnimPercent = VoxelDoorUtils::getAnimPercentOrZero(doorVoxel.x, doorVoxel.y, doorVoxel.z, voxelChunk);

			for (int i = 0; i < VoxelDoorUtils::FACE_COUNT; i++)
			{
				const RenderTransform faceRenderTransform = MakeDoorFaceRenderTransform(doorType, i, worldPosition, doorAnimPercent);

				const auto doorTransformIter = renderChunk.doorTransformBuffers.find(doorVoxel);
				DebugAssert(doorTransformIter != renderChunk.doorTransformBuffers.end());
				const UniformBufferID doorTransformBufferID = doorTransformIter->second;
				renderer.populateUniformBufferIndexRenderTransform(doorTransformBufferID, i, faceRenderTransform);
			}
		}

		const VoxelFaceCombineChunk &faceCombineChunk = voxelFaceCombineChunkManager.getChunkAtPosition(chunkPos);
		Span<const VoxelFaceCombineResultID> dirtyFaceCombineResultIDs = faceCombineChunk.dirtyIDs;

		// Update draw calls of dirty voxels.
		// - @todo: there is some double/triple updating possible here, maybe optimize.
		Span<const VoxelInt3> dirtyShapeDefVoxels = voxelChunk.dirtyShapeDefPositions;
		Span<const VoxelInt3> dirtyFaceActivationVoxels = voxelChunk.dirtyFaceActivationPositions;
		Span<const VoxelInt3> dirtyDoorVisInstVoxels = voxelChunk.dirtyDoorVisInstPositions;
		Span<const VoxelInt3> dirtyFadeAnimInstVoxels = voxelChunk.dirtyFadeAnimInstPositions;

		// @todo all these dirty lists should be one list of DirtyVoxelFlags instead

		this->clearChunkCombinedVoxelDrawCalls(renderChunk, dirtyFaceCombineResultIDs, renderer);

		this->clearChunkNonCombinedVoxelDrawCalls(renderChunk, dirtyShapeDefVoxels);
		this->clearChunkNonCombinedVoxelDrawCalls(renderChunk, dirtyFaceActivationVoxels);
		this->clearChunkNonCombinedVoxelDrawCalls(renderChunk, dirtyDoorAnimInstVoxels);
		this->clearChunkNonCombinedVoxelDrawCalls(renderChunk, dirtyDoorVisInstVoxels);
		this->clearChunkNonCombinedVoxelDrawCalls(renderChunk, dirtyFadeAnimInstVoxels);

		this->updateChunkCombinedVoxelDrawCalls(renderChunk, dirtyShapeDefVoxels, voxelChunk, faceCombineChunk, voxelChunkManager, ceilingScale, chasmAnimPercent, renderer);
		this->updateChunkCombinedVoxelDrawCalls(renderChunk, dirtyFaceActivationVoxels, voxelChunk, faceCombineChunk, voxelChunkManager, ceilingScale, chasmAnimPercent, renderer);
		this->updateChunkCombinedVoxelDrawCalls(renderChunk, dirtyDoorAnimInstVoxels, voxelChunk, faceCombineChunk, voxelChunkManager, ceilingScale, chasmAnimPercent, renderer);
		this->updateChunkCombinedVoxelDrawCalls(renderChunk, dirtyDoorVisInstVoxels, voxelChunk, faceCombineChunk, voxelChunkManager, ceilingScale, chasmAnimPercent, renderer);
		this->updateChunkCombinedVoxelDrawCalls(renderChunk, dirtyFadeAnimInstVoxels, voxelChunk, faceCombineChunk, voxelChunkManager, ceilingScale, chasmAnimPercent, renderer);

		this->updateChunkDiagonalVoxelDrawCalls(renderChunk, dirtyShapeDefVoxels, voxelChunk, voxelChunkManager, ceilingScale);
		this->updateChunkDiagonalVoxelDrawCalls(renderChunk, dirtyFaceActivationVoxels, voxelChunk, voxelChunkManager, ceilingScale);
		this->updateChunkDiagonalVoxelDrawCalls(renderChunk, dirtyFadeAnimInstVoxels, voxelChunk, voxelChunkManager, ceilingScale);

		this->updateChunkDoorVoxelDrawCalls(renderChunk, dirtyShapeDefVoxels, voxelChunk, voxelChunkManager, ceilingScale);
		this->updateChunkDoorVoxelDrawCalls(renderChunk, dirtyDoorAnimInstVoxels, voxelChunk, voxelChunkManager, ceilingScale);
		this->updateChunkDoorVoxelDrawCalls(renderChunk, dirtyDoorVisInstVoxels, voxelChunk, voxelChunkManager, ceilingScale);
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
