#include <algorithm>
#include <array>

#include "RenderCommandBuffer.h"
#include "RenderEntityChunkManager.h"
#include "Renderer.h"
#include "RenderLightChunkManager.h"
#include "RenderTransform.h"
#include "../Assets/TextureManager.h"
#include "../Entities/EntityChunkManager.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Entities/EntityObservedResult.h"
#include "../Entities/EntityVisibilityChunkManager.h"
#include "../Voxels/VoxelChunkManager.h"
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
					const int textureWidth = textureBuilder.getWidth();
					const int textureHeight = textureBuilder.getHeight();
					const int texelCount = textureWidth * textureHeight;
					constexpr int bytesPerTexel = 1;
					DebugAssert(textureBuilder.type == TextureBuilderType::Paletted);

					const ObjectTextureID textureID = renderer.createObjectTexture(textureWidth, textureHeight, bytesPerTexel);
					if (textureID < 0)
					{
						DebugLogWarning("Couldn't create entity anim texture \"" + textureAsset.filename + "\".");
						continue;
					}

					ScopedObjectTextureRef textureRef(textureID, renderer);

					const TextureBuilderPalettedTexture &palettedTexture = textureBuilder.paletteTexture;
					const uint8_t *srcTexels = palettedTexture.texels.begin();

					LockedTexture lockedTexture = renderer.lockObjectTexture(textureID);
					uint8_t *dstTexels = static_cast<uint8_t*>(lockedTexture.texels);

					// Copy texels from source texture, mirroring if necessary.
					for (int y = 0; y < textureHeight; y++)
					{
						for (int x = 0; x < textureWidth; x++)
						{
							const int srcX = !keyframeList.isMirrored ? x : (textureWidth - 1 - x);
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

		const ObjectTextureID textureID = renderer.createObjectTexture(textureWidth, textureHeight, bytesPerTexel);
		if (textureID < 0)
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

void RenderEntityLoadedAnimation::init(EntityDefID defID, Buffer<ScopedObjectTextureRef> &&textureRefs)
{
	this->defID = defID;
	this->textureRefs = std::move(textureRefs);
}

RenderEntityChunkManager::RenderEntityChunkManager()
{
	
}

void RenderEntityChunkManager::init(Renderer &renderer)
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

	constexpr std::array<double, entityMeshVertexCount * positionComponentsPerVertex> entityPositions =
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

	renderer.populateVertexPositionBuffer(this->meshInst.positionBufferID, entityPositions);
	renderer.populateVertexAttributeBuffer(this->meshInst.normalBufferID, dummyEntityNormals);
	renderer.populateVertexAttributeBuffer(this->meshInst.texCoordBufferID, entityTexCoords);
	renderer.populateIndexBuffer(this->meshInst.indexBufferID, entityIndices);
}

void RenderEntityChunkManager::shutdown(Renderer &renderer)
{
	for (int i = static_cast<int>(this->activeChunks.size()) - 1; i >= 0; i--)
	{
		ChunkPtr &chunkPtr = this->activeChunks[i];
		this->recycleChunk(i);
	}

	this->anims.clear();
	this->meshInst.freeBuffers(renderer);
	this->paletteIndicesTextureRefs.clear();
	this->drawCallsCache.clear();
}

ObjectTextureID RenderEntityChunkManager::getTextureID(EntityInstanceID entityInstID, const WorldDouble3 &cameraPosition,
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
	return textureRefs.get(linearizedKeyframeIndex).get();
}

void RenderEntityChunkManager::loadTexturesForChunkEntities(const EntityChunk &entityChunk, const EntityChunkManager &entityChunkManager,
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

void RenderEntityChunkManager::addDrawCall(UniformBufferID transformBufferID, int transformIndex, ObjectTextureID textureID0,
	const std::optional<ObjectTextureID> &textureID1, Span<const RenderLightID> lightIDs, PixelShaderType pixelShaderType,
	std::vector<RenderDrawCall> &drawCalls)
{
	RenderDrawCall drawCall;
	drawCall.transformBufferID = transformBufferID;
	drawCall.transformIndex = transformIndex;
	drawCall.preScaleTranslationBufferID = -1;
	drawCall.positionBufferID = this->meshInst.positionBufferID;
	drawCall.normalBufferID = this->meshInst.normalBufferID;
	drawCall.texCoordBufferID = this->meshInst.texCoordBufferID;
	drawCall.indexBufferID = this->meshInst.indexBufferID;
	drawCall.textureIDs[0] = textureID0;
	drawCall.textureIDs[1] = textureID1.has_value() ? *textureID1 : -1;
	drawCall.lightingType = RenderLightingType::PerPixel;
	drawCall.lightPercent = 0.0;

	DebugAssert(std::size(drawCall.lightIDs) >= lightIDs.getCount());
	std::copy(lightIDs.begin(), lightIDs.end(), std::begin(drawCall.lightIDs));
	drawCall.lightIdCount = lightIDs.getCount();

	drawCall.vertexShaderType = VertexShaderType::Entity;
	drawCall.pixelShaderType = pixelShaderType;
	drawCall.pixelShaderParam0 = 0.0;
	drawCall.enableDepthRead = true;
	drawCall.enableDepthWrite = true;

	drawCalls.emplace_back(std::move(drawCall));
}

void RenderEntityChunkManager::rebuildChunkDrawCalls(RenderEntityChunk &renderChunk, const EntityVisibilityChunk &entityVisChunk,
	const RenderLightChunk &renderLightChunk, const WorldDouble3 &cameraPosition, double ceilingScale,
	const EntityChunkManager &entityChunkManager)
{
	renderChunk.drawCalls.clear();

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
			DebugAssertMsg(paletteIndicesIter != this->paletteIndicesTextureRefs.end(), "Expected entity palette indices texture for ID " + std::to_string(paletteIndicesInstID) + ".");
			textureID1 = paletteIndicesIter->second.get();
			pixelShaderType = PixelShaderType::AlphaTestedWithPaletteIndexLookup;
		}
		else if (isGhost)
		{
			pixelShaderType = PixelShaderType::AlphaTestedWithLightLevelOpacity;
		}
		else if (isPuddle)
		{
			pixelShaderType = PixelShaderType::AlphaTestedWithHorizonMirror;
		}

		const CoordDouble3 entityCoord = VoxelUtils::worldPointToCoord(entityPosition);
		const VoxelDouble3 &entityLightPoint = entityCoord.point; // Where the entity receives its light (can't use center due to some really tall entities reaching outside the chunk).
		const VoxelInt3 entityLightVoxel = VoxelUtils::pointToVoxel(entityLightPoint, ceilingScale);
		Span<const RenderLightID> lightIdsView; // Limitation of reusing lights per voxel: entity is unlit if they are outside the world.
		if (renderLightChunk.isValidVoxel(entityLightVoxel.x, entityLightVoxel.y, entityLightVoxel.z))
		{
			const RenderLightIdList &voxelLightIdList = renderLightChunk.lightIdLists.get(entityLightVoxel.x, entityLightVoxel.y, entityLightVoxel.z);
			lightIdsView = voxelLightIdList.getLightIDs();
		}

		const UniformBufferID transformBufferID = entityInst.renderTransformBufferID;
		const int entityTransformIndex = 0; // Each entity has their own transform buffer for now.
		this->addDrawCall(transformBufferID, entityTransformIndex, textureID0, textureID1, lightIdsView, pixelShaderType, renderChunk.drawCalls);
	}
}

