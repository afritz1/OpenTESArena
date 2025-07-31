#include <algorithm>

#include "RenderCommandBuffer.h"
#include "RenderEntityManager.h"
#include "Renderer.h"
#include "RenderTransform.h"
#include "../Assets/TextureManager.h"
#include "../Entities/EntityChunkManager.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Entities/EntityObservedResult.h"
#include "../Entities/EntityVisibilityChunkManager.h"
#include "../Voxels/VoxelDoorUtils.h"

#include "components/debug/Debug.h"

namespace
{
	// Creates a buffer of texture refs, intended to be accessed with linearized keyframe indices.
	Buffer<ScopedObjectTextureRef> MakeEntityAnimationTextures(const EntityAnimationDefinition &animDef,
		TextureManager &textureManager, Renderer &renderer)
	{
		Buffer<ScopedObjectTextureRef> textureRefs(animDef.keyframeCount);

		// Need to go by state + keyframe list because the keyframes don't know whether they're mirrored.
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
					constexpr int bytesPerTexel = 1;
					const ObjectTextureID textureID = renderer.createObjectTexture(textureBuilder.width, textureBuilder.height, bytesPerTexel);
					if (textureID < 0)
					{
						DebugLogWarning("Couldn't create entity anim texture \"" + textureAsset.filename + "\".");
						continue;
					}

					Span2D<const uint8_t> srcTexels = textureBuilder.getTexels8();

					LockedTexture lockedTexture = renderer.lockObjectTexture(textureID);
					Span2D<uint8_t> dstTexels = lockedTexture.getTexels8();

					// Copy texels from source texture, mirroring if necessary.
					for (int y = 0; y < textureBuilder.height; y++)
					{
						for (int x = 0; x < textureBuilder.width; x++)
						{
							const int srcX = !keyframeList.isMirrored ? x : (textureBuilder.width - 1 - x);
							const uint8_t srcTexel = srcTexels.get(srcX, y);
							dstTexels.set(x, y, srcTexel);
						}
					}

					renderer.unlockObjectTexture(textureID);

					textureRefs.set(writeIndex, ScopedObjectTextureRef(textureID, renderer));
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

		const ObjectTextureID textureID = renderer.createObjectTexture(textureWidth, textureHeight, bytesPerTexel);
		if (textureID < 0)
		{
			DebugCrash("Couldn't create entity palette indices texture.");
		}

		LockedTexture lockedTexture = renderer.lockObjectTexture(textureID);
		uint8_t *dstTexels = lockedTexture.getTexels8().begin();
		std::copy(paletteIndices.begin(), paletteIndices.end(), dstTexels);
		renderer.unlockObjectTexture(textureID);
		return ScopedObjectTextureRef(textureID, renderer);
	}
}

void RenderEntityLoadedAnimation::init(EntityDefID defID, Buffer<ScopedObjectTextureRef> &&textureRefs)
{
	this->defID = defID;
	this->textureRefs = std::move(textureRefs);
}

RenderEntityManager::RenderEntityManager()
{

}

