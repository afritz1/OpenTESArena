#include <algorithm>

#include "RenderCamera.h"
#include "RenderCommand.h"
#include "RenderEntityManager.h"
#include "Renderer.h"
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

	ObjectTextureID CreateEntityPaletteIndicesTextureID(const PaletteIndices &paletteIndices, Renderer &renderer)
	{
		const int textureWidth = static_cast<int>(paletteIndices.size());
		constexpr int textureHeight = 1;
		constexpr int bytesPerTexel = 1;

		const ObjectTextureID textureID = renderer.createObjectTexture(textureWidth, textureHeight, bytesPerTexel);
		if (textureID < 0)
		{
			DebugLogError("Couldn't create entity palette indices texture.");
			return -1;
		}

		if (!renderer.populateObjectTexture8Bit(textureID, paletteIndices))
		{
			DebugLogError("Couldn't populate entity palette indices texture.");
		}

		return textureID;
	}

	FragmentShaderType GetEntityFragmentShaderType(const EntityDefinition &entityDef)
	{
		FragmentShaderType fragmentShaderType;
		if (EntityUtils::isGhost(entityDef))
		{
			fragmentShaderType = FragmentShaderType::AlphaTestedWithLightLevelOpacity;
		}
		else if (EntityUtils::isPuddle(entityDef))
		{
			fragmentShaderType = FragmentShaderType::AlphaTestedWithHorizonMirrorFirstPass;
		}
		else if (entityDef.type == EntityDefinitionType::Citizen)
		{
			fragmentShaderType = FragmentShaderType::AlphaTestedWithPaletteIndexLookup;
		}
		else
		{
			fragmentShaderType = FragmentShaderType::AlphaTested;
		}

		return fragmentShaderType;
	}

	RenderMaterialKey MakeEntityRenderMaterialKey(FragmentShaderType fragmentShaderType, Span<const ObjectTextureID> textureIDs)
	{
		RenderMaterialKey materialKey;
		materialKey.init(VertexShaderType::Entity, fragmentShaderType, textureIDs, RenderLightingType::PerPixel, true, true, true);
		return materialKey;
	}

	RenderMaterialKey MakePuddleSecondPassMaterialKey(ObjectTextureID textureID)
	{
		constexpr RenderLightingType lightingType = RenderLightingType::PerMesh; // Don't spend effort lighting reflection, value is unused.

		RenderMaterialKey materialKey;
		materialKey.init(VertexShaderType::Entity, FragmentShaderType::AlphaTestedWithHorizonMirrorSecondPass, Span<const ObjectTextureID>(&textureID, 1), lightingType, true, true, true);
		return materialKey;
	}
}

RenderEntityLoadedAnimation::RenderEntityLoadedAnimation()
{
	this->defID = -1;
}

void RenderEntityLoadedAnimation::init(EntityDefID defID, Buffer<ScopedObjectTextureRef> &&textureRefs)
{
	this->defID = defID;
	this->textureRefs = std::move(textureRefs);
}

RenderEntityPaletteIndicesEntry::RenderEntityPaletteIndicesEntry()
{
	this->paletteIndicesInstanceID = -1;
	this->textureID = -1;
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
	this->paletteIndicesEntries.clear();
	this->drawCallsCache.clear();
	this->ghostDrawCallsCache.clear();
	this->puddleSecondPassDrawCallsCache.clear();
}

