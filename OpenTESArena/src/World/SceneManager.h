#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include "Jolt/Jolt.h"
#include "Jolt/Physics/PhysicsSystem.h"

#include "ChunkManager.h"
#include "../Collision/CollisionChunkManager.h"
#include "../Entities/EntityChunkManager.h"
#include "../Entities/EntityVisibilityChunkManager.h"
#include "../Rendering/RenderEntityChunkManager.h"
#include "../Rendering/RenderLightChunkManager.h"
#include "../Rendering/RenderSkyManager.h"
#include "../Rendering/RenderTextureUtils.h"
#include "../Rendering/RenderVoxelChunkManager.h"
#include "../Rendering/RenderWeatherManager.h"
#include "../Sky/SkyInstance.h"
#include "../Sky/SkyVisibilityManager.h"
#include "../Voxels/VoxelChunkManager.h"
#include "../Voxels/VoxelFaceCombineChunkManager.h"
#include "../Voxels/VoxelFaceEnableChunkManager.h"
#include "../Voxels/VoxelFrustumCullingChunkManager.h"

class TextureManager;
class Renderer;

enum class WeatherType;

struct SceneManager
{
	// Chunk managers for the active scene.
	ChunkManager chunkManager;
	VoxelChunkManager voxelChunkManager;
	EntityChunkManager entityChunkManager;
	VoxelFaceEnableChunkManager voxelFaceEnableChunkManager;
	VoxelFaceCombineChunkManager voxelFaceCombineChunkManager;
	CollisionChunkManager collisionChunkManager;
	VoxelFrustumCullingChunkManager voxelFrustumCullingChunkManager;
	EntityVisibilityChunkManager entityVisChunkManager;
	RenderVoxelChunkManager renderVoxelChunkManager;
	RenderEntityChunkManager renderEntityChunkManager;
	RenderLightChunkManager renderLightChunkManager;

	// Game world systems not tied to chunks.
	SkyInstance skyInstance;
	SkyVisibilityManager skyVisManager;
	RenderSkyManager renderSkyManager;
	RenderWeatherManager renderWeatherManager;

	ScopedObjectTextureRef gameWorldPaletteTextureRef;

	// Light tables; these might be switched between instantaneously depending on weather and time of day.
	ScopedObjectTextureRef normalLightTableDaytimeTextureRef;
	ScopedObjectTextureRef normalLightTableNightTextureRef;
	ScopedObjectTextureRef fogLightTableTextureRef;

	SceneManager();

	void init(TextureManager &textureManager, Renderer &renderer);
	void updateGameWorldPalette(bool isInterior, WeatherType weatherType, bool isFoggy, double dayPercent, TextureManager &textureManager);
	void endFrame(JPH::PhysicsSystem &physicsSystem, Renderer &renderer);
};

#endif