void RenderEntityManager::init(Renderer &renderer)
{
	// Populate entity mesh buffers. All entities share the same buffers, and the normals buffer is updated every frame.
	constexpr int positionComponentsPerVertex = MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
	constexpr int normalComponentsPerVertex = MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
	constexpr int texCoordComponentsPerVertex = MeshUtils::TEX_COORD_COMPONENTS_PER_VERTEX;
	constexpr int entityMeshVertexCount = 4;
	constexpr int entityMeshIndexCount = 6;

	this->meshInst.positionBufferID = renderer.createVertexPositionBuffer(entityMeshVertexCount, positionComponentsPerVertex);
	if (this->meshInst.positionBufferID < 0)
	{
		DebugLogError("Couldn't create vertex position buffer for entity mesh ID.");
		return;
	}

	this->meshInst.normalBufferID = renderer.createVertexAttributeBuffer(entityMeshVertexCount, normalComponentsPerVertex);
	if (this->meshInst.normalBufferID < 0)
	{
		DebugLogError("Couldn't create vertex normal attribute buffer for entity mesh def.");
		this->meshInst.freeBuffers(renderer);
		return;
	}

	this->meshInst.texCoordBufferID = renderer.createVertexAttributeBuffer(entityMeshVertexCount, texCoordComponentsPerVertex);
	if (this->meshInst.texCoordBufferID < 0)
	{
		DebugLogError("Couldn't create vertex tex coord attribute buffer for entity mesh def.");
		this->meshInst.freeBuffers(renderer);
		return;
	}

	this->meshInst.indexBufferID = renderer.createIndexBuffer(entityMeshIndexCount);
	if (this->meshInst.indexBufferID < 0)
	{
		DebugLogError("Couldn't create index buffer for entity mesh def.");
		this->meshInst.freeBuffers(renderer);
		return;
	}

	constexpr double entityPositions[entityMeshVertexCount * positionComponentsPerVertex] =
	{
		0.0, 1.0, -0.50,
		0.0, 0.0, -0.50,
		0.0, 0.0, 0.50,
		0.0, 1.0, 0.50
	};

	constexpr double dummyEntityNormals[entityMeshVertexCount * normalComponentsPerVertex] =
	{
		0.0, 0.0, 0.0,
		0.0, 0.0, 0.0,
		0.0, 0.0, 0.0,
		0.0, 0.0, 0.0
	};

	constexpr double entityTexCoords[entityMeshVertexCount * texCoordComponentsPerVertex] =
	{
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0
	};

	constexpr int32_t entityIndices[entityMeshIndexCount] =
	{
		0, 1, 2,
		2, 3, 0
	};

	renderer.populateVertexPositionBuffer(this->meshInst.positionBufferID, entityPositions);
	renderer.populateVertexAttributeBuffer(this->meshInst.normalBufferID, dummyEntityNormals);
	renderer.populateVertexAttributeBuffer(this->meshInst.texCoordBufferID, entityTexCoords);
	renderer.populateIndexBuffer(this->meshInst.indexBufferID, entityIndices);
}

void RenderEntityManager::shutdown(Renderer &renderer)
{
	this->anims.clear();
	this->meshInst.freeBuffers(renderer);
	this->paletteIndicesTextureRefs.clear();
	this->drawCallsCache.clear();
}

ObjectTextureID RenderEntityManager::getTextureID(EntityInstanceID entityInstID, const WorldDouble3 &cameraPosition,
	const EntityChunkManager &entityChunkManager) const
{
	const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
	const EntityDefID entityDefID = entityInst.defID;
	const auto defIter = std::find_if(this->anims.begin(), this->anims.end(),
		[entityDefID](const RenderEntityLoadedAnimation &loadedAnim)
	{
		return loadedAnim.defID == entityDefID;
	});

	DebugAssertMsg(defIter != this->anims.end(), "Expected loaded entity animation for def ID " + std::to_string(entityDefID) + ".");

	EntityObservedResult observedResult;
	entityChunkManager.getEntityObservedResult(entityInstID, cameraPosition, observedResult);

	const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityDefID);
	const EntityAnimationDefinition &animDef = entityDef.animDef;
	const int linearizedKeyframeIndex = observedResult.linearizedKeyframeIndex;
	Span<const ScopedObjectTextureRef> textureRefs = defIter->textureRefs;
	return textureRefs[linearizedKeyframeIndex].get();
}

