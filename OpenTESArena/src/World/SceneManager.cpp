#include "SceneManager.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Rendering/ArenaRenderUtils.h"

#include "components/debug/Debug.h"

SceneManager::SceneManager()
{
	
}

void SceneManager::init(TextureManager &textureManager, Renderer &renderer)
{
	ObjectTextureID gameWorldPaletteTextureID = ArenaLevelUtils::allocGameWorldPaletteTexture(ArenaPaletteName::Default, textureManager, renderer);
	this->gameWorldPaletteTextureRef.init(gameWorldPaletteTextureID, renderer);

	const ObjectTextureID normalLightTableDaytimeTextureID = ArenaLevelUtils::allocLightTableTexture(ArenaTextureName::NormalLightTable, textureManager, renderer);
	const ObjectTextureID normalLightTableNightTextureID = ArenaLevelUtils::allocLightTableTexture(ArenaTextureName::NormalLightTable, textureManager, renderer);
	const ObjectTextureID fogLightTableTextureID = ArenaLevelUtils::allocLightTableTexture(ArenaTextureName::FogLightTable, textureManager, renderer);
	this->normalLightTableDaytimeTextureRef.init(normalLightTableDaytimeTextureID, renderer);
	this->normalLightTableNightTextureRef.init(normalLightTableNightTextureID, renderer);
	this->fogLightTableTextureRef.init(fogLightTableTextureID, renderer);

	const int lightTableWidth = this->normalLightTableDaytimeTextureRef.getWidth();
	const int lightTableHeight = this->normalLightTableDaytimeTextureRef.getHeight();
	DebugAssert(this->normalLightTableNightTextureRef.getWidth() == lightTableWidth);
	DebugAssert(this->normalLightTableNightTextureRef.getHeight() == lightTableHeight);
	DebugAssert(this->fogLightTableTextureRef.getWidth() == lightTableWidth);
	DebugAssert(this->fogLightTableTextureRef.getHeight() == lightTableHeight);

	// For light tables active during night, fog, or in interiors, modify the last couple light levels to be
	// completely absent of light, including full brights.
	LockedTexture nightLockedTexture = this->normalLightTableNightTextureRef.lockTexels();
	LockedTexture fogLockedTexture = this->fogLightTableTextureRef.lockTexels();
	DebugAssert(nightLockedTexture.isValid());
	DebugAssert(fogLockedTexture.isValid());
	uint8_t *nightTexels = static_cast<uint8_t*>(nightLockedTexture.texels);
	uint8_t *fogTexels = static_cast<uint8_t*>(fogLockedTexture.texels);

	const int y = lightTableHeight - 1;
	for (int x = 0; x < lightTableWidth; x++)
	{
		const int dstIndex = x + (y * lightTableWidth);
		nightTexels[dstIndex] = ArenaRenderUtils::PALETTE_INDEX_DRY_CHASM_COLOR;
		fogTexels[dstIndex] = ArenaRenderUtils::PALETTE_INDEX_SKY_COLOR_FOG; // @todo: overwrite the dry chasm color in the palette (index 112) with fog when fog is active
	}

	this->normalLightTableNightTextureRef.unlockTexels();
	this->fogLightTableTextureRef.unlockTexels();
}

void SceneManager::cleanUp()
{
	this->chunkManager.cleanUp();
	this->voxelChunkManager.cleanUp();
	this->entityChunkManager.cleanUp();
}
