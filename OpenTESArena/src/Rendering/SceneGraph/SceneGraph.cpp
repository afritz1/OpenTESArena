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
#include "../../World/ChunkManager.h"
#include "../../World/LevelInstance.h"
#include "../../World/MapDefinition.h"
#include "../../World/MapType.h"
#include "../../World/VoxelFacing2D.h"
#include "../../World/VoxelGeometry.h"

#include "components/debug/Debug.h"

namespace sgGeometry
{
	// Quad texture coordinates (top left, top right, etc.).
	const Double2 UV_TL(0.0, 0.0);
	const Double2 UV_TR(1.0, 0.0);
	const Double2 UV_BL(0.0, 1.0);
	const Double2 UV_BR(1.0, 1.0);

	// Makes the world space position of where a voxel should be.
	Double3 MakeVoxelPosition(const ChunkInt2 &chunk, const VoxelInt3 &voxel, double ceilingScale)
	{
		const Int3 absoluteVoxel = VoxelUtils::chunkVoxelToNewVoxel(chunk, voxel);
		return Double3(
			static_cast<double>(absoluteVoxel.x),
			static_cast<double>(absoluteVoxel.y) * ceilingScale,
			static_cast<double>(absoluteVoxel.z));
	}

	// Makes the world space position of where an entity's bottom center should be. The ceiling scale is already
	// in the 3D point.
	Double3 MakeEntityPosition(const ChunkInt2 &chunk, const VoxelDouble3 &point)
	{
		return VoxelUtils::chunkPointToNewPoint(chunk, point);
	}

	// Makes a world space triangle. The given vertices are in model space and contain the 0->1 values where 1 is
	// a voxel corner.
	void MakeWorldSpaceVertices(const Double3 &voxelPosition, const Double3 &v0, const Double3 &v1, const Double3 &v2,
		double ceilingScale, Double3 *outV0, Double3 *outV1, Double3 *outV2)
	{
		outV0->x = voxelPosition.x + v0.x;
		outV0->y = voxelPosition.y + (v0.y * ceilingScale);
		outV0->z = voxelPosition.z + v0.z;

		outV1->x = voxelPosition.x + v1.x;
		outV1->y = voxelPosition.y + (v1.y * ceilingScale);
		outV1->z = voxelPosition.z + v1.z;

		outV2->x = voxelPosition.x + v2.x;
		outV2->y = voxelPosition.y + (v2.y * ceilingScale);
		outV2->z = voxelPosition.z + v2.z;
	}
}

namespace sgMesh
{
	constexpr int MAX_VERTICES_PER_VOXEL = 8;
	constexpr int MAX_INDICES_PER_VOXEL = 36;
	constexpr int INDICES_PER_TRIANGLE = 3;
	constexpr int COMPONENTS_PER_VERTEX = 3; // XYZ
	constexpr int ATTRIBUTES_PER_VERTEX = 2; // XY texture coordinates