void RenderEntityManager::loadTexturesForChunkEntities(const EntityChunk &entityChunk, const EntityChunkManager &entityChunkManager,
	TextureManager &textureManager, Renderer &renderer)
{
	for (const EntityInstanceID entityInstID : entityChunk.entityIDs)
	{
		const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
		const EntityDefID entityDefID = entityInst.defID;

		const auto animIter = std::find_if(this->anims.begin(), this->anims.end(),
			[entityDefID](const RenderEntityLoadedAnimation &loadedAnim)
		{
			return loadedAnim.defID == entityDefID;
		});

		if (animIter == this->anims.end())
		{
			const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityDefID);
			const EntityAnimationDefinition &animDef = entityDef.animDef;
			Buffer<ScopedObjectTextureRef> textureRefs = MakeEntityAnimationTextures(animDef, textureManager, renderer);

			RenderEntityLoadedAnimation loadedEntityAnim;
			loadedEntityAnim.init(entityDefID, std::move(textureRefs));
			this->anims.emplace_back(std::move(loadedEntityAnim));
		}

		if (entityInst.isCitizen())
		{
			const EntityPaletteIndicesInstanceID paletteIndicesInstID = entityInst.paletteIndicesInstID;
			const auto paletteIndicesIter = this->paletteIndicesTextureRefs.find(paletteIndicesInstID);
			if (paletteIndicesIter == this->paletteIndicesTextureRefs.end())
			{
				const PaletteIndices &paletteIndices = entityChunkManager.getEntityPaletteIndices(paletteIndicesInstID);
				ScopedObjectTextureRef paletteIndicesTextureRef = MakeEntityPaletteIndicesTextureRef(paletteIndices, renderer);
				this->paletteIndicesTextureRefs.emplace(paletteIndicesInstID, std::move(paletteIndicesTextureRef));
			}
		}
	}
}

void RenderEntityManager::loadTexturesForEntity(EntityDefID entityDefID, TextureManager &textureManager, Renderer &renderer)
{
	const auto animIter = std::find_if(this->anims.begin(), this->anims.end(),
		[entityDefID](const RenderEntityLoadedAnimation &loadedAnim)
	{
		return loadedAnim.defID == entityDefID;
	});

	if (animIter == this->anims.end())
	{
		const EntityDefinitionLibrary &entityDefLibrary = EntityDefinitionLibrary::getInstance();
		const EntityDefinition &entityDef = entityDefLibrary.getDefinition(entityDefID);
		const EntityAnimationDefinition &animDef = entityDef.animDef;
		Buffer<ScopedObjectTextureRef> textureRefs = MakeEntityAnimationTextures(animDef, textureManager, renderer);

		RenderEntityLoadedAnimation loadedEntityAnim;
		loadedEntityAnim.init(entityDefID, std::move(textureRefs));
		this->anims.emplace_back(std::move(loadedEntityAnim));
	}
}

void RenderEntityManager::populateCommandBuffer(RenderCommandBuffer &commandBuffer) const
{
	if (!this->drawCallsCache.empty())
	{
		commandBuffer.addDrawCalls(this->drawCallsCache);
	}

	if (!this->puddleSecondPassDrawCallsCache.empty())
	{
		// Puddles require two passes to avoid race conditions when rasterizing.
		commandBuffer.addDrawCalls(this->puddleSecondPassDrawCallsCache);
	}
}

void RenderEntityManager::loadScene(TextureManager &textureManager, Renderer &renderer)
{
	// Load global VFX textures.
	// @todo load these one time in SceneManager::init() and use some sort of ResourceLifetimeType to prevent them from unloading in here
	const EntityDefinitionLibrary &entityDefLibrary = EntityDefinitionLibrary::getInstance();
	for (int i = 0; i < entityDefLibrary.getDefinitionCount(); i++)
	{
		const EntityDefID entityDefID = static_cast<EntityDefID>(i);
		const EntityDefinition &entityDef = entityDefLibrary.getDefinition(entityDefID);
		if (!EntityUtils::isSceneManagedResource(entityDef.type))
		{
			this->loadTexturesForEntity(entityDefID, textureManager, renderer);
		}
	}
}

