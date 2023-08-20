#include "SceneManager.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/ExeData.h"
#include "../Assets/TextureManager.h"
#include "../Game/ArenaClockUtils.h"
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

void SceneManager::updateGameWorldPalette(bool isInterior, WeatherType weatherType, bool isFoggy, double daytimePercent, TextureManager &textureManager)
{
	constexpr int paletteLength = 256;

	// Update sky gradient. Write to palette indices 1-8 using one of the three palettes.
	const std::string *skyGradientFilename = &ArenaPaletteName::Default;
	std::optional<int> daytimePaletteIndexOffset;
	if (!isInterior)
	{
		skyGradientFilename = (weatherType == WeatherType::Clear) ? &ArenaPaletteName::Daytime : &ArenaPaletteName::Dreary;

		const double daytimePercent6AM = 0.25;
		const double daytimePercent6PM = 0.75;
		const double skyGradientDaytimePercent = (daytimePercent - daytimePercent6AM) / (daytimePercent6PM - daytimePercent6AM);
		daytimePaletteIndexOffset = std::clamp(static_cast<int>(skyGradientDaytimePercent * static_cast<double>(paletteLength)), 0, paletteLength - 1);
	}

	const std::optional<PaletteID> skyGradientPaletteID = textureManager.tryGetPaletteID(skyGradientFilename->c_str());
	if (!skyGradientPaletteID.has_value())
	{
		DebugLogError("Couldn't get palette ID for sky gradient \"" + (*skyGradientFilename) + "\".");
		return;
	}

	const Palette &skyGradientPalette = textureManager.getPaletteHandle(*skyGradientPaletteID);
	const BufferView<const Color> skyGradientPaletteTexels(skyGradientPalette);
	LockedTexture lockedTexture = this->gameWorldPaletteTextureRef.lockTexels();
	if (!lockedTexture.isValid())
	{
		DebugLogError("Couldn't lock sky gradient texture \"" + (*skyGradientFilename) + "\" for updating.");
		return;
	}

	DebugAssert(lockedTexture.bytesPerTexel == 4);
	DebugAssert((this->gameWorldPaletteTextureRef.getWidth() * this->gameWorldPaletteTextureRef.getHeight()) == skyGradientPaletteTexels.getCount());

	int srcTexelsIndexStart = daytimePaletteIndexOffset.has_value() ? *daytimePaletteIndexOffset : 1;
	const int skyGradientColorCount = static_cast<int>(std::size(ArenaRenderUtils::PALETTE_INDICES_SKY_COLOR));
	BufferView<uint32_t> gameWorldTexels(reinterpret_cast<uint32_t*>(lockedTexture.texels), paletteLength);
	BufferView<uint32_t> gameWorldSkyGradientTexels(gameWorldTexels.begin() + 1, skyGradientColorCount);
	for (int i = 0; i < skyGradientColorCount; i++)
	{
		const int srcIndex = (srcTexelsIndexStart + i) % paletteLength;
		gameWorldSkyGradientTexels[i] = skyGradientPaletteTexels[srcIndex].toARGB();
	}

	// Update window color in the palette.
	uint32_t windowColor;
	if (isInterior)
	{
		windowColor = gameWorldTexels[0]; // Black by default.
	}
	else if (isFoggy)
	{
		windowColor = gameWorldTexels[ArenaRenderUtils::PALETTE_INDEX_SKY_COLOR_FOG];
	}
	else
	{
		// Use transition colors if during sunrise/sunset.
		const double startBrighteningPercent = ArenaClockUtils::AmbientStartBrightening.getDaytimePercent();
		const double endBrighteningPercent = ArenaClockUtils::AmbientEndBrightening.getDaytimePercent();
		const double startDimmingPercent = ArenaClockUtils::AmbientStartDimming.getDaytimePercent();
		const double endDimmingPercent = ArenaClockUtils::AmbientEndDimming.getDaytimePercent();
		const bool isDuringSunrise = (daytimePercent >= startBrighteningPercent) && (daytimePercent < endBrighteningPercent);
		const bool isDuringSunset = (daytimePercent >= startDimmingPercent) && (daytimePercent < endDimmingPercent);
		const bool isDuringNight = (daytimePercent >= endDimmingPercent) || (daytimePercent < startBrighteningPercent);

		const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
		const ExeData &exeData = binaryAssetLibrary.getExeData();
		const BufferView<const uint8_t> windowColorBytes = exeData.light.windowTwilightColors;

		auto getWindowRGBForTransitionPercent = [&windowColorBytes](double percent)
		{
			constexpr int bytesPerWindowColor = 3;
			const int totalWindowColors = windowColorBytes.getCount() / bytesPerWindowColor;
			const int windowColorIndex = std::clamp(static_cast<int>(static_cast<double>(totalWindowColors) * percent), 0, totalWindowColors - 1);
			const int windowColorByteOffset = windowColorIndex * bytesPerWindowColor;

			// Convert 6-bit RGB bytes to 32-bit integer.
			constexpr int componentMultiplier = 4;
			const uint8_t windowR = windowColorBytes[windowColorByteOffset];
			const uint8_t windowG = windowColorBytes[windowColorByteOffset + 1];
			const uint8_t windowB = windowColorBytes[windowColorByteOffset + 2];
			const uint8_t windowRCorrected = windowR * componentMultiplier;
			const uint8_t windowGCorrected = windowG * componentMultiplier;
			const uint8_t windowBCorrected = windowB * componentMultiplier;
			const uint32_t windowRGBCorrected = (windowRCorrected << 16) | (windowGCorrected << 8) | windowBCorrected;
			return windowRGBCorrected;
		};

		if (isDuringSunrise)
		{
			const double transitionPercent = std::clamp((daytimePercent - startBrighteningPercent) / (endBrighteningPercent - startBrighteningPercent), 0.0, 1.0);
			windowColor = getWindowRGBForTransitionPercent(transitionPercent);
		}
		else if (isDuringSunset)
		{
			const double transitionPercent = std::clamp(1.0 - ((daytimePercent - startDimmingPercent) / (endDimmingPercent - startDimmingPercent)), 0.0, 1.0);
			windowColor = getWindowRGBForTransitionPercent(transitionPercent);
		}
		else if (isDuringNight)
		{
			windowColor = getWindowRGBForTransitionPercent(0.0);
		}
		else
		{
			windowColor = gameWorldTexels[0]; // Black during the day.
		}
	}

	gameWorldTexels[ArenaRenderUtils::PALETTE_INDEX_WINDOW] = windowColor;

	this->gameWorldPaletteTextureRef.unlockTexels();
}

void SceneManager::cleanUp()
{
	this->chunkManager.cleanUp();
	this->voxelChunkManager.cleanUp();
	this->entityChunkManager.cleanUp();
	this->renderChunkManager.cleanUp();
}