void RenderEntityChunkManager::rebuildDrawCallsList()
{
	this->drawCallsCache.clear();

	// @todo: puddles don't show reflections of entities in later chunks, maybe need to sort chunks far->near by distance sqr,
	// not just entities per-chunk in EntityVisibilityChunk.

	// Assumed to be sorted during entity visibility calculations.
	for (size_t i = 0; i < this->activeChunks.size(); i++)
	{
		const ChunkPtr &chunkPtr = this->activeChunks[i];
		Span<const RenderDrawCall> drawCalls = chunkPtr->drawCalls;
		this->drawCallsCache.insert(this->drawCallsCache.end(), drawCalls.begin(), drawCalls.end());
	}
}

void RenderEntityChunkManager::loadTexturesForEntity(EntityDefID entityDefID, TextureManager &textureManager, Renderer &renderer)
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

void RenderEntityChunkManager::populateCommandBuffer(RenderCommandBuffer &commandBuffer) const
{
	// Need to have barriers around puddle draw calls to avoid artifacts with reflected entities.
	int currentStartIndex = 0;
	int currentCount = 0;

	const int drawCallCount = static_cast<int>(this->drawCallsCache.size());
	for (int i = 0; i < drawCallCount; i++)
	{
		const RenderDrawCall &drawCall = this->drawCallsCache[i];
		const bool isPuddle = drawCall.pixelShaderType == PixelShaderType::AlphaTestedWithHorizonMirror;
		if (isPuddle)
		{
			if (currentCount > 0)
			{
				commandBuffer.addDrawCalls(Span<const RenderDrawCall>(this->drawCallsCache.data() + currentStartIndex, currentCount));
				currentCount = 0;
			}

			commandBuffer.addDrawCalls(Span<const RenderDrawCall>(this->drawCallsCache.data() + i, 1));
			currentStartIndex = i + 1;
			continue;
		}

		currentCount++;
	}

	if (currentCount > 0)
	{
		// Add one last draw call range.
		commandBuffer.addDrawCalls(Span<const RenderDrawCall>(this->drawCallsCache.data() + currentStartIndex, currentCount));
	}
}