void RenderEntityManager::update(Span<const ChunkInt2> activeChunkPositions, Span<const ChunkInt2> newChunkPositions,
	const WorldDouble3 &cameraPosition, const VoxelDouble2 &cameraDirXZ, double ceilingScale, const EntityChunkManager &entityChunkManager,
	const EntityVisibilityChunkManager &entityVisChunkManager, TextureManager &textureManager, Renderer &renderer)
{
	for (const EntityInstanceID entityInstID : entityChunkManager.getQueuedDestroyEntityIDs())
	{
		const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
		if (entityInst.isCitizen())
		{
			const EntityPaletteIndicesInstanceID paletteIndicesInstID = entityInst.paletteIndicesInstID;
			const auto paletteIndicesIter = this->paletteIndicesTextureRefs.find(paletteIndicesInstID);
			if (paletteIndicesIter != this->paletteIndicesTextureRefs.end())
			{
				this->paletteIndicesTextureRefs.erase(paletteIndicesIter);
			}
		}
	}

	for (const ChunkInt2 chunkPos : newChunkPositions)
	{
		const EntityChunk &entityChunk = entityChunkManager.getChunkAtPosition(chunkPos);
		this->loadTexturesForChunkEntities(entityChunk, entityChunkManager, textureManager, renderer);
	}

	// The rotation all entities share for facing the camera.
	const Radians allEntitiesRotationRadians = -MathUtils::fullAtan2(cameraDirXZ) - Constants::HalfPi;
	const Matrix4d allEntitiesRotationMatrix = Matrix4d::yRotation(allEntitiesRotationRadians);

	this->drawCallsCache.clear();
	this->puddleSecondPassDrawCallsCache.clear();

	for (const ChunkInt2 chunkPos : activeChunkPositions)
	{
		const EntityChunk &entityChunk = entityChunkManager.getChunkAtPosition(chunkPos);

		// Update entity render transforms.
		for (const EntityInstanceID entityInstID : entityChunk.entityIDs)
		{
			const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
			const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityInst.defID);
			const EntityAnimationDefinition &animDef = entityDef.animDef;
			const WorldDouble3 entityPosition = entityChunkManager.getEntityPosition(entityInst.positionID);

			EntityObservedResult observedResult;
			entityChunkManager.getEntityObservedResult(entityInstID, cameraPosition, observedResult);

			const int linearizedKeyframeIndex = observedResult.linearizedKeyframeIndex;
			DebugAssertIndex(animDef.keyframes, linearizedKeyframeIndex);
			const EntityAnimationDefinitionKeyframe &keyframe = animDef.keyframes[linearizedKeyframeIndex];

			const UniformBufferID transformBufferID = entityInst.renderTransformBufferID;
			RenderTransform entityRenderTransform;
			entityRenderTransform.translation = Matrix4d::translation(entityPosition.x, entityPosition.y, entityPosition.z);
			entityRenderTransform.rotation = allEntitiesRotationMatrix;
			entityRenderTransform.scale = Matrix4d::scale(1.0, keyframe.height, keyframe.width);
			renderer.populateUniformBuffer(transformBufferID, entityRenderTransform);
		}

		const EntityVisibilityChunk &entityVisChunk = entityVisChunkManager.getChunkAtPosition(chunkPos);

		// Generate draw calls from visible entity chunks.
		for (const VisibleEntityEntry &visibleEntity : entityVisChunk.visibleEntityEntries)
		{
			const EntityInstanceID entityInstID = visibleEntity.id;
			const WorldDouble3 entityPosition = visibleEntity.position;
			const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
			const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityInst.defID);

			const ObjectTextureID textureID0 = this->getTextureID(entityInstID, cameraPosition, entityChunkManager);
			std::optional<ObjectTextureID> textureID1 = std::nullopt;
			PixelShaderType pixelShaderType = PixelShaderType::AlphaTested;

			const bool isCitizen = entityInst.isCitizen();
			const bool isGhost = EntityUtils::isGhost(entityDef);
			const bool isPuddle = EntityUtils::isPuddle(entityDef);
			if (isCitizen)
			{
				const EntityPaletteIndicesInstanceID paletteIndicesInstID = entityInst.paletteIndicesInstID;
				const auto paletteIndicesIter = this->paletteIndicesTextureRefs.find(paletteIndicesInstID);
				DebugAssertMsgFormat(paletteIndicesIter != this->paletteIndicesTextureRefs.end(), "Expected entity palette indices texture at ID %d for entity %d.", paletteIndicesInstID, entityInstID);
				textureID1 = paletteIndicesIter->second.get();
				pixelShaderType = PixelShaderType::AlphaTestedWithPaletteIndexLookup;
			}
			else if (isGhost)
			{
				pixelShaderType = PixelShaderType::AlphaTestedWithLightLevelOpacity;
			}
			else if (isPuddle)
			{
				pixelShaderType = PixelShaderType::AlphaTestedWithHorizonMirrorFirstPass;
			}

			const UniformBufferID transformBufferID = entityInst.renderTransformBufferID;
			const int entityTransformIndex = 0; // Each entity has their own transform buffer for now.

			RenderDrawCall drawCall;
			drawCall.transformBufferID = transformBufferID;
			drawCall.transformIndex = entityTransformIndex;
			drawCall.preScaleTranslationBufferID = -1;
			drawCall.positionBufferID = this->meshInst.positionBufferID;
			drawCall.normalBufferID = this->meshInst.normalBufferID;
			drawCall.texCoordBufferID = this->meshInst.texCoordBufferID;
			drawCall.indexBufferID = this->meshInst.indexBufferID;
			drawCall.textureIDs[0] = textureID0;
			drawCall.textureIDs[1] = textureID1.has_value() ? *textureID1 : -1;
			drawCall.lightingType = RenderLightingType::PerPixel;
			drawCall.lightPercent = 0.0;
			drawCall.vertexShaderType = VertexShaderType::Entity;
			drawCall.pixelShaderType = pixelShaderType;
			drawCall.pixelShaderParam0 = 0.0;
			drawCall.enableBackFaceCulling = true;
			drawCall.enableDepthRead = true;
			drawCall.enableDepthWrite = true;

			if (drawCall.pixelShaderType == PixelShaderType::AlphaTestedWithHorizonMirrorFirstPass)
			{
				RenderDrawCall puddleSecondPassDrawCall = drawCall;
				puddleSecondPassDrawCall.pixelShaderType = PixelShaderType::AlphaTestedWithHorizonMirrorSecondPass;
				puddleSecondPassDrawCall.lightingType = RenderLightingType::PerMesh; // Don't spend effort lighting reflection, value is unused.
				puddleSecondPassDrawCall.lightPercent = 0.0;

				this->puddleSecondPassDrawCallsCache.emplace_back(std::move(puddleSecondPassDrawCall));
			}

			this->drawCallsCache.emplace_back(std::move(drawCall));
		}
	}

	// Update normals buffer.
	const VoxelDouble2 entityDir = -cameraDirXZ;
	constexpr int entityMeshVertexCount = 4;
	const double entityNormals[entityMeshVertexCount * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX] =
	{
		entityDir.x, 0.0, entityDir.y,
		entityDir.x, 0.0, entityDir.y,
		entityDir.x, 0.0, entityDir.y,
		entityDir.x, 0.0, entityDir.y
	};

	renderer.populateVertexAttributeBuffer(this->meshInst.normalBufferID, entityNormals);
}

void RenderEntityManager::endFrame()
{

}

void RenderEntityManager::unloadScene(Renderer &renderer)
{
	this->anims.clear();
	this->paletteIndicesTextureRefs.clear();
	this->drawCallsCache.clear();
	this->puddleSecondPassDrawCallsCache.clear();
}
