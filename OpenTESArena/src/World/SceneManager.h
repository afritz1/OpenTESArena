#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include "ChunkManager.h"
#include "../Collision/CollisionChunkManager.h"
#include "../Entities/EntityChunkManager.h"
#include "../Rendering/RenderChunkManager.h"
#include "../Rendering/RenderSkyManager.h"
#include "../Rendering/RenderTextureUtils.h"
#include "../Rendering/RenderWeatherManager.h"
#include "../Sky/SkyInstance.h"
#include "../Voxels/VoxelChunkManager.h"
#include "../Voxels/VoxelVisibilityChunkManager.h"

class TextureManager;
class Renderer;

enum class WeatherType;

struct SceneManager
{
	// Chunk managers for the active scene.
	ChunkManager chunkManager;
	VoxelChunkManager voxelChunkManager;
	EntityChunkManager entityChunkManager;
	CollisionChunkManager collisionChunkManager;
	VoxelVisibilityChunkManager voxelVisibilityChunkManager;
	RenderChunkManager renderChunkManager;

	// Game world systems not tied to chunks.
	SkyInstance skyInstance;
	RenderSkyManager renderSkyManager;
	RenderWeatherManager renderWeatherManager;

	ScopedObjectTextureRef gameWorldPaletteTextureRef;

	// Light tables; these might be switched between instantaneously depending on weather and time of day.
	ScopedObjectTextureRef normalLightTableDaytimeTextureRef;
	ScopedObjectTextureRef normalLightTableNightTextureRef;
	ScopedObjectTextureRef fogLightTableTextureRef;

	SceneManager();

	void init(TextureManager &textureManager, Renderer &renderer);
	void updateGameWorldPalette(bool isInterior, WeatherType weatherType, bool isFoggy, double daytimePercent, TextureManager &textureManager);
	void cleanUp();
};

struct SceneTransitionState
{
	MapType mapType;
	int levelIndex;
	int skyIndex;
};

struct SceneInteriorSavedState
{
	// Don't need to store levelIndex or skyIndex since the LEVELUP/DOWN voxel lets us infer it
};

struct SceneCitySavedState
{
	int weatherIndex;
	CoordInt3 returnCoord;
};

using SceneWildSavedState = SceneCitySavedState;

#endif
