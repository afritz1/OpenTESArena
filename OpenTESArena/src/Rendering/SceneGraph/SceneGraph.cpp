#include <array>
#include <numeric>
#include <optional>

#include "SceneGraph.h"
#include "../ArenaRenderUtils.h"
#include "../RenderCamera.h"
#include "../Renderer.h"
#include "../RendererSystem3D.h"
#include "../RenderTriangle.h"
#include "../../Assets/MIFUtils.h"
#include "../../Entities/EntityManager.h"
#include "../../Entities/EntityVisibilityState.h"
#include "../../Math/Constants.h"
#include "../../Math/Matrix4.h"
#include "../../Media/TextureManager.h"
#include "../../World/ArenaMeshUtils.h"
#include "../../World/ChunkManager.h"
#include "../../World/LevelInstance.h"
#include "../../World/MapDefinition.h"
#include "../../World/MapType.h"
#include "../../World/VoxelFacing2D.h"

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
	void LoadVoxelDefTextures(const VoxelTextureDefinition &voxelTextureDef, std::vector<SceneGraph::LoadedVoxelTexture> &voxelTextures,
		TextureManager &textureManager, Renderer &renderer)
	{
		for (int i = 0; i < voxelTextureDef.textureCount; i++)
		{
			const TextureAsset &textureAsset = voxelTextureDef.getTextureAsset(i);
			const auto cacheIter = std::find_if(voxelTextures.begin(), voxelTextures.end(),
				[&textureAsset](const SceneGraph::LoadedVoxelTexture &loadedTexture)
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
				SceneGraph::LoadedVoxelTexture newTexture;
				newTexture.init(textureAsset, std::move(voxelTextureRef));
				voxelTextures.emplace_back(std::move(newTexture));
			}
		}
	}

	// Loads the given entity definition's textures into the entity textures list if they haven't been loaded yet.
	void LoadEntityDefTextures(const EntityDefinition &entityDef, std::vector<SceneGraph::LoadedEntityTexture> &entityTextures,
		TextureManager &textureManager, Renderer &renderer)
	{
		const EntityAnimationDefinition &animDef = entityDef.getAnimDef();
		const bool reflective = (entityDef.getType() == EntityDefinition::Type::Doodad) && entityDef.getDoodad().puddle;

		auto processKeyframe = [&entityTextures, &textureManager, &renderer, reflective](
			const EntityAnimationDefinition::Keyframe &keyframe, bool flipped)
		{
			const TextureAsset &textureAsset = keyframe.getTextureAsset();
			const auto cacheIter = std::find_if(entityTextures.begin(), entityTextures.end(),
				[&textureAsset, flipped, reflective](const SceneGraph::LoadedEntityTexture &loadedTexture)
			{
				return (loadedTexture.textureAsset == textureAsset) && (loadedTexture.flipped == flipped) &&
					(loadedTexture.reflective == reflective);
			});

			if (cacheIter == entityTextures.end())
			{
				const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAsset);
				if (!textureBuilderID.has_value())
				{
					DebugLogWarning("Couldn't load entity texture \"" + textureAsset.filename + "\".");
					return;
				}

				const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
				const int textureWidth = textureBuilder.getWidth();
				const int textureHeight = textureBuilder.getHeight();

				ObjectTextureID entityTextureID;
				if (!renderer.tryCreateObjectTexture(textureWidth, textureHeight, false, &entityTextureID))
				{
					DebugLogWarning("Couldn't create entity texture \"" + textureAsset.filename + "\".");
					return;
				}

				ScopedObjectTextureRef entityTextureRef(entityTextureID, renderer);
				DebugAssert(textureBuilder.getType() == TextureBuilder::Type::Paletted);
				const TextureBuilder::PalettedTexture &srcTexture = textureBuilder.getPaletted();
				const uint8_t *srcTexels = srcTexture.texels.get();

				LockedTexture lockedEntityTexture = renderer.lockObjectTexture(entityTextureID);
				if (!lockedEntityTexture.isValid())
				{
					DebugLogWarning("Couldn't lock entity texture \"" + textureAsset.filename + "\".");
					return;
				}

				DebugAssert(!lockedEntityTexture.isTrueColor);
				uint8_t *dstTexels = static_cast<uint8_t*>(lockedEntityTexture.texels);

				for (int y = 0; y < textureHeight; y++)
				{
					for (int x = 0; x < textureWidth; x++)
					{
						// Mirror texture if this texture is for an angle that gets mirrored.
						const int srcIndex = x + (y * textureWidth);
						const int dstIndex = (!flipped ? x : (textureWidth - 1 - x)) + (y * textureWidth);
						dstTexels[dstIndex] = srcTexels[srcIndex];
					}
				}

				renderer.unlockObjectTexture(entityTextureID);

				SceneGraph::LoadedEntityTexture newTexture;
				newTexture.init(textureAsset, flipped, reflective, std::move(entityTextureRef));
				entityTextures.emplace_back(std::move(newTexture));
			}
		};

		for (int i = 0; i < animDef.getStateCount(); i++)
		{
			const EntityAnimationDefinition::State &state = animDef.getState(i);
			for (int j = 0; j < state.getKeyframeListCount(); j++)
			{
				const EntityAnimationDefinition::KeyframeList &keyframeList = state.getKeyframeList(j);
				const bool flipped = keyframeList.isFlipped();
				for (int k = 0; k < keyframeList.getKeyframeCount(); k++)
				{
					const EntityAnimationDefinition::Keyframe &keyframe = keyframeList.getKeyframe(k);
					processKeyframe(keyframe, flipped);
				}
			}
		}
	}
}

void SceneGraph::LoadedVoxelTexture::init(const TextureAsset &textureAsset,
	ScopedObjectTextureRef &&objectTextureRef)
{
	this->textureAsset = textureAsset;
	this->objectTextureRef = std::move(objectTextureRef);
}

void SceneGraph::LoadedEntityTexture::init(const TextureAsset &textureAsset, bool flipped,
	bool reflective, ScopedObjectTextureRef &&objectTextureRef)
{
	this->textureAsset = textureAsset;
	this->flipped = flipped;
	this->reflective = reflective;
	this->objectTextureRef = std::move(objectTextureRef);
}

SceneGraph::LoadedChasmFloorTextureList::LoadedChasmFloorTextureList()
{
	this->type = static_cast<LoadedChasmFloorTextureList::Type>(-1);
	this->paletteIndex = 0;
}