	constexpr int GetVoxelVertexCount(ArenaTypes::VoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaTypes::VoxelType::None:
			return 0;
		case ArenaTypes::VoxelType::Wall:
		case ArenaTypes::VoxelType::Raised:
		case ArenaTypes::VoxelType::TransparentWall:
		case ArenaTypes::VoxelType::Chasm:
		case ArenaTypes::VoxelType::Door:
			return 8;
		case ArenaTypes::VoxelType::Floor:
		case ArenaTypes::VoxelType::Ceiling:
		case ArenaTypes::VoxelType::Diagonal:
		case ArenaTypes::VoxelType::Edge:
			return 4;
		default:
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)));
		}
	}

	constexpr int GetVoxelOpaqueIndexCount(ArenaTypes::VoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaTypes::VoxelType::None:
		case ArenaTypes::VoxelType::TransparentWall:
		case ArenaTypes::VoxelType::Door:
		case ArenaTypes::VoxelType::Edge:
			return 0;
		case ArenaTypes::VoxelType::Wall:
			return 12 * INDICES_PER_TRIANGLE;
		case ArenaTypes::VoxelType::Raised:
			return 4 * INDICES_PER_TRIANGLE;
		case ArenaTypes::VoxelType::Chasm:
		case ArenaTypes::VoxelType::Floor:
		case ArenaTypes::VoxelType::Ceiling:
		case ArenaTypes::VoxelType::Diagonal:
			return 2 * INDICES_PER_TRIANGLE;
		default:
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)));
		}
	}

	constexpr int GetVoxelAlphaTestedIndexCount(ArenaTypes::VoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaTypes::VoxelType::None:
		case ArenaTypes::VoxelType::Wall:
		case ArenaTypes::VoxelType::Floor:
		case ArenaTypes::VoxelType::Ceiling:
		case ArenaTypes::VoxelType::Diagonal:
			return 0;
		case ArenaTypes::VoxelType::Raised:
		case ArenaTypes::VoxelType::TransparentWall:
		case ArenaTypes::VoxelType::Chasm:
		case ArenaTypes::VoxelType::Door:
			return 12 * INDICES_PER_TRIANGLE;
		case ArenaTypes::VoxelType::Edge:
			return 2 * INDICES_PER_TRIANGLE;
		default:
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)));
		}
	}

	void WriteWallMeshBuffers(const VoxelDefinition::WallData &wall, BufferView<double> outVertices,
		BufferView<double> outAttributes, BufferView<int32_t> outOpaqueIndices)
	{
		constexpr std::array<double, GetVoxelVertexCount(ArenaTypes::VoxelType::Wall) * 3> vertices =
		{
			// X=0

			// X=1

			// Y=0

			// Y=1

			// Z=0

			// Z=1

		};

		// @todo
		DebugNotImplemented();
	}

	void WriteFloorMeshBuffers(const VoxelDefinition::FloorData &floor, BufferView<double> outVertices,
		BufferView<double> outAttributes, BufferView<int32_t> outOpaqueIndices)
	{
		constexpr std::array<double, GetVoxelVertexCount(ArenaTypes::VoxelType::Floor) * 3> vertices =
		{
			// X=0

			// X=1

			// Z=0

			// Z=1

		};

		// @todo
		DebugNotImplemented();
	}

	void WriteCeilingMeshBuffers(const VoxelDefinition::CeilingData &ceiling, BufferView<double> outVertices,
		BufferView<double> outAttributes, BufferView<int32_t> outOpaqueIndices)
	{
		constexpr std::array<double, GetVoxelVertexCount(ArenaTypes::VoxelType::Ceiling) * 3> vertices =
		{
			// X=0

			// X=1

			// Z=0

			// Z=1

		};

		// @todo
		DebugNotImplemented();
	}

	void WriteRaisedMeshBuffers(const VoxelDefinition::RaisedData &raised, BufferView<double> outVertices,
		BufferView<double> outAttributes, BufferView<int32_t> outOpaqueIndices, BufferView<int32_t> outAlphaTestedIndices)
	{
		constexpr std::array<double, GetVoxelVertexCount(ArenaTypes::VoxelType::Raised) * 3> vertices =
		{
			// X=0

			// X=1

			// Y=0

			// Y=1

			// Z=0

			// Z=1

		};

		// @todo
		DebugNotImplemented();
	}

	void WriteDiagonalMeshBuffers(const VoxelDefinition::DiagonalData &diagonal, BufferView<double> outVertices,
		BufferView<double> outAttributes, BufferView<int32_t> outOpaqueIndices)
	{
		constexpr std::array<double, GetVoxelVertexCount(ArenaTypes::VoxelType::Diagonal) * 3> vertices =
		{
			// X=0

			// X=1

			// Z=0

			// Z=1

		};

		// @todo
		DebugNotImplemented();
	}

	void WriteTransparentWallMeshBuffers(const VoxelDefinition::TransparentWallData &transparentWall,
		BufferView<double> outVertices, BufferView<double> outAttributes, BufferView<int32_t> outAlphaTestedIndices)
	{
		constexpr std::array<double, GetVoxelVertexCount(ArenaTypes::VoxelType::TransparentWall) * 3> vertices =
		{
			// X=0

			// X=1

			// Y=0

			// Y=1

			// Z=0

			// Z=1

		};

		// @todo
		DebugNotImplemented();
	}

	void WriteEdgeMeshBuffers(const VoxelDefinition::EdgeData &edge, BufferView<double> outVertices,
		BufferView<double> outAttributes, BufferView<int32_t> outAlphaTestedIndices)
	{
		// @todo: four different vertex buffers depending on the side? The vertical size is always the same.
		constexpr std::array<double, GetVoxelVertexCount(ArenaTypes::VoxelType::Edge) * 3> vertices =
		{
			// X=0

			// X=1

			// Y=0

			// Y=1

			// Z=0

			// Z=1

		};

		// @todo
		DebugNotImplemented();
	}

	void WriteChasmMeshBuffers(const VoxelDefinition::ChasmData &chasm, BufferView<double> outVertices,
		BufferView<double> outAttributes, BufferView<int32_t> outOpaqueIndices, BufferView<int32_t> outAlphaTestedIndices)
	{
		constexpr std::array<double, GetVoxelVertexCount(ArenaTypes::VoxelType::Chasm) * 3> vertices =
		{
			// X=0

			// X=1

			// Y=0

			// Y=1

			// Z=0

			// Z=1

		};

		// @todo
		DebugNotImplemented();
	}

	void WriteDoorMeshBuffers(const VoxelDefinition::DoorData &door, BufferView<double> outVertices,
		BufferView<double> outAttributes, BufferView<int32_t> outAlphaTestedIndices)
	{
		constexpr std::array<double, GetVoxelVertexCount(ArenaTypes::VoxelType::Door) * 3> vertices =
		{
			// X=0

			// X=1

			// Y=0

			// Y=1

			// Z=0

			// Z=1

		};

		// @todo
		DebugNotImplemented();
	}

	void WriteVoxelMeshBuffers(const VoxelDefinition &voxelDef, BufferView<double> outVertices,
		BufferView<double> outAttributes, BufferView<int32_t> outOpaqueIndices,
		BufferView<int32_t> outAlphaTestedIndices)
	{
		const ArenaTypes::VoxelType voxelType = voxelDef.type;
		switch (voxelType)
		{
		case ArenaTypes::VoxelType::Wall:
			WriteWallMeshBuffers(voxelDef.wall, outVertices, outAttributes, outOpaqueIndices);
			break;
		case ArenaTypes::VoxelType::Floor:
			WriteFloorMeshBuffers(voxelDef.floor, outVertices, outAttributes, outOpaqueIndices);
			break;
		case ArenaTypes::VoxelType::Ceiling:
			WriteCeilingMeshBuffers(voxelDef.ceiling, outVertices, outAttributes, outOpaqueIndices);
			break;
		case ArenaTypes::VoxelType::Raised:
			WriteRaisedMeshBuffers(voxelDef.raised, outVertices, outAttributes, outOpaqueIndices, outAlphaTestedIndices);
			break;
		case ArenaTypes::VoxelType::Diagonal:
			WriteDiagonalMeshBuffers(voxelDef.diagonal, outVertices, outAttributes, outOpaqueIndices);
			break;
		case ArenaTypes::VoxelType::TransparentWall:
			WriteTransparentWallMeshBuffers(voxelDef.transparentWall, outVertices, outAttributes, outAlphaTestedIndices);
			break;
		case ArenaTypes::VoxelType::Edge:
			WriteEdgeMeshBuffers(voxelDef.edge, outVertices, outAttributes, outAlphaTestedIndices);
			break;
		case ArenaTypes::VoxelType::Chasm:
			WriteChasmMeshBuffers(voxelDef.chasm, outVertices, outAttributes, outOpaqueIndices, outAlphaTestedIndices);
			break;
		case ArenaTypes::VoxelType::Door:
			WriteDoorMeshBuffers(voxelDef.door, outVertices, outAttributes, outAlphaTestedIndices);
			break;
		default:
			DebugNotImplementedMsg(std::to_string(static_cast<int>(voxelType)));
		}
	}
}