void RenderEntityManager::loadMaterialsForChunkEntities(const EntityChunk &entityChunk, const EntityChunkManager &entityChunkManager,
	TextureManager &textureManager, Renderer &renderer)
{
	for (const EntityInstanceID entityInstID : entityChunk.entityIDs)
	{
		const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
		const EntityDefID entityDefID = entityInst.defID;

		const RenderEntityLoadedAnimation *loadedAnim = nullptr;
		for (const RenderEntityLoadedAnimation &currentLoadedAnim : this->anims)
		{
			if (currentLoadedAnim.defID == entityDefID)
			{
				loadedAnim = &currentLoadedAnim;
				break;
			}
		}

		const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityDefID);
		const FragmentShaderType fragmentShaderType = GetEntityFragmentShaderType(entityDef);

		if (loadedAnim == nullptr)
		{
			const EntityAnimationDefinition &animDef = entityDef.animDef;
			Buffer<ScopedObjectTextureRef> textureRefs = MakeEntityAnimationTextures(animDef, textureManager, renderer);

			RenderEntityLoadedAnimation &newLoadedAnim = this->anims.emplace_back(RenderEntityLoadedAnimation());
			newLoadedAnim.init(entityDefID, std::move(textureRefs));

			loadedAnim = &newLoadedAnim;
		}

		if (entityInst.isCitizen())
		{
			const EntityPaletteIndicesInstanceID paletteIndicesInstID = entityInst.paletteIndicesInstID;
			const auto paletteIndicesIter = std::find_if(this->paletteIndicesEntries.begin(), this->paletteIndicesEntries.end(),
				[paletteIndicesInstID](const RenderEntityPaletteIndicesEntry &entry)
			{
				return entry.paletteIndicesInstanceID == paletteIndicesInstID;
			});

			if (paletteIndicesIter == this->paletteIndicesEntries.end())
			{
				const PaletteIndices &paletteIndices = entityChunkManager.getEntityPaletteIndices(paletteIndicesInstID);
				const ObjectTextureID paletteIndicesTextureID = CreateEntityPaletteIndicesTextureID(paletteIndices, renderer);

				Span<const ScopedObjectTextureRef> loadedAnimTextureRefs = loadedAnim->textureRefs;

				RenderEntityPaletteIndicesEntry newEntry;
				newEntry.paletteIndicesInstanceID = paletteIndicesInstID;
				newEntry.textureID = paletteIndicesTextureID;
				newEntry.materialIDs.init(loadedAnimTextureRefs.getCount());
				for (int i = 0; i < loadedAnimTextureRefs.getCount(); i++)
				{
					const ObjectTextureID materialTextureIDs[] = { loadedAnimTextureRefs[i].get(), paletteIndicesTextureID };
					RenderMaterialKey materialKey = MakeEntityRenderMaterialKey(FragmentShaderType::AlphaTestedWithPaletteIndexLookup, materialTextureIDs);
					newEntry.materialIDs[i] = renderer.createMaterial(materialKey);
				}

				this->paletteIndicesEntries.emplace_back(std::move(newEntry));
			}
		}
		else
		{
			auto addMaterialIfUnique = [this, &renderer](RenderMaterialKey materialKey)
			{
				const auto materialIter = std::find_if(this->materials.begin(), this->materials.end(),
					[materialKey](const RenderMaterial &material)
				{
					return material.key == materialKey;
				});

				if (materialIter == this->materials.end())
				{
					RenderMaterial material;
					material.key = materialKey;
					material.id = renderer.createMaterial(material.key);

					this->materials.emplace_back(std::move(material));
				}
			};

			for (int i = 0; i < loadedAnim->textureRefs.getCount(); i++)
			{
				const ObjectTextureID materialTextureIDs[] = { loadedAnim->textureRefs[i].get() };
				const RenderMaterialKey materialKey = MakeEntityRenderMaterialKey(fragmentShaderType, materialTextureIDs);
				addMaterialIfUnique(materialKey);
			}

			if (fragmentShaderType == FragmentShaderType::AlphaTestedWithHorizonMirrorFirstPass)
			{
				for (int i = 0; i < loadedAnim->textureRefs.getCount(); i++)
				{
					const RenderMaterialKey puddleSecondPassMaterialKey = MakePuddleSecondPassMaterialKey(loadedAnim->textureRefs[i].get());
					addMaterialIfUnique(puddleSecondPassMaterialKey);
				}
			}
		}
	}
}