void SceneGraph::LoadedChasmFloorTextureList::initColor(uint8_t paletteIndex,
	ScopedObjectTextureRef &&objectTextureRef)
{
	this->type = LoadedChasmFloorTextureList::Type::Color;
	this->paletteIndex = paletteIndex;
	this->objectTextureRefs.emplace_back(std::move(objectTextureRef));
}

void SceneGraph::LoadedChasmFloorTextureList::initTextured(std::vector<TextureAsset> &&textureAssets,
	std::vector<ScopedObjectTextureRef> &&objectTextureRefs)
{
	this->type = LoadedChasmFloorTextureList::Type::Textured;
	this->textureAssets = std::move(textureAssets);
	this->objectTextureRefs = std::move(objectTextureRefs);
}

void SceneGraph::LoadedChasmTextureKey::init(const ChunkInt2 &chunkPos, Chunk::ChasmID chasmID,
	int chasmFloorListIndex, int chasmWallIndex)
{
	DebugAssert(chasmFloorListIndex >= 0);
	DebugAssert(chasmWallIndex >= 0);
	this->chunkPos = chunkPos;
	this->chasmID = chasmID;
	this->chasmFloorListIndex = chasmFloorListIndex;
	this->chasmWallIndex = chasmWallIndex;
}

ObjectTextureID SceneGraph::getVoxelTextureID(const TextureAsset &textureAsset) const
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

ObjectTextureID SceneGraph::getEntityTextureID(const TextureAsset &textureAsset, bool flipped, bool reflective) const
{
	const auto iter = std::find_if(this->entityTextures.begin(), this->entityTextures.end(),
		[&textureAsset, flipped, reflective](const LoadedEntityTexture &loadedTexture)
	{
		return (loadedTexture.textureAsset == textureAsset) && (loadedTexture.flipped == flipped) &&
			(loadedTexture.reflective == reflective);
	});

	DebugAssertMsg(iter != this->entityTextures.end(), "No loaded entity texture for \"" + textureAsset.filename + "\".");
	const ScopedObjectTextureRef &objectTextureRef = iter->objectTextureRef;
	return objectTextureRef.get();
}

ObjectTextureID SceneGraph::getChasmFloorTextureID(const ChunkInt2 &chunkPos, Chunk::ChasmID chasmID,
	double chasmAnimPercent) const
{
	const auto keyIter = std::find_if(this->chasmTextureKeys.begin(), this->chasmTextureKeys.end(),
		[&chunkPos, chasmID](const LoadedChasmTextureKey &key)
	{
		return (key.chunkPos == chunkPos) && (key.chasmID == chasmID);
	});

	DebugAssertMsg(keyIter != this->chasmTextureKeys.end(), "No chasm texture key for chasm ID \"" +
		std::to_string(chasmID) + "\" in chunk (" + chunkPos.toString() + ").");

	const int floorListIndex = keyIter->chasmFloorListIndex;
	const LoadedChasmFloorTextureList &textureList = this->chasmFloorTextureLists[floorListIndex];
	const std::vector<ScopedObjectTextureRef> &objectTextureRefs = textureList.objectTextureRefs;
	const int textureCount = static_cast<int>(objectTextureRefs.size());
	const int index = std::clamp(static_cast<int>(static_cast<double>(textureCount) * chasmAnimPercent), 0, textureCount - 1);
	DebugAssertIndex(objectTextureRefs, index);
	const ScopedObjectTextureRef &objectTextureRef = objectTextureRefs[index];
	return objectTextureRef.get();
}

ObjectTextureID SceneGraph::getChasmWallTextureID(const ChunkInt2 &chunkPos, Chunk::ChasmID chasmID) const
{
	const auto keyIter = std::find_if(this->chasmTextureKeys.begin(), this->chasmTextureKeys.end(),
		[&chunkPos, chasmID](const LoadedChasmTextureKey &key)
	{
		return (key.chunkPos == chunkPos) && (key.chasmID == chasmID);
	});

	DebugAssertMsg(keyIter != this->chasmTextureKeys.end(), "No chasm texture key for chasm ID \"" +
		std::to_string(chasmID) + "\" in chunk (" + chunkPos.toString() + ").");

	const int wallIndex = keyIter->chasmWallIndex;
	const LoadedChasmWallTexture &wallTexture = this->chasmWallTextures[wallIndex];
	const ScopedObjectTextureRef &objectTextureRef = wallTexture.objectTextureRef;
	return objectTextureRef.get();
}

std::optional<int> SceneGraph::tryGetGraphChunkIndex(const ChunkInt2 &chunkPos) const
{
	for (int i = 0; i < static_cast<int>(this->graphChunks.size()); i++)
	{
		const SceneGraphChunk &graphChunk = this->graphChunks[i];
		if (graphChunk.position == chunkPos)
		{
			return i;
		}
	}

	return std::nullopt;
}

BufferView<const RenderDrawCall> SceneGraph::getDrawCalls() const
{
	return BufferView<const RenderDrawCall>(this->drawCallsCache.data(), static_cast<int>(this->drawCallsCache.size()));
}

void SceneGraph::loadVoxelTextures(const Chunk &chunk, TextureManager &textureManager, Renderer &renderer)
{
	for (int i = 0; i < chunk.getVoxelTextureDefCount(); i++)
	{
		const VoxelTextureDefinition &voxelTextureDef = chunk.getVoxelTextureDef(i);
		sgTexture::LoadVoxelDefTextures(voxelTextureDef, this->voxelTextures, textureManager, renderer);
	}
	
	for (int i = 0; i < chunk.getChasmCount(); i++)
	{
		const ChasmDefinition &chasmDef = chunk.getChasm(i);
		//sgTexture::LoadVoxelDefTextures(chasmDef.) // @todo: handle dry and not-dry chasm texture loading
		
		DebugLogError("loadVoxelTextures() chasms not implemented.");
	}
}