namespace sgTexture
{
	// Loads the given voxel definition's textures into the voxel textures list if they haven't been loaded yet.
	void LoadVoxelDefTextures(const VoxelDefinition &voxelDef, std::vector<SceneGraph::LoadedVoxelTexture> &voxelTextures,
		TextureManager &textureManager, Renderer &renderer)
	{
		for (int i = 0; i < voxelDef.getTextureAssetCount(); i++)
		{
			const TextureAsset &textureAsset = voxelDef.getTextureAsset(i);
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

	// Loads the chasm floor materials for the given chasm. This should be done before loading the chasm wall
	// as each wall material has a dependency on the floor texture.
	void LoadChasmFloorTextures(ArenaTypes::ChasmType chasmType, std::vector<SceneGraph::LoadedChasmTextureList> &chasmTextureLists,
		TextureManager &textureManager, Renderer &renderer)
	{
		const auto cacheIter = std::find_if(chasmTextureLists.begin(), chasmTextureLists.end(),
			[chasmType](const SceneGraph::LoadedChasmTextureList &loadedTextureList)
		{
			return loadedTextureList.chasmType == chasmType;
		});

		DebugAssertMsg(cacheIter == chasmTextureLists.end(), "Already loaded chasm floor textures for type \"" +
			std::to_string(static_cast<int>(chasmType)) + "\".");

		const bool hasTexturedAnim = chasmType != ArenaTypes::ChasmType::Dry;
		if (hasTexturedAnim)
		{
			const std::string chasmFilename = [chasmType]()
			{
				if (chasmType == ArenaTypes::ChasmType::Wet)
				{
					return ArenaRenderUtils::CHASM_WATER_FILENAME;
				}
				else if (chasmType == ArenaTypes::ChasmType::Lava)
				{
					return ArenaRenderUtils::CHASM_LAVA_FILENAME;
				}
				else
				{
					DebugUnhandledReturnMsg(std::string, std::to_string(static_cast<int>(chasmType)));
				}
			}();

			SceneGraph::LoadedChasmTextureList newTextureList;
			newTextureList.init(chasmType);

			const Buffer<TextureAsset> textureAssets = TextureUtils::makeTextureAssets(chasmFilename, textureManager);
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

				// Populate chasmTextureRefs, leave wall entries empty since they're populated next.
				newTextureList.chasmTextureRefs.emplace_back(std::move(chasmTextureRef));
			}

			chasmTextureLists.emplace_back(std::move(newTextureList));
		}
		else
		{
			// Dry chasms are just a single color, no texture asset available.
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

			DebugAssert(!lockedTexture.isTrueColor);
			uint8_t *texels = static_cast<uint8_t*>(lockedTexture.texels);
			*texels = ArenaRenderUtils::PALETTE_INDEX_DRY_CHASM_COLOR;
			renderer.unlockObjectTexture(dryChasmTextureID);

			SceneGraph::LoadedChasmTextureList newTextureList;
			newTextureList.init(chasmType);

			// Populate chasmTextureRefs, leave wall entries empty since they're populated next.
			newTextureList.chasmTextureRefs.emplace_back(std::move(dryChasmTextureRef));
			chasmTextureLists.emplace_back(std::move(newTextureList));
		}
	}

	// Loads the chasm wall material for the given chasm and texture asset. This expects the chasm floor material to
	// already be loaded and available for sharing.
	void LoadChasmWallTextures(ArenaTypes::ChasmType chasmType, const TextureAsset &textureAsset,
		std::vector<SceneGraph::LoadedChasmTextureList> &chasmTextureLists, TextureManager &textureManager, Renderer &renderer)
	{
		const auto listIter = std::find_if(chasmTextureLists.begin(), chasmTextureLists.end(),
			[chasmType](const SceneGraph::LoadedChasmTextureList &loadedTextureList)
		{
			return loadedTextureList.chasmType == chasmType;
		});

		DebugAssertMsg(listIter != chasmTextureLists.end(), "Expected loaded chasm floor texture list for type \"" +
			std::to_string(static_cast<int>(chasmType)) + "\".");

		std::vector<SceneGraph::LoadedChasmTextureList::WallEntry> &wallEntries = listIter->wallEntries;
		const auto entryIter = std::find_if(wallEntries.begin(), wallEntries.end(),
			[&textureAsset](const SceneGraph::LoadedChasmTextureList::WallEntry &wallEntry)
		{
			return wallEntry.wallTextureAsset == textureAsset;
		});

		if (entryIter == wallEntries.end())
		{
			const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAsset);
			if (!textureBuilderID.has_value())
			{
				DebugLogWarning("Couldn't load chasm wall texture \"" + textureAsset.filename + "\".");
				return;
			}

			const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
			ObjectTextureID wallTextureID;
			if (!renderer.tryCreateObjectTexture(textureBuilder, &wallTextureID))
			{
				DebugLogWarning("Couldn't create chasm wall texture \"" + textureAsset.filename + "\".");
				return;
			}

			SceneGraph::LoadedChasmTextureList::WallEntry newWallEntry;
			newWallEntry.wallTextureAsset = textureAsset;
			newWallEntry.wallTextureRef.init(wallTextureID, renderer);
			wallEntries.emplace_back(std::move(newWallEntry));
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

void SceneGraph::LoadedChasmTextureList::init(ArenaTypes::ChasmType chasmType)
{
	this->chasmType = chasmType;
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

ObjectTextureID SceneGraph::getChasmFloorTextureID(ArenaTypes::ChasmType chasmType, double chasmAnimPercent) const
{
	const auto iter = std::find_if(this->chasmTextureLists.begin(), this->chasmTextureLists.end(),
		[chasmType](const LoadedChasmTextureList &loadedTextureList)
	{
		return loadedTextureList.chasmType == chasmType;
	});

	DebugAssertMsg(iter != this->chasmTextureLists.end(), "No loaded chasm floor texture for type \"" +
		std::to_string(static_cast<int>(chasmType)) + "\".");
	const std::vector<ScopedObjectTextureRef> &floorTextureRefs = iter->chasmTextureRefs;
	const int textureCount = static_cast<int>(floorTextureRefs.size());
	const int index = std::clamp(static_cast<int>(static_cast<double>(textureCount) * chasmAnimPercent), 0, textureCount - 1);
	DebugAssertIndex(floorTextureRefs, index);
	const ScopedObjectTextureRef &floorTextureRef = floorTextureRefs[index];
	return floorTextureRef.get();
}

ObjectTextureID SceneGraph::getChasmWallTextureID(ArenaTypes::ChasmType chasmType, const TextureAsset &textureAsset) const
{
	const auto listIter = std::find_if(this->chasmTextureLists.begin(), this->chasmTextureLists.end(),
		[chasmType, &textureAsset](const LoadedChasmTextureList &loadedTextureList)
	{
		return loadedTextureList.chasmType == chasmType;
	});

	DebugAssertMsg(listIter != this->chasmTextureLists.end(), "No loaded chasm floor texture for type \"" +
		std::to_string(static_cast<int>(chasmType)) + "\".");

	const std::vector<LoadedChasmTextureList::WallEntry> &entries = listIter->wallEntries;
	const auto entryIter = std::find_if(entries.begin(), entries.end(),
		[&textureAsset](const LoadedChasmTextureList::WallEntry &entry)
	{
		return entry.wallTextureAsset == textureAsset;
	});

	DebugAssertMsg(entryIter != entries.end(), "No loaded chasm wall texture for type \"" +
		std::to_string(static_cast<int>(chasmType)) + "\" and texture \"" + textureAsset.filename + "\".");

	const ScopedObjectTextureRef &wallTextureRef = entryIter->wallTextureRef;
	return wallTextureRef.get();
}

BufferView<const RenderDrawCall> SceneGraph::getDrawCalls() const
{
	return BufferView<const RenderDrawCall>(this->drawCalls.data(), static_cast<int>(this->drawCalls.size()));
}

void SceneGraph::loadTextures(const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
	const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, TextureManager &textureManager,
	Renderer &renderer)
{
	// Load chasm floor textures, independent of voxels in the level. Do this before chasm wall texture loading
	// because walls are multi-textured and depend on the chasm animation textures.
	sgTexture::LoadChasmFloorTextures(ArenaTypes::ChasmType::Dry, this->chasmTextureLists, textureManager, renderer);
	sgTexture::LoadChasmFloorTextures(ArenaTypes::ChasmType::Wet, this->chasmTextureLists, textureManager, renderer);
	sgTexture::LoadChasmFloorTextures(ArenaTypes::ChasmType::Lava, this->chasmTextureLists, textureManager, renderer);

	// Load textures known at level load time. Note that none of the object texture IDs allocated here are
	// matched with voxel/entity instances until the chunks containing them are created.
	auto loadLevelDefTextures = [this, &mapDefinition, &textureManager, &renderer](int levelIndex)
	{
		const LevelInfoDefinition &levelInfoDef = mapDefinition.getLevelInfoForLevel(levelIndex);

		for (int i = 0; i < levelInfoDef.getVoxelDefCount(); i++)
		{
			const VoxelDefinition &voxelDef = levelInfoDef.getVoxelDef(i);
			sgTexture::LoadVoxelDefTextures(voxelDef, this->voxelTextures, textureManager, renderer);

			if (voxelDef.type == ArenaTypes::VoxelType::Chasm)
			{
				const VoxelDefinition::ChasmData &chasm = voxelDef.chasm;
				sgTexture::LoadChasmWallTextures(chasm.type, chasm.textureAsset, this->chasmTextureLists, textureManager, renderer);
			}
		}

		for (int i = 0; i < levelInfoDef.getEntityDefCount(); i++)
		{
			const EntityDefinition &entityDef = levelInfoDef.getEntityDef(i);
			sgTexture::LoadEntityDefTextures(entityDef, this->entityTextures, textureManager, renderer);
		}
	};

	const MapType mapType = mapDefinition.getMapType();
	if ((mapType == MapType::Interior) || (mapType == MapType::City))
	{
		// Load textures for the active level.
		DebugAssert(activeLevelIndex.has_value());
		loadLevelDefTextures(*activeLevelIndex);
	}
	else if (mapType == MapType::Wilderness)
	{
		// Load textures for all wilderness chunks.
		for (int i = 0; i < mapDefinition.getLevelCount(); i++)
		{
			loadLevelDefTextures(i);
		}
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(mapType)));
	}

	// Load citizen textures if citizens can exist in the level.
	if ((mapType == MapType::City) || (mapType == MapType::Wilderness))
	{
		DebugAssert(citizenGenInfo.has_value());
		const EntityDefinition &maleEntityDef = *citizenGenInfo->maleEntityDef;
		const EntityDefinition &femaleEntityDef = *citizenGenInfo->femaleEntityDef;
		sgTexture::LoadEntityDefTextures(maleEntityDef, this->entityTextures, textureManager, renderer);
		sgTexture::LoadEntityDefTextures(femaleEntityDef, this->entityTextures, textureManager, renderer);
	}
}

void SceneGraph::loadVoxels(const LevelInstance &levelInst, const RenderCamera &camera, double chasmAnimPercent,
	bool nightLightsAreActive, RendererSystem3D &renderer)
{
	// Expect empty chunks to have been created just now (it's done before this in the edge case
	// there are no voxels at all since entities rely on chunks existing).
	DebugAssert(!this->graphChunks.empty());

	const ChunkManager &chunkManager = levelInst.getChunkManager();
	for (int chunkIndex = 0; chunkIndex < chunkManager.getChunkCount(); chunkIndex++)
	{
		const Chunk &chunk = chunkManager.getChunk(chunkIndex);
		SceneGraphChunk &graphChunk = this->graphChunks[chunkIndex];

		// Add voxel definitions to the scene graph.
		for (int voxelDefIndex = 0; voxelDefIndex < chunk.getVoxelDefCount(); voxelDefIndex++)
		{
			const Chunk::VoxelID voxelID = static_cast<Chunk::VoxelID>(voxelDefIndex);
			const VoxelDefinition &voxelDef = chunk.getVoxelDef(voxelID);
			const ArenaTypes::VoxelType voxelType = voxelDef.type;

			SceneGraphVoxelDefinition graphVoxelDef;
			if (voxelType != ArenaTypes::VoxelType::None) // Only attempt to create buffers for non-air voxels.
			{
				const int vertexCount = sgMesh::GetVoxelVertexCount(voxelType);
				if (!renderer.tryCreateVertexBuffer(vertexCount, sgMesh::COMPONENTS_PER_VERTEX, &graphVoxelDef.vertexBufferID))
				{
					DebugLogError("Couldn't create vertex buffer for voxel ID " + std::to_string(voxelID) +
						" in chunk (" + chunk.getPosition().toString() + ").");
					continue;
				}

				if (!renderer.tryCreateAttributeBuffer(vertexCount, sgMesh::COMPONENTS_PER_VERTEX, &graphVoxelDef.attributeBufferID))
				{
					DebugLogError("Couldn't create attribute buffer for voxel ID " + std::to_string(voxelID) +
						" in chunk (" + chunk.getPosition().toString() + ").");
					graphVoxelDef.freeBuffers(renderer);
					continue;
				}

				std::array<double, sgMesh::MAX_VERTICES_PER_VOXEL * sgMesh::COMPONENTS_PER_VERTEX> vertices;
				std::array<double, sgMesh::MAX_VERTICES_PER_VOXEL * sgMesh::ATTRIBUTES_PER_VERTEX> attributes;
				std::array<int32_t, sgMesh::MAX_INDICES_PER_VOXEL> opaqueIndices, alphaTestedIndices;
				vertices.fill(0.0);
				attributes.fill(0.0);
				opaqueIndices.fill(0);
				alphaTestedIndices.fill(0);

				// Generate mesh data for this voxel definition.
				sgMesh::WriteVoxelMeshBuffers(voxelDef,
					BufferView<double>(vertices.data(), static_cast<int>(vertices.size())),
					BufferView<double>(attributes.data(), static_cast<int>(attributes.size())),
					BufferView<int32_t>(opaqueIndices.data(), static_cast<int>(opaqueIndices.size())),
					BufferView<int32_t>(alphaTestedIndices.data(), static_cast<int>(alphaTestedIndices.size())));

				renderer.populateVertexBuffer(graphVoxelDef.vertexBufferID, BufferView<const double>(vertices.data(), vertexCount));
				renderer.populateAttributeBuffer(graphVoxelDef.attributeBufferID, BufferView<const double>(attributes.data(), vertexCount));

				const int opaqueIndexCount = sgMesh::GetVoxelOpaqueIndexCount(voxelType);
				if (opaqueIndexCount > 0)
				{
					if (!renderer.tryCreateIndexBuffer(opaqueIndexCount, &graphVoxelDef.opaqueIndexBufferID))
					{
						DebugLogError("Couldn't create opaque index buffer for voxel ID " + std::to_string(voxelID) +
							" in chunk (" + chunk.getPosition().toString() + ").");
						graphVoxelDef.freeBuffers(renderer);
						continue;
					}

					renderer.populateIndexBuffer(graphVoxelDef.opaqueIndexBufferID,
						BufferView<const int32_t>(opaqueIndices.data(), opaqueIndexCount));
				}

				const int alphaTestedIndexCount = sgMesh::GetVoxelAlphaTestedIndexCount(voxelType);
				if (alphaTestedIndexCount > 0)
				{
					if (!renderer.tryCreateIndexBuffer(alphaTestedIndexCount, &graphVoxelDef.alphaTestedIndexBufferID))
					{
						DebugLogError("Couldn't create alpha-tested index buffer for voxel ID " + std::to_string(voxelID) +
							" in chunk (" + chunk.getPosition().toString() + ").");
						graphVoxelDef.freeBuffers(renderer);
						continue;
					}

					renderer.populateIndexBuffer(graphVoxelDef.alphaTestedIndexBufferID,
						BufferView<const int32_t>(alphaTestedIndices.data(), alphaTestedIndexCount));
				}
			}

			graphChunk.voxelDefs.emplace_back(std::move(graphVoxelDef));
		}

		// Assign voxel definition indices for each graph voxel.
		for (WEInt z = 0; z < Chunk::DEPTH; z++)
		{
			for (int y = 0; y < chunk.getHeight(); y++)
			{
				for (SNInt x = 0; x < Chunk::WIDTH; x++)
				{
					// Get the voxel def mapping's index and use it for this voxel.
					const Chunk::VoxelID voxelID = chunk.getVoxel(x, y, z);
					const auto defIter = graphChunk.voxelDefMappings.find(voxelID);
					DebugAssert(defIter != graphChunk.voxelDefMappings.end());
					graphChunk.voxels.set(x, y, z, defIter->second);
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
	DebugAssert(this->drawCalls.empty());

	// Create empty graph chunks using the chunk manager's chunks as a reference.
	const ChunkManager &chunkManager = levelInst.getChunkManager();
	for (int i = 0; i < chunkManager.getChunkCount(); i++)
	{
		const Chunk &chunk = chunkManager.getChunk(i);
		SceneGraphChunk graphChunk;
		graphChunk.init(chunk.getPosition(), chunk.getHeight());
		this->graphChunks.emplace_back(std::move(graphChunk));
	}

	// @todo: load textures somewhere in here and in a way that their draw calls can be generated; maybe want to store
	// TextureAssets with SceneGraphVoxelDefinition? Might want textures to be ref-counted if reused between chunks.

	const double ceilingScale = levelInst.getCeilingScale();
	this->loadTextures(activeLevelIndex, mapDefinition, citizenGenInfo, textureManager, renderer);

	SceneGraphChunk &graphChunk = this->graphChunks.at(0);
	const Buffer3D<SceneGraphVoxelID> &graphChunkVoxels = graphChunk.voxels;
	for (WEInt z = 0; z < graphChunkVoxels.getDepth(); z++)
	{
		for (int y = 0; y < graphChunkVoxels.getHeight(); y++)
		{
			for (SNInt x = 0; x < graphChunkVoxels.getWidth(); x++)
			{
				SceneGraphVoxelDefinition graphVoxelDef;

				const int vertexCount = 3;
				if (!renderer3D.tryCreateVertexBuffer(vertexCount, sgMesh::COMPONENTS_PER_VERTEX, &graphVoxelDef.vertexBufferID))
				{
					DebugLogError("Couldn't create vertex buffer.");
					return;
				}

				if (!renderer3D.tryCreateAttributeBuffer(vertexCount, sgMesh::ATTRIBUTES_PER_VERTEX, &graphVoxelDef.attributeBufferID))
				{
					DebugLogError("Couldn't create attribute buffer.");
					return;
				}

				const int indexCount = 3;
				if (!renderer3D.tryCreateIndexBuffer(indexCount, &graphVoxelDef.opaqueIndexBufferID))
				{
					DebugLogError("Couldn't create index buffer.");
					return;
				}

				const Double3 v0(
					static_cast<double>(x),
					static_cast<double>(y),
					static_cast<double>(z));
				const Double3 v1(
					v0.x,
					v0.y + ceilingScale,
					v0.z);
				const Double3 v2(
					v0.x,
					v0.y,
					v0.z + 1.0);

				const double v[9] =
				{
					v0.x, v0.y, v0.z,
					v1.x, v1.y, v1.z,
					v2.x, v2.y, v2.z
				};

				const BufferView<const double> vView(v, static_cast<int>(std::size(v)));
				renderer3D.populateVertexBuffer(graphVoxelDef.vertexBufferID, vView);

				constexpr double a[6] =
				{
					0.0, 1.0, // Bottom left
					0.0, 0.0, // Top left
					1.0, 1.0  // Bottom right
				};

				const BufferView<const double> aView(a, static_cast<int>(std::size(a)));
				renderer3D.populateAttributeBuffer(graphVoxelDef.attributeBufferID, aView);

				const int32_t i[3] = { 1, 0, 2 }; // Counter-clockwise
				const BufferView<const int32_t> iView(i, static_cast<int>(std::size(i)));
				renderer3D.populateIndexBuffer(graphVoxelDef.opaqueIndexBufferID, iView);

				const SceneGraphVoxelID graphVoxelID = graphChunk.addVoxelDef(std::move(graphVoxelDef));
				graphChunk.voxels.set(x, y, z, graphVoxelID);

				RenderDrawCall drawCall;
				drawCall.vertexBufferID = graphVoxelDef.vertexBufferID;
				drawCall.attributeBufferID = graphVoxelDef.attributeBufferID;
				drawCall.indexBufferID = graphVoxelDef.opaqueIndexBufferID;
				drawCall.textureIDs[0] = 5;
				drawCall.vertexShaderType = VertexShaderType::Default;
				drawCall.pixelShaderType = PixelShaderType::Opaque;

				this->drawCalls.emplace_back(std::move(drawCall));
			}
		}
	}

	//this->loadVoxels(levelInst, camera, chasmAnimPercent, nightLightsAreActive, renderer3D);

	/*this->loadEntities(levelInst, camera, entityDefLibrary, nightLightsAreActive, playerHasLight, renderer3D);
	this->loadSky(skyInst, daytimePercent, latitude, renderer3D);
	this->loadWeather(skyInst, daytimePercent, renderer3D);*/

	// @todo: populate draw calls since update() only operates on dirty stuff from chunk manager/entity manager/etc.
}

void SceneGraph::unloadScene(RendererSystem3D &renderer)
{
	// Free vertex/attribute/index buffer IDs from renderer.
	for (SceneGraphChunk &chunk : this->graphChunks)
	{
		chunk.freeBuffers(renderer);
	}

	this->graphChunks.clear();
	this->drawCalls.clear();
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
