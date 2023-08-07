#include "SceneManager.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"

#include "components/debug/Debug.h"

SceneManager::SceneManager()
{
	
}

void SceneManager::init(TextureManager &textureManager, Renderer &renderer)
{
	ObjectTextureID gameWorldPaletteTextureID = ArenaLevelUtils::allocGameWorldPaletteTexture(ArenaPaletteName::Default, textureManager, renderer);
	this->gameWorldPaletteTextureRef.init(gameWorldPaletteTextureID, renderer);

	const ObjectTextureID normalLightTableTextureID = ArenaLevelUtils::allocLightTableTexture(ArenaTextureName::NormalLightTable, textureManager, renderer);
	const ObjectTextureID fogLightTableTextureID = ArenaLevelUtils::allocLightTableTexture(ArenaTextureName::FogLightTable, textureManager, renderer);
	this->normalLightTableTextureRef.init(normalLightTableTextureID, renderer);
	this->fogLightTableTextureRef.init(fogLightTableTextureID, renderer);
}

void SceneManager::cleanUp()
{
	this->chunkManager.cleanUp();
	this->voxelChunkManager.cleanUp();
	this->entityChunkManager.cleanUp();
}