void SceneGraph::loadEntityTextures(const Chunk &chunk, TextureManager &textureManager, Renderer &renderer)
{
	DebugLogError("loadEntityTextures() not implemented.");

	// @todo: sgTexture::LoadEntityDefTextures()... maybe want EntityAnimationDefinition instead of EntityDefinition

	/*
	// Load citizen textures if citizens can exist in the level.
	if ((mapType == MapType::City) || (mapType == MapType::Wilderness))
	{
		DebugAssert(citizenGenInfo.has_value());
		const EntityDefinition &maleEntityDef = *citizenGenInfo->maleEntityDef;
		const EntityDefinition &femaleEntityDef = *citizenGenInfo->femaleEntityDef;
		sgTexture::LoadEntityDefTextures(maleEntityDef, this->entityTextures, textureManager, renderer);
		sgTexture::LoadEntityDefTextures(femaleEntityDef, this->entityTextures, textureManager, renderer);
	}
	*/
}

void SceneGraph::loadVoxelMeshBuffers(SceneGraphChunk &graphChunk, const Chunk &chunk, const RenderCamera &camera,
	double ceilingScale, bool nightLightsAreActive, RendererSystem3D &renderer)
{
	const ChunkInt2 &chunkPos = chunk.getPosition();

	// Add scene graph voxel definitions and create mappings to them.
	for (int meshDefIndex = 0; meshDefIndex < chunk.getVoxelMeshDefCount(); meshDefIndex++)
	{
		const Chunk::VoxelMeshDefID voxelMeshDefID = static_cast<Chunk::VoxelMeshDefID>(meshDefIndex);
		const VoxelMeshDefinition &voxelMeshDef = chunk.getVoxelMeshDef(voxelMeshDefID);

		SceneGraphVoxelDefinition graphVoxelDef;
		if (!voxelMeshDef.isEmpty()) // Only attempt to create buffers for non-air voxels.
		{
			constexpr int componentsPerVertex = MeshUtils::COMPONENTS_PER_VERTEX;
			constexpr int attributesPerVertex = MeshUtils::ATTRIBUTES_PER_VERTEX;

			const int vertexCount = voxelMeshDef.rendererVertexCount;
			if (!renderer.tryCreateVertexBuffer(vertexCount, componentsPerVertex, &graphVoxelDef.vertexBufferID))
			{
				DebugLogError("Couldn't create vertex buffer for voxel mesh ID " + std::to_string(voxelMeshDefID) +
					" in chunk (" + chunk.getPosition().toString() + ").");
				continue;
			}

			if (!renderer.tryCreateAttributeBuffer(vertexCount, attributesPerVertex, &graphVoxelDef.attributeBufferID))
			{
				DebugLogError("Couldn't create attribute buffer for voxel mesh ID " + std::to_string(voxelMeshDefID) +
					" in chunk (" + chunk.getPosition().toString() + ").");
				graphVoxelDef.freeBuffers(renderer);
				continue;
			}

			ArenaMeshUtils::InitCache meshInitCache;

			// Generate mesh geometry and indices for this voxel definition.
			voxelMeshDef.writeRendererGeometryBuffers(ceilingScale, meshInitCache.verticesView, meshInitCache.attributesView);
			voxelMeshDef.writeRendererIndexBuffers(meshInitCache.opaqueIndices0View, meshInitCache.opaqueIndices1View,
				meshInitCache.opaqueIndices2View, meshInitCache.alphaTestedIndices0View);

			renderer.populateVertexBuffer(graphVoxelDef.vertexBufferID,
				BufferView<const double>(meshInitCache.vertices.data(), vertexCount * MeshUtils::COMPONENTS_PER_VERTEX));
			renderer.populateAttributeBuffer(graphVoxelDef.attributeBufferID,
				BufferView<const double>(meshInitCache.attributes.data(), vertexCount * MeshUtils::ATTRIBUTES_PER_VERTEX));

			const int opaqueIndexBufferCount = voxelMeshDef.opaqueIndicesListCount;
			for (int bufferIndex = 0; bufferIndex < opaqueIndexBufferCount; bufferIndex++)
			{
				const int opaqueIndexCount = static_cast<int>(voxelMeshDef.getOpaqueIndicesList(bufferIndex).size());
				IndexBufferID &opaqueIndexBufferID = graphVoxelDef.opaqueIndexBufferIDs[bufferIndex];
				if (!renderer.tryCreateIndexBuffer(opaqueIndexCount, &opaqueIndexBufferID))
				{
					DebugLogError("Couldn't create opaque index buffer for voxel mesh ID " +
						std::to_string(voxelMeshDefID) + " in chunk (" + chunk.getPosition().toString() + ").");
					graphVoxelDef.freeBuffers(renderer);
					continue;
				}

				graphVoxelDef.opaqueIndexBufferIdCount++;

				const auto &indices = *meshInitCache.opaqueIndicesPtrs[bufferIndex];
				renderer.populateIndexBuffer(opaqueIndexBufferID,
					BufferView<const int32_t>(indices.data(), opaqueIndexCount));
			}

			const bool hasAlphaTestedIndexBuffer = voxelMeshDef.alphaTestedIndicesListCount > 0;
			if (hasAlphaTestedIndexBuffer)
			{
				const int alphaTestedIndexCount = static_cast<int>(voxelMeshDef.alphaTestedIndices.size());
				if (!renderer.tryCreateIndexBuffer(alphaTestedIndexCount, &graphVoxelDef.alphaTestedIndexBufferID))
				{
					DebugLogError("Couldn't create alpha-tested index buffer for voxel mesh ID " +
						std::to_string(voxelMeshDefID) + " in chunk (" + chunk.getPosition().toString() + ").");
					graphVoxelDef.freeBuffers(renderer);
					continue;
				}

				renderer.populateIndexBuffer(graphVoxelDef.alphaTestedIndexBufferID,
					BufferView<const int32_t>(meshInitCache.alphaTestedIndices0.data(), alphaTestedIndexCount));
			}
		}

		const SceneGraphVoxelID graphVoxelID = graphChunk.addVoxelDef(std::move(graphVoxelDef));
		graphChunk.voxelDefMappings.emplace(voxelMeshDefID, graphVoxelID);
	}
}

