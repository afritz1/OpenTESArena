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

	bool LoadedChasmFloorComparer(const SceneGraph::LoadedChasmFloorTextureList &textureList, const ChasmDefinition &chasmDef)
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

	void LoadChasmDefTextures(Chunk::ChasmID chasmID, const Chunk &chunk, std::vector<SceneGraph::LoadedChasmFloorTextureList> &chasmTextureLists,
		std::vector<SceneGraph::LoadedChasmTextureKey> &chasmTextureKeys, TextureManager &textureManager, Renderer &renderer)
	{
		const ChunkInt2 chunkPos = chunk.getPosition();
		const ChasmDefinition &chasmDef = chunk.getChasm(chasmID);

		// Check if this chasm already has a mapping.
		const auto keyIter = std::find_if(chasmTextureKeys.begin(), chasmTextureKeys.end(),
			[chasmID, &chunkPos](const SceneGraph::LoadedChasmTextureKey &loadedKey)
		{
			return (loadedKey.chasmID == chasmID) && (loadedKey.chunkPos == chunkPos);
		});

		if (keyIter != chasmTextureKeys.end())
		{
			DebugLog("Chasm texture key already loaded for chasm \"" + std::to_string(chasmID) + "\" in chunk (" + chunkPos.toString() + ").");
			return;
		}

		// Check if any loaded chasms reference the same assets.
		const auto chasmIter = std::find_if(chasmTextureLists.begin(), chasmTextureLists.end(),
			[&chasmDef](const SceneGraph::LoadedChasmFloorTextureList &textureList)
		{
			return LoadedChasmFloorComparer(textureList, chasmDef);
		});

		SceneGraph::LoadedChasmTextureKey key;
		if (chasmIter != chasmTextureLists.end())
		{
			// Add a key referencing that same mapping for this chasmID + chunkPos.
			DebugLog("Chasm texture(s) already loaded for chasm \"" + std::to_string(chasmID) + "\" in chunk (" + chunkPos.toString() + ").");

			const int loadedChasmFloorListIndex = static_cast<int>(std::distance(chasmTextureLists.begin(), chasmIter));
			const int loadedChasmWallIndex = loadedChasmFloorListIndex; // @todo
			DebugLogError("Not implemented: loadedChasmWallIndex");
			key.init(chunkPos, chasmID, loadedChasmFloorListIndex, loadedChasmWallIndex);
		}
		else
		{
			// Load the required textures and add a key for them.
			SceneGraph::LoadedChasmFloorTextureList newTextureList;
			if (chasmDef.animType == ChasmDefinition::AnimationType::SolidColor)
			{
				// Dry chasms are a single color, no texture asset.
				ObjectTextureID dryChasmTextureID;
				if (!renderer.tryCreateObjectTexture(1, 1, false, &dryChasmTextureID))
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

				DebugAssert(!lockedTexture.isTrueColor);
				uint8_t *texels = static_cast<uint8_t*>(lockedTexture.texels);
				*texels = paletteIndex;
				renderer.unlockObjectTexture(dryChasmTextureID);

				newTextureList.initColor(paletteIndex, std::move(dryChasmTextureRef));
				chasmTextureLists.emplace_back(std::move(newTextureList));
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
				chasmTextureLists.emplace_back(std::move(newTextureList));
			}
			else
			{
				DebugNotImplementedMsg(std::to_string(static_cast<int>(chasmDef.animType)));
			}

			const int chasmFloorListIndex = static_cast<int>(chasmTextureLists.size()) - 1;
			const int chasmWallIndex = chasmFloorListIndex; // @todo
			key.init(chunkPos, chasmID, chasmFloorListIndex, chasmWallIndex);
		}

		chasmTextureKeys.emplace_back(std::move(key));
	}
}

void SceneGraph::LoadedVoxelTexture::init(const TextureAsset &textureAsset,
	ScopedObjectTextureRef &&objectTextureRef)
{
	this->textureAsset = textureAsset;
	this->objectTextureRef = std::move(objectTextureRef);
}

SceneGraph::LoadedChasmFloorTextureList::LoadedChasmFloorTextureList()
{
	this->animType = static_cast<ChasmDefinition::AnimationType>(-1);
	this->paletteIndex = 0;
}

void SceneGraph::LoadedChasmFloorTextureList::initColor(uint8_t paletteIndex,
	ScopedObjectTextureRef &&objectTextureRef)
{
	this->animType = ChasmDefinition::AnimationType::SolidColor;
	this->paletteIndex = paletteIndex;
	this->objectTextureRefs.emplace_back(std::move(objectTextureRef));
}

void SceneGraph::LoadedChasmFloorTextureList::initTextured(std::vector<TextureAsset> &&textureAssets,
	std::vector<ScopedObjectTextureRef> &&objectTextureRefs)
{
	this->animType = ChasmDefinition::AnimationType::Animated;
	this->textureAssets = std::move(textureAssets);
	this->objectTextureRefs = std::move(objectTextureRefs);
}

int SceneGraph::LoadedChasmFloorTextureList::getTextureIndex(double chasmAnimPercent) const
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
	const int index = textureList.getTextureIndex(chasmAnimPercent);
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
		const Chunk::ChasmID chasmID = static_cast<Chunk::ChasmID>(i);
		sgTexture::LoadChasmDefTextures(chasmID, chunk, this->chasmFloorTextureLists, this->chasmTextureKeys,
			textureManager, renderer);
	}
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