void RenderEntityManager::loadMaterialsForEntity(EntityDefID entityDefID, TextureManager &textureManager, Renderer &renderer)
{
	const auto existingAnimIter = std::find_if(this->anims.begin(), this->anims.end(),
		[entityDefID](const RenderEntityLoadedAnimation &loadedAnim)
	{
		return loadedAnim.defID == entityDefID;
	});

	if (existingAnimIter != this->anims.end())
	{
		return;
	}

	const EntityDefinitionLibrary &entityDefLibrary = EntityDefinitionLibrary::getInstance();
	const EntityDefinition &entityDef = entityDefLibrary.getDefinition(entityDefID);
	DebugAssert(entityDef.type != EntityDefinitionType::Citizen);

	const EntityAnimationDefinition &animDef = entityDef.animDef;
	Buffer<ScopedObjectTextureRef> textureRefs = MakeEntityAnimationTextures(animDef, textureManager, renderer);

	RenderEntityLoadedAnimation &loadedAnim = this->anims.emplace_back(RenderEntityLoadedAnimation());
	loadedAnim.init(entityDefID, std::move(textureRefs));

	const FragmentShaderType fragmentShaderType = GetEntityFragmentShaderType(entityDef);

	for (int i = 0; i < loadedAnim.textureRefs.getCount(); i++)
	{
		const ObjectTextureID materialTextureIDs[] = { loadedAnim.textureRefs[i].get() };
		const RenderMaterialKey materialKey = MakeEntityRenderMaterialKey(fragmentShaderType, materialTextureIDs);

		RenderMaterial material;
		material.key = materialKey;
		material.id = renderer.createMaterial(material.key);

		this->materials.emplace_back(std::move(material));
	}
}

void RenderEntityManager::populateCommandList(RenderCommandList &commandList) const
{
	if (!this->drawCallsCache.empty())
	{
		commandList.addDrawCalls(this->drawCallsCache);
	}

	if (!this->ghostDrawCallsCache.empty())
	{
		commandList.addDrawCalls(this->ghostDrawCallsCache);
	}

	if (!this->puddleSecondPassDrawCallsCache.empty())
	{
		// Puddles require two passes to avoid race conditions when rasterizing.
		commandList.addDrawCalls(this->puddleSecondPassDrawCallsCache);
	}
}

void RenderEntityManager::loadScene(TextureManager &textureManager, Renderer &renderer)
{
	// Load global VFX materials.
	// @todo load these one time in SceneManager::init() and use some sort of ResourceLifetimeType to prevent them from unloading in here
	const EntityDefinitionLibrary &entityDefLibrary = EntityDefinitionLibrary::getInstance();
	for (int i = 0; i < entityDefLibrary.getDefinitionCount(); i++)
	{
		const EntityDefID entityDefID = static_cast<EntityDefID>(i);
		const EntityDefinition &entityDef = entityDefLibrary.getDefinition(entityDefID);
		if (!EntityUtils::isSceneManagedResource(entityDef.type))
		{
			this->loadMaterialsForEntity(entityDefID, textureManager, renderer);
		}
	}
}