void RenderEntityChunkManager::loadScene(TextureManager &textureManager, Renderer &renderer)
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

void RenderEntityChunkManager::updateActiveChunks(Span<const ChunkInt2> newChunkPositions, Span<const ChunkInt2> freedChunkPositions,
	const VoxelChunkManager &voxelChunkManager, Renderer &renderer)
{
	for (const ChunkInt2 chunkPos : freedChunkPositions)
	{
		const int chunkIndex = this->getChunkIndex(chunkPos);
		this->recycleChunk(chunkIndex);
	}

	for (const ChunkInt2 chunkPos : newChunkPositions)
	{
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);

		const int spawnIndex = this->spawnChunk();
		RenderEntityChunk &renderChunk = this->getChunkAtIndex(spawnIndex);
		renderChunk.init(chunkPos, voxelChunk.height);
	}

	// Free any unneeded chunks for memory savings in case the chunk distance was once large
	// and is now small. This is significant even for chunk distance 2->1, or 25->9 chunks.
	this->chunkPool.clear();
}

void RenderEntityChunkManager::update(Span<const ChunkInt2> activeChunkPositions, Span<const ChunkInt2> newChunkPositions,
	const WorldDouble3 &cameraPosition, const VoxelDouble2 &cameraDirXZ, double ceilingScale, const VoxelChunkManager &voxelChunkManager,
	const EntityChunkManager &entityChunkManager, const EntityVisibilityChunkManager &entityVisChunkManager,
	const RenderLightChunkManager &renderLightChunkManager, TextureManager &textureManager, Renderer &renderer)
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
		RenderEntityChunk &renderChunk = this->getChunkAtPosition(chunkPos);
		const EntityChunk &entityChunk = entityChunkManager.getChunkAtPosition(chunkPos);
		this->loadTexturesForChunkEntities(entityChunk, entityChunkManager, textureManager, renderer);
	}

	// The rotation all entities share for facing the camera.
	const Radians allEntitiesRotationRadians = -MathUtils::fullAtan2(cameraDirXZ) - Constants::HalfPi;
	const Matrix4d allEntitiesRotationMatrix = Matrix4d::yRotation(allEntitiesRotationRadians);

	for (const ChunkInt2 chunkPos : activeChunkPositions)
	{
		RenderEntityChunk &renderChunk = this->getChunkAtPosition(chunkPos);
		const EntityChunk &entityChunk = entityChunkManager.getChunkAtPosition(chunkPos);
		const EntityVisibilityChunk &entityVisChunk = entityVisChunkManager.getChunkAtPosition(chunkPos);
		const RenderLightChunk &renderLightChunk = renderLightChunkManager.getChunkAtPosition(chunkPos);

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

		this->rebuildChunkDrawCalls(renderChunk, entityVisChunk, renderLightChunk, cameraPosition, ceilingScale, entityChunkManager);
	}

	this->rebuildDrawCallsList();

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

	renderer.populateVertexAttributeBuffer(this->meshInst.normalBufferID, entityNormals);
}

void RenderEntityChunkManager::cleanUp()
{
	
}

void RenderEntityChunkManager::unloadScene(Renderer &renderer)
{
	this->anims.clear();
	this->paletteIndicesTextureRefs.clear();
	this->drawCallsCache.clear();
	this->recycleAllChunks();
}