void SceneGraph::loadVoxelDrawCalls(SceneGraphChunk &graphChunk, const Chunk &chunk, double ceilingScale,
	double chasmAnimPercent)
{
	const ChunkInt2 &chunkPos = chunk.getPosition();

	auto addDrawCall = [this, ceilingScale](SceneGraphChunk &graphChunk, SNInt x, int y, WEInt z,
		VertexBufferID vertexBufferID, AttributeBufferID attributeBufferID, IndexBufferID indexBufferID,
		ObjectTextureID textureID, PixelShaderType pixelShaderType, bool allowsBackFaces)
	{
		RenderDrawCall drawCall;
		drawCall.vertexBufferID = vertexBufferID;
		drawCall.attributeBufferID = attributeBufferID;
		drawCall.indexBufferID = indexBufferID;
		drawCall.textureIDs[0] = textureID; // @todo: add another parameter for multi-texturing for chasm walls
		drawCall.vertexShaderType = VertexShaderType::Default;
		drawCall.pixelShaderType = pixelShaderType;
		drawCall.worldSpaceOffset = Double3(
			static_cast<SNDouble>(x),
			static_cast<double>(y) * ceilingScale,
			static_cast<WEDouble>(z));
		drawCall.allowBackFaces = allowsBackFaces;

		graphChunk.voxelDrawCalls.emplace_back(std::move(drawCall));
	};

	// Generate draw calls for each non-air voxel.
	for (WEInt z = 0; z < Chunk::DEPTH; z++)
	{
		for (int y = 0; y < chunk.getHeight(); y++)
		{
			for (SNInt x = 0; x < Chunk::WIDTH; x++)
			{
				const Chunk::VoxelMeshDefID voxelMeshDefID = chunk.getVoxelMeshDefID(x, y, z);
				const Chunk::VoxelTextureDefID voxelTextureDefID = chunk.getVoxelTextureDefID(x, y, z);
				const Chunk::VoxelTraitsDefID voxelTraitsDefID = chunk.getVoxelTraitsDefID(x, y, z);
				const VoxelMeshDefinition &voxelMeshDef = chunk.getVoxelMeshDef(voxelMeshDefID);
				const VoxelTextureDefinition &voxelTextureDef = chunk.getVoxelTextureDef(voxelTextureDefID);
				const VoxelTraitsDefinition &voxelTraitsDef = chunk.getVoxelTraitsDef(voxelTraitsDefID);
				if (voxelMeshDef.isEmpty())
				{
					continue;
				}

				const auto defIter = graphChunk.voxelDefMappings.find(voxelMeshDefID);
				DebugAssert(defIter != graphChunk.voxelDefMappings.end());
				const SceneGraphVoxelID graphVoxelID = defIter->second;
				graphChunk.voxels.set(x, y, z, graphVoxelID);

				// Convert voxel XYZ to world space.
				const NewInt2 worldXZ = VoxelUtils::chunkVoxelToNewVoxel(chunk.getPosition(), VoxelInt2(x, z));
				const int worldY = y;

				const bool allowsBackFaces = voxelMeshDef.allowsBackFaces;
				const ArenaTypes::VoxelType voxelType = voxelTraitsDef.type;

				Chunk::ChasmID chasmID;
				const bool usesVoxelTextures = !chunk.tryGetChasmID(x, y, z, &chasmID);

				const SceneGraphVoxelDefinition &graphVoxelDef = graphChunk.voxelDefs[graphVoxelID];
				for (int bufferIndex = 0; bufferIndex < graphVoxelDef.opaqueIndexBufferIdCount; bufferIndex++)
				{
					ObjectTextureID textureID = -1;

					if (usesVoxelTextures)
					{
						const int textureAssetIndex = sgTexture::GetVoxelOpaqueTextureAssetIndex(voxelType, bufferIndex);
						const auto voxelTextureIter = std::find_if(this->voxelTextures.begin(), this->voxelTextures.end(),
							[&voxelTextureDef, textureAssetIndex](const SceneGraph::LoadedVoxelTexture &loadedTexture)
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
						textureID = this->getChasmFloorTextureID(chunkPos, chasmID, chasmAnimPercent);
					}

					if (textureID < 0)
					{
						continue;
					}

					const IndexBufferID opaqueIndexBufferID = graphVoxelDef.opaqueIndexBufferIDs[bufferIndex];
					addDrawCall(graphChunk, worldXZ.x, worldY, worldXZ.y, graphVoxelDef.vertexBufferID,
						graphVoxelDef.attributeBufferID, opaqueIndexBufferID, textureID, PixelShaderType::Opaque,
						allowsBackFaces);
				}

				if (graphVoxelDef.alphaTestedIndexBufferID >= 0)
				{
					ObjectTextureID textureID = -1;

					if (usesVoxelTextures)
					{
						const int textureAssetIndex = sgTexture::GetVoxelAlphaTestedTextureAssetIndex(voxelType);
						const auto voxelTextureIter = std::find_if(this->voxelTextures.begin(), this->voxelTextures.end(),
							[&voxelTextureDef, textureAssetIndex](const SceneGraph::LoadedVoxelTexture &loadedTexture)
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
					}
					else
					{
						textureID = this->getChasmWallTextureID(chunkPos, chasmID);
					}

					if (textureID < 0)
					{
						continue;
					}

					addDrawCall(graphChunk, worldXZ.x, worldY, worldXZ.y, graphVoxelDef.vertexBufferID,
						graphVoxelDef.attributeBufferID, graphVoxelDef.alphaTestedIndexBufferID, textureID,
						PixelShaderType::AlphaTested, allowsBackFaces);
				}
			}
		}
	}
}

/*void SceneGraph::loadEntities(const LevelInstance &levelInst, const RenderCamera &camera,
	const EntityDefinitionLibrary &entityDefLibrary, bool nightLightsAreActive, bool playerHasLight,
	RendererSystem3D &renderer)
{
	DebugAssert(!this->graphChunks.empty());

	// @todo
	DebugNotImplemented();
}

void SceneGraph::loadSky(const SkyInstance &skyInst, double daytimePercent, double latitude, RendererSystem3D &renderer)
{
	// @todo
	DebugNotImplemented();
}

void SceneGraph::loadWeather(const SkyInstance &skyInst, double daytimePercent, RendererSystem3D &renderer)
{
	// @todo
	DebugNotImplemented();
}*/

void SceneGraph::loadScene(const LevelInstance &levelInst, const SkyInstance &skyInst,
	const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
	const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, const RenderCamera &camera,
	double chasmAnimPercent, bool nightLightsAreActive, bool playerHasLight, double daytimePercent,
	double latitude, const EntityDefinitionLibrary &entityDefLibrary, TextureManager &textureManager,
	Renderer &renderer, RendererSystem3D &renderer3D)
{
	DebugAssert(this->graphChunks.empty());

	const double ceilingScale = levelInst.getCeilingScale();
	const ChunkManager &chunkManager = levelInst.getChunkManager();
	for (int i = 0; i < chunkManager.getChunkCount(); i++)
	{
		const Chunk &chunk = chunkManager.getChunk(i);
		
		SceneGraphChunk graphChunk;
		graphChunk.init(chunk.getPosition(), chunk.getHeight());

		this->loadVoxelTextures(chunk, textureManager, renderer);
		this->loadVoxelMeshBuffers(graphChunk, chunk, camera, ceilingScale, nightLightsAreActive, renderer3D);
		this->loadVoxelDrawCalls(graphChunk, chunk, ceilingScale, chasmAnimPercent);
		//this->loadEntities(levelInst, camera, entityDefLibrary, nightLightsAreActive, playerHasLight, renderer3D);

		this->graphChunks.emplace_back(std::move(graphChunk));
	}

	//this->loadSky(skyInst, daytimePercent, latitude, renderer3D);
	//this->loadWeather(skyInst, daytimePercent, renderer3D);

	// Gather draw calls from each chunk.
	// @todo: do this in update() so it can operate on dirty stuff from chunk manager/entity manager/etc.
	this->drawCallsCache.clear();
	for (size_t i = 0; i < this->graphChunks.size(); i++)
	{
		const SceneGraphChunk &graphChunk = this->graphChunks[i];
		this->drawCallsCache.insert(this->drawCallsCache.end(), graphChunk.voxelDrawCalls.begin(), graphChunk.voxelDrawCalls.end());
	}
}

void SceneGraph::unloadScene(RendererSystem3D &renderer)
{
	this->voxelTextures.clear();
	this->entityTextures.clear();
	this->chasmFloorTextureLists.clear();
	this->chasmWallTextures.clear();
	this->chasmTextureKeys.clear();

	// Free vertex/attribute/index buffer IDs from renderer.
	for (SceneGraphChunk &chunk : this->graphChunks)
	{
		chunk.freeBuffers(renderer);
	}

	this->graphChunks.clear();
	this->drawCallsCache.clear();
}

/*void SceneGraph::updateVoxels(const LevelInstance &levelInst, const RenderCamera &camera, double chasmAnimPercent,
	bool nightLightsAreActive, RendererSystem3D &renderer)
{
	const ChunkManager &chunkManager = levelInst.getChunkManager();
	const int chunkCount = chunkManager.getChunkCount();

	// Remove stale graph chunks.
	for (int i = static_cast<int>(this->graphChunks.size()) - 1; i >= 0; i--)
	{
		const SceneGraphChunk &graphChunk = this->graphChunks[i];
		const ChunkInt2 &graphChunkPos = graphChunk.position;

		bool isStale = true;
		for (int j = 0; j < chunkCount; j++)
		{
			const Chunk &chunk = chunkManager.getChunk(j);
			const ChunkInt2 &chunkPos = chunk.getPosition();
			if (chunkPos == graphChunkPos)
			{
				isStale = false;
				break;
			}
		}

		if (isStale)
		{
			this->graphChunks.erase(this->graphChunks.begin() + i);
		}
	}

	// Insert new empty graph chunks (to have their voxels updated by the associated chunk's dirty voxels).
	for (int i = 0; i < chunkCount; i++)
	{
		const Chunk &chunk = chunkManager.getChunk(i);
		const ChunkInt2 &chunkPos = chunk.getPosition();

		bool shouldInsert = true;
		for (int j = 0; j < static_cast<int>(this->graphChunks.size()); j++)
		{
			const SceneGraphChunk &graphChunk = this->graphChunks[j];
			const ChunkInt2 &graphChunkPos = graphChunk.position;
			if (graphChunkPos == chunkPos)
			{
				shouldInsert = false;
				break;
			}
		}

		if (shouldInsert)
		{
			SceneGraphChunk graphChunk;
			graphChunk.init(chunkPos, chunk.getHeight());
			this->graphChunks.emplace_back(std::move(graphChunk));
		}
	}

	// @todo: decide how to load voxels into these new graph chunks - maybe want to do the chunk adding/removing
	// before updateVoxels(), same as how loadVoxels() expects the chunks to already be there (albeit empty).

	// Arbitrary value, just needs to be long enough to touch the farthest chunks in practice.
	// - @todo: maybe use far clipping plane value?
	constexpr double frustumLength = 1000.0;

	const Double2 cameraEye2D(camera.point.x, camera.point.z);
	const Double2 cameraFrustumLeftPoint2D(
		camera.point.x + ((camera.forwardScaled.x - camera.rightScaled.x) * frustumLength),
		camera.point.z + ((camera.forwardScaled.z - camera.rightScaled.z) * frustumLength));
	const Double2 cameraFrustumRightPoint2D(
		camera.point.x + ((camera.forwardScaled.x + camera.rightScaled.x) * frustumLength),
		camera.point.z + ((camera.forwardScaled.z + camera.rightScaled.z) * frustumLength));

	// Update dirty voxels in each scene graph chunk.
	// @todo: animating voxel instances should be set dirty every frame in Chunk::update() or whatever
	for (int i = 0; i < chunkCount; i++)
	{
		const Chunk &chunk = chunkManager.getChunk(i);
		const SNInt chunkWidth = Chunk::WIDTH;
		const int chunkHeight = chunk.getHeight();
		const WEInt chunkDepth = Chunk::DEPTH;

		const ChunkInt2 chunkPos = chunk.getPosition();

		auto getVoxelFadePercentOrDefault = [&chunk](const VoxelInt3 &voxelPos)
		{
			const VoxelInstance *fadingVoxelInst = chunk.tryGetVoxelInst(voxelPos, VoxelInstance::Type::Fading);
			return (fadingVoxelInst != nullptr) ? fadingVoxelInst->getFadeState().getPercentFaded() : 0.0;
		};

		auto getVoxelOpenDoorPercentOrDefault = [&chunk](const VoxelInt3 &voxelPos)
		{
			const VoxelInstance *openDoorVoxelInst = chunk.tryGetVoxelInst(voxelPos, VoxelInstance::Type::OpenDoor);
			return (openDoorVoxelInst != nullptr) ? openDoorVoxelInst->getDoorState().getPercentOpen() : 0.0;
		};

		// Get the scene graph chunk associated with the world space chunk.
		const auto graphChunkIter = std::find_if(this->graphChunks.begin(), this->graphChunks.end(),
			[&chunkPos](const SceneGraphChunk &graphChunk)
		{
			return graphChunk.position == chunkPos;
		});

		DebugAssertMsg(graphChunkIter != this->graphChunks.end(), "Expected scene graph chunk (" + chunkPos.toString() + ") to have been added.");
		SceneGraphChunk &graphChunk = *graphChunkIter;

		// @todo: these two buffers could probably be removed if SceneGraphVoxel is going to store them instead.
		std::array<RenderTriangle, sgGeometry::MAX_TRIANGLES_PER_VOXEL> opaqueTrianglesBuffer, alphaTestedTrianglesBuffer;
		int opaqueTriangleCount = 0;
		int alphaTestedTriangleCount = 0;

		for (int dirtyVoxelIndex = 0; dirtyVoxelIndex < chunk.getDirtyVoxelCount(); dirtyVoxelIndex++)
		{
			const VoxelInt3 &voxelPos = chunk.getDirtyVoxel(dirtyVoxelIndex);
			const Chunk::VoxelID voxelID = chunk.getVoxel(voxelPos.x, voxelPos.y, voxelPos.z);
			const VoxelDefinition &voxelDef = chunk.getVoxelDef(voxelID);

			opaqueTriangleCount = 0;
			alphaTestedTriangleCount = 0;
			if (voxelDef.type == ArenaTypes::VoxelType::Wall)
			{
				const VoxelDefinition::WallData &wall = voxelDef.wall;
				const ObjectMaterialID sideMaterialID = levelInst.getVoxelMaterialID(wall.sideTextureAsset);
				const ObjectMaterialID floorMaterialID = levelInst.getVoxelMaterialID(wall.floorTextureAsset);
				const ObjectMaterialID ceilingMaterialID = levelInst.getVoxelMaterialID(wall.ceilingTextureAsset);
				const double fadePercent = getVoxelFadePercentOrDefault(voxelPos);
				sgGeometry::WriteWall(chunkPos, voxelPos, ceilingScale, sideMaterialID, floorMaterialID, ceilingMaterialID, fadePercent,
					BufferView<RenderTriangle>(opaqueTrianglesBuffer.data(), 12), &opaqueTriangleCount);
			}
			else if (voxelDef.type == ArenaTypes::VoxelType::Floor)
			{
				const VoxelDefinition::FloorData &floor = voxelDef.floor;
				const ObjectMaterialID materialID = levelInst.getVoxelMaterialID(floor.textureAsset);
				const double fadePercent = getVoxelFadePercentOrDefault(voxelPos);
				sgGeometry::WriteFloor(chunkPos, voxelPos, ceilingScale, materialID, fadePercent,
					BufferView<RenderTriangle>(opaqueTrianglesBuffer.data(), 2), &opaqueTriangleCount);
			}
			else if (voxelDef.type == ArenaTypes::VoxelType::Ceiling)
			{
				const VoxelDefinition::CeilingData &ceiling = voxelDef.ceiling;
				const ObjectMaterialID materialID = levelInst.getVoxelMaterialID(ceiling.textureAsset);
				const double fadePercent = getVoxelFadePercentOrDefault(voxelPos);
				sgGeometry::WriteCeiling(chunkPos, voxelPos, ceilingScale, materialID, fadePercent,
					BufferView<RenderTriangle>(opaqueTrianglesBuffer.data(), 2), &opaqueTriangleCount);
			}
			else if (voxelDef.type == ArenaTypes::VoxelType::Raised)
			{
				const VoxelDefinition::RaisedData &raised = voxelDef.raised;
				const ObjectMaterialID sideMaterialID = levelInst.getVoxelMaterialID(raised.sideTextureAsset);
				const ObjectMaterialID floorMaterialID = levelInst.getVoxelMaterialID(raised.floorTextureAsset);
				const ObjectMaterialID ceilingMaterialID = levelInst.getVoxelMaterialID(raised.ceilingTextureAsset);
				const double fadePercent = getVoxelFadePercentOrDefault(voxelPos);
				sgGeometry::WriteRaised(chunkPos, voxelPos, ceilingScale, raised.yOffset, raised.ySize,
					raised.vTop, raised.vBottom, sideMaterialID, floorMaterialID, ceilingMaterialID, fadePercent,
					BufferView<RenderTriangle>(opaqueTrianglesBuffer.data(), 4), &opaqueTriangleCount,
					BufferView<RenderTriangle>(alphaTestedTrianglesBuffer.data(), 8), &alphaTestedTriangleCount);
			}
			else if (voxelDef.type == ArenaTypes::VoxelType::Diagonal)
			{
				const VoxelDefinition::DiagonalData &diagonal = voxelDef.diagonal;
				const ObjectMaterialID materialID = levelInst.getVoxelMaterialID(diagonal.textureAsset);
				const double fadePercent = getVoxelFadePercentOrDefault(voxelPos);
				sgGeometry::WriteDiagonal(chunkPos, voxelPos, ceilingScale, diagonal.type1, materialID, fadePercent,
					BufferView<RenderTriangle>(opaqueTrianglesBuffer.data(), 4), &opaqueTriangleCount);
			}
			else if (voxelDef.type == ArenaTypes::VoxelType::TransparentWall)
			{
				const VoxelDefinition::TransparentWallData &transparentWall = voxelDef.transparentWall;
				const ObjectMaterialID materialID = levelInst.getVoxelMaterialID(transparentWall.textureAsset);
				sgGeometry::WriteTransparentWall(chunkPos, voxelPos, ceilingScale, materialID,
					BufferView<RenderTriangle>(alphaTestedTrianglesBuffer.data(), 8), &alphaTestedTriangleCount);
			}
			else if (voxelDef.type == ArenaTypes::VoxelType::Edge)
			{
				const VoxelDefinition::EdgeData &edge = voxelDef.edge;
				const ObjectMaterialID materialID = levelInst.getVoxelMaterialID(edge.textureAsset);
				sgGeometry::WriteEdge(chunkPos, voxelPos, ceilingScale, edge.facing, edge.yOffset, edge.flipped, materialID,
					BufferView<RenderTriangle>(alphaTestedTrianglesBuffer.data(), 4), &alphaTestedTriangleCount);
			}
			else if (voxelDef.type == ArenaTypes::VoxelType::Chasm)
			{
				const VoxelInstance *chasmVoxelInst = chunk.tryGetVoxelInst(voxelPos, VoxelInstance::Type::Chasm);
				bool hasNorthFace = false;
				bool hasSouthFace = false;
				bool hasEastFace = false;
				bool hasWestFace = false;
				if (chasmVoxelInst != nullptr)
				{
					const VoxelInstance::ChasmState &chasmState = chasmVoxelInst->getChasmState();
					hasNorthFace = chasmState.getNorth();
					hasSouthFace = chasmState.getSouth();
					hasEastFace = chasmState.getEast();
					hasWestFace = chasmState.getWest();
				}

				const VoxelDefinition::ChasmData &chasm = voxelDef.chasm;
				const bool isDry = chasm.type == ArenaTypes::ChasmType::Dry;
				const ObjectMaterialID floorMaterialID = levelInst.getChasmFloorMaterialID(chasm.type, chasmAnimPercent);
				const ObjectMaterialID sideMaterialID = levelInst.getChasmWallMaterialID(chasm.type, chasmAnimPercent, chasm.textureAsset);
				sgGeometry::WriteChasm(chunkPos, voxelPos, ceilingScale, hasNorthFace, hasSouthFace, hasEastFace, hasWestFace,
					isDry, floorMaterialID, sideMaterialID, BufferView<RenderTriangle>(opaqueTrianglesBuffer.data(), 10), &opaqueTriangleCount);
			}
			else if (voxelDef.type == ArenaTypes::VoxelType::Door)
			{
				const VoxelDefinition::DoorData &door = voxelDef.door;
				const double animPercent = getVoxelOpenDoorPercentOrDefault(voxelPos);
				const ObjectMaterialID materialID = levelInst.getVoxelMaterialID(door.textureAsset);
				sgGeometry::WriteDoor(chunkPos, voxelPos, ceilingScale, door.type, animPercent,
					materialID, BufferView<RenderTriangle>(alphaTestedTrianglesBuffer.data(), 16), &alphaTestedTriangleCount);
			}

			SceneGraphVoxel &graphVoxel = graphChunk.voxels.get(voxelPos.x, voxelPos.y, voxelPos.z);
			Buffer<RenderTriangle> &dstOpaqueTriangles = graphVoxel.opaqueTriangles;
			Buffer<RenderTriangle> &dstAlphaTestedTriangles = graphVoxel.alphaTestedTriangles;
			if (opaqueTriangleCount > 0)
			{
				const auto srcStart = opaqueTrianglesBuffer.cbegin();
				const auto srcEnd = srcStart + opaqueTriangleCount;
				dstOpaqueTriangles.init(opaqueTriangleCount);
				std::copy(srcStart, srcEnd, dstOpaqueTriangles.get());
			}
			else if ((opaqueTriangleCount == 0) && (dstOpaqueTriangles.getCount() > 0))
			{
				dstOpaqueTriangles.clear();
			}

			if (alphaTestedTriangleCount > 0)
			{
				const auto srcStart = alphaTestedTrianglesBuffer.cbegin();
				const auto srcEnd = srcStart + alphaTestedTriangleCount;
				dstAlphaTestedTriangles.init(alphaTestedTriangleCount);
				std::copy(srcStart, srcEnd, dstAlphaTestedTriangles.get());
			}
			else if ((alphaTestedTriangleCount == 0) && (dstAlphaTestedTriangles.getCount() > 0))
			{
				dstAlphaTestedTriangles.clear();
			}
		}
	}

	// Regenerate draw lists.
	// @todo: maybe this is where we need to call the voxel animation logic functions so we know what material ID
	// to use for chasms, etc.? Might be good to finally bring in the VoxelRenderDefinition, etc..
	for (int i = 0; i < static_cast<int>(this->graphChunks.size()); i++)
	{
		const Chunk &chunk = chunkManager.getChunk(i);
		const ChunkInt2 &chunkPos = chunk.getPosition();
		const ChunkInt2 relativeChunkPos = chunkPos - camera.chunk; // Relative to camera chunk.
		constexpr double chunkDimReal = static_cast<double>(ChunkUtils::CHUNK_DIM);

		// Top right and bottom left world space corners of this chunk.
		const NewDouble2 chunkTR2D = VoxelUtils::chunkPointToNewPoint(relativeChunkPos, VoxelDouble2::Zero);
		const NewDouble2 chunkBL2D = VoxelUtils::chunkPointToNewPoint(relativeChunkPos, VoxelDouble2(chunkDimReal, chunkDimReal));

		// See if this chunk's geometry should reach the draw list.
		const bool isChunkVisible = MathUtils::triangleRectangleIntersection(
			cameraEye2D, cameraFrustumRightPoint2D, cameraFrustumLeftPoint2D, chunkTR2D, chunkBL2D);

		if (!isChunkVisible)
		{
			continue;
		}

		const SceneGraphChunk &graphChunk = this->graphChunks[i];
		const Buffer3D<SceneGraphVoxel> &graphChunkVoxels = graphChunk.voxels;

		for (WEInt z = 0; z < graphChunkVoxels.getDepth(); z++)
		{
			for (SNInt x = 0; x < graphChunkVoxels.getWidth(); x++)
			{
				const VoxelInt2 voxelColumnPos(x, z);
				const VoxelDouble2 voxelColumnPoint(
					static_cast<SNDouble>(voxelColumnPos.x),
					static_cast<WEDouble>(voxelColumnPos.y));
				const NewDouble2 voxelTR2D = VoxelUtils::chunkPointToNewPoint(relativeChunkPos, voxelColumnPoint);
				const NewDouble2 voxelBL2D = VoxelUtils::chunkPointToNewPoint(relativeChunkPos, voxelColumnPoint + VoxelDouble2(1.0, 1.0));

				// See if this voxel's geometry should reach the draw list.
				// @todo: the 2D camera triangle here is not correct when looking up or down, currently results in missing triangles on-screen; need a larger triangle based on the angle to compensate.
				// @todo: replace this per-voxel-column operation with a quadtree look-up that can do large groups of voxel columns at once
				const bool isVoxelColumnVisible = MathUtils::triangleRectangleIntersection(
					cameraEye2D, cameraFrustumRightPoint2D, cameraFrustumLeftPoint2D, voxelTR2D, voxelBL2D);

				if (!isVoxelColumnVisible)
				{
					continue;
				}

				for (int y = 0; y < graphChunkVoxels.getHeight(); y++)
				{
					const SceneGraphVoxel &graphVoxel = graphChunkVoxels.get(x, y, z);
					const Buffer<RenderTriangle> &srcOpaqueTriangles = graphVoxel.opaqueTriangles;
					const Buffer<RenderTriangle> &srcAlphaTestedTriangles = graphVoxel.alphaTestedTriangles;
					const int srcOpaqueTriangleCount = srcOpaqueTriangles.getCount();
					const int srcAlphaTestedTriangleCount = srcAlphaTestedTriangles.getCount();
					if (srcOpaqueTriangleCount > 0)
					{
						const RenderTriangle *srcStart = srcOpaqueTriangles.get();
						const RenderTriangle *srcEnd = srcOpaqueTriangles.end();
						this->opaqueVoxelTriangles.insert(this->opaqueVoxelTriangles.end(), srcStart, srcEnd);
					}

					if (srcAlphaTestedTriangleCount > 0)
					{
						const RenderTriangle *srcStart = srcAlphaTestedTriangles.get();
						const RenderTriangle *srcEnd = srcAlphaTestedTriangles.end();
						this->alphaTestedVoxelTriangles.insert(this->alphaTestedVoxelTriangles.end(), srcStart, srcEnd);
					}
				}
			}
		}
	}

	// @todo: sort opaque chunk geometry near to far
	// @todo: sort alpha-tested chunk geometry far to near
	// ^ for both of these, the goal is so we can essentially just memcpy each chunk's geometry into the scene graph's draw lists.
}

void SceneGraph::updateEntities(const LevelInstance &levelInst, const RenderCamera &camera,
	const EntityDefinitionLibrary &entityDefLibrary, bool nightLightsAreActive, bool playerHasLight,
	RendererSystem3D &renderer)
{
	DebugNotImplemented();
	/*const ChunkManager &chunkManager = levelInst.getChunkManager();
	const int chunkCount = chunkManager.getChunkCount();

	const EntityManager &entityManager = levelInst.getEntityManager();
	std::vector<const Entity*> entityPtrs;

	const CoordDouble2 cameraPos2D(camera.chunk, VoxelDouble2(camera.point.x, camera.point.z));
	const VoxelDouble3 entityDir = -camera.forward;

	for (int i = 0; i < chunkCount; i++)
	{
		const Chunk &chunk = chunkManager.getChunk(i);
		const ChunkInt2 &chunkPosition = chunk.getPosition();
		const int entityCountInChunk = entityManager.getCountInChunk(chunkPosition);
		entityPtrs.resize(entityCountInChunk);
		const int writtenEntityCount = entityManager.getEntitiesInChunk(
			chunkPosition, entityPtrs.data(), static_cast<int>(entityPtrs.size()));
		DebugAssert(writtenEntityCount == entityCountInChunk);

		for (const Entity *entityPtr : entityPtrs)
		{
			if (entityPtr != nullptr)
			{
				const Entity &entity = *entityPtr;
				const CoordDouble2 &entityCoord = entity.getPosition();
				const EntityDefID entityDefID = entity.getDefinitionID();
				const EntityDefinition &entityDef = entityManager.getEntityDef(entityDefID, entityDefLibrary);
				const EntityAnimationDefinition &animDef = entityDef.getAnimDef();
				//const EntityAnimationInstance &animInst = entity.getAnimInstance();

				EntityVisibilityState3D visState;
				entityManager.getEntityVisibilityState3D(entity, cameraPos2D, ceilingScale, chunkManager, entityDefLibrary, visState);
				const EntityAnimationDefinition::State &animState = animDef.getState(visState.stateIndex);
				const EntityAnimationDefinition::KeyframeList &animKeyframeList = animState.getKeyframeList(visState.angleIndex);
				const EntityAnimationDefinition::Keyframe &animKeyframe = animKeyframeList.getKeyframe(visState.keyframeIndex);
				const TextureAsset &textureAsset = animKeyframe.getTextureAsset();
				const bool flipped = animKeyframeList.isFlipped();
				const bool reflective = (entityDef.getType() == EntityDefinition::Type::Doodad) && (entityDef.getDoodad().puddle);
				const ObjectMaterialID materialID = levelInst.getEntityMaterialID(textureAsset, flipped, reflective);

				std::array<RenderTriangle, 2> entityTrianglesBuffer;
				sgGeometry::WriteEntity(visState.flatPosition.chunk, visState.flatPosition.point, materialID,
					animKeyframe.getWidth(), animKeyframe.getHeight(), entityDir,
					BufferView<RenderTriangle>(entityTrianglesBuffer.data(), static_cast<int>(entityTrianglesBuffer.size())));

				const auto srcStart = entityTrianglesBuffer.cbegin();
				const auto srcEnd = srcStart + 2;
				this->entityTriangles.insert(this->entityTriangles.end(), srcStart, srcEnd);
			}
		}
	}
}

void SceneGraph::updateSky(const SkyInstance &skyInst, double daytimePercent, double latitude)
{
	//this->clearSky();
	DebugNotImplemented();
}

void SceneGraph::updateWeather(const SkyInstance &skyInst)
{
	// @todo
	DebugNotImplemented();
}

/*void SceneGraph::updateScene(const LevelInstance &levelInst, const SkyInstance &skyInst,
	const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
	const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, const RenderCamera &camera,
	double chasmAnimPercent, bool nightLightsAreActive, bool playerHasLight,
	double daytimePercent, double latitude, const EntityDefinitionLibrary &entityDefLibrary,
	TextureManager &textureManager, RendererSystem3D &renderer)
{
	// @todo: update chunks first so we know which chunks need to be fully loaded in with loadVoxels(), etc..
	DebugNotImplemented();

	this->updateVoxels(levelInst, camera, chasmAnimPercent, nightLightsAreActive, renderer);
	this->updateEntities(levelInst, camera, entityDefLibrary, nightLightsAreActive, playerHasLight, renderer);
	this->updateSky(skyInst, daytimePercent, latitude);
	this->updateWeather(skyInst);
}*/