void RenderEntityManager::update(Span<const ChunkInt2> activeChunkPositions, Span<const ChunkInt2> newChunkPositions,
	const RenderCamera &camera, const VoxelDouble2 &cameraDirXZ, double ceilingScale, const EntityChunkManager &entityChunkManager,
	const EntityVisibilityChunkManager &entityVisChunkManager, Span<RenderTransformHeap> transformHeaps, TextureManager &textureManager,
	Renderer &renderer)
{
	// Free destroyed entity palettes + materials.
	for (const EntityInstanceID entityInstID : entityChunkManager.getQueuedDestroyEntityIDs())
	{
		const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
		if (entityInst.isCitizen())
		{
			const EntityPaletteIndicesInstanceID paletteIndicesInstID = entityInst.paletteIndicesInstID;

			for (int i = 0; i < static_cast<int>(this->paletteIndicesEntries.size()); i++)
			{
				RenderEntityPaletteIndicesEntry &paletteIndicesEntry = this->paletteIndicesEntries[i];

				if (paletteIndicesEntry.paletteIndicesInstanceID == paletteIndicesInstID)
				{
					renderer.freeObjectTexture(paletteIndicesEntry.textureID);

					for (RenderMaterialID paletteIndicesMaterialID : paletteIndicesEntry.materialIDs)
					{
						renderer.freeMaterial(paletteIndicesMaterialID);
					}

					this->paletteIndicesEntries.erase(this->paletteIndicesEntries.begin() + i);
					break;
				}
			}
		}
	}

	for (const ChunkInt2 chunkPos : newChunkPositions)
	{
		const EntityChunk &entityChunk = entityChunkManager.getChunkAtPosition(chunkPos);
		this->loadMaterialsForChunkEntities(entityChunk, entityChunkManager, textureManager, renderer);
	}

	this->drawCallsCache.clear();
	this->ghostDrawCallsCache.clear();
	this->puddleSecondPassDrawCallsCache.clear();

	// The rotation all entities share for facing the camera.
	const Radians allEntitiesRotationRadians = -MathUtils::fullAtan2(cameraDirXZ) - Constants::HalfPi;
	const Matrix4d allEntitiesRotationMatrix = Matrix4d::yRotation(allEntitiesRotationRadians);

	for (const ChunkInt2 chunkPos : activeChunkPositions)
	{
		const EntityChunk &entityChunk = entityChunkManager.getChunkAtPosition(chunkPos);
		const EntityVisibilityChunk &entityVisChunk = entityVisChunkManager.getChunkAtPosition(chunkPos);

		// Generate draw calls from visible entity chunks.
		for (const VisibleEntityEntry &visibleEntity : entityVisChunk.visibleEntityEntries)
		{
			const EntityInstanceID entityInstID = visibleEntity.id;
			const WorldDouble3 entityPosition = visibleEntity.position;
			const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
			const EntityDefID entityDefID = entityInst.defID;
			const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityDefID);
			const EntityAnimationDefinition &animDef = entityDef.animDef;
			const auto animDefIter = std::find_if(this->anims.begin(), this->anims.end(),
				[entityDefID](const RenderEntityLoadedAnimation &loadedAnim)
			{
				return loadedAnim.defID == entityDefID;
			});

			DebugAssertMsgFormat(animDefIter != this->anims.end(), "Expected loaded entity animation for def ID %d.", entityDefID);

			EntityObservedResult observedResult;
			entityChunkManager.getEntityObservedResult(entityInstID, camera.worldPoint, observedResult);
			const int linearizedKeyframeIndex = observedResult.linearizedKeyframeIndex;
			DebugAssertIndex(animDef.keyframes, linearizedKeyframeIndex);
			const EntityAnimationDefinitionKeyframe &keyframe = animDef.keyframes[linearizedKeyframeIndex];

			const FragmentShaderType fragmentShaderType = GetEntityFragmentShaderType(entityDef);
			const ObjectTextureID observedTextureIDs[] = { animDefIter->textureRefs[linearizedKeyframeIndex].get() };

			RenderMaterialID materialID = -1;
			if (entityInst.isCitizen())
			{
				for (const RenderEntityPaletteIndicesEntry &paletteIndicesEntry : this->paletteIndicesEntries)
				{
					if (paletteIndicesEntry.paletteIndicesInstanceID == entityInst.paletteIndicesInstID)
					{
						materialID = paletteIndicesEntry.materialIDs[linearizedKeyframeIndex];
						break;
					}
				}
			}
			else
			{
				const RenderMaterialKey materialKey = MakeEntityRenderMaterialKey(fragmentShaderType, observedTextureIDs);

				for (const RenderMaterial &material : this->materials)
				{
					if (material.key == materialKey)
					{
						materialID = material.id;
						break;
					}
				}
			}

			DebugAssert(materialID >= 0);

			RenderTransformHeap &transformHeap = transformHeaps[entityInst.transformHeapIndex];

			RenderDrawCall drawCall;
			drawCall.transformBufferID = transformHeap.uniformBufferID;
			drawCall.transformIndex = entityInst.transformIndex;
			drawCall.positionBufferID = this->meshInst.positionBufferID;
			drawCall.normalBufferID = this->meshInst.normalBufferID;
			drawCall.texCoordBufferID = this->meshInst.texCoordBufferID;
			drawCall.indexBufferID = this->meshInst.indexBufferID;
			drawCall.materialID = materialID;
			drawCall.materialInstID = -1;

			if (fragmentShaderType == FragmentShaderType::AlphaTestedWithHorizonMirrorFirstPass)
			{
				const RenderMaterialKey puddleSecondPassMaterialKey = MakePuddleSecondPassMaterialKey(observedTextureIDs[0]);

				RenderMaterialID puddleSecondPassMaterialID = -1;
				for (const RenderMaterial &material : this->materials)
				{
					if (material.key == puddleSecondPassMaterialKey)
					{
						puddleSecondPassMaterialID = material.id;
						break;
					}
				}

				DebugAssert(puddleSecondPassMaterialID >= 0);

				RenderDrawCall puddleSecondPassDrawCall = drawCall;
				puddleSecondPassDrawCall.materialID = puddleSecondPassMaterialID;
				puddleSecondPassDrawCall.multipassType = RenderMultipassType::Puddles;

				this->puddleSecondPassDrawCallsCache.emplace_back(std::move(puddleSecondPassDrawCall));
			}

			if (fragmentShaderType == FragmentShaderType::AlphaTestedWithLightLevelOpacity)
			{
				drawCall.multipassType = RenderMultipassType::Ghosts;
				this->ghostDrawCallsCache.emplace_back(std::move(drawCall));
			}
			else
			{
				drawCall.multipassType = RenderMultipassType::None;
				this->drawCallsCache.emplace_back(std::move(drawCall));
			}

			// Update render transform after physics update so the floating origin is correct.
			const WorldDouble3 floatingEntityPosition = entityPosition - camera.floatingOriginPoint;
			const Matrix4d entityTranslationMatrix = Matrix4d::translation(floatingEntityPosition.x, floatingEntityPosition.y, floatingEntityPosition.z);
			const Matrix4d entityScaleMatrix = Matrix4d::scale(1.0, keyframe.height, keyframe.width);

			Matrix4d &entityModelMatrix = transformHeap.pool.values[entityInst.transformIndex];
			entityModelMatrix = entityTranslationMatrix * (allEntitiesRotationMatrix * entityScaleMatrix);
		}
	}

	// Update normals buffer.
	const Double2 entityDir = -cameraDirXZ;
	constexpr int entityMeshVertexCount = 4;
	const double entityNormals[entityMeshVertexCount * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX] =
	{
		entityDir.x, 0.0, entityDir.y,
		entityDir.x, 0.0, entityDir.y,
		entityDir.x, 0.0, entityDir.y,
		entityDir.x, 0.0, entityDir.y
	};

	renderer.populateVertexAttributeBuffer(this->meshInst.normalBufferID, entityNormals);

	// Update model matrix buffers in bulk.
	for (const RenderTransformHeap &transformHeap : transformHeaps)
	{
		if (transformHeap.pool.getUsedCount() > 0)
		{
			Span<const Matrix4d> modelMatrices(transformHeap.pool.values.get(), transformHeap.pool.capacity);
			renderer.populateUniformBufferMatrix4s(transformHeap.uniformBufferID, modelMatrices);
		}
	}
}

void RenderEntityManager::endFrame()
{

}

void RenderEntityManager::unloadScene(Renderer &renderer)
{
	this->anims.clear();

	for (RenderEntityPaletteIndicesEntry &entry : this->paletteIndicesEntries)
	{
		if (entry.textureID >= 0)
		{
			renderer.freeObjectTexture(entry.textureID);
			entry.textureID = -1;
		}

		for (RenderMaterialID materialID : entry.materialIDs)
		{
			if (materialID >= 0)
			{
				renderer.freeMaterial(materialID);
			}
		}
	}

	this->paletteIndicesEntries.clear();

	for (RenderMaterial &material : this->materials)
	{
		if (material.id >= 0)
		{
			renderer.freeMaterial(material.id);
		}
	}

	this->materials.clear();
	this->drawCallsCache.clear();
	this->ghostDrawCallsCache.clear();
	this->puddleSecondPassDrawCallsCache.clear();
}
