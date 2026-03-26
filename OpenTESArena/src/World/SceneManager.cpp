#include "SceneManager.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/ExeData.h"
#include "../Assets/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/RendererUtils.h"
#include "../Time/ArenaClockUtils.h"
#include "../Time/ClockLibrary.h"

#include "components/debug/Debug.h"

SceneManager::SceneManager()
{
	this->gameWorldPaletteID = -1;
}

void SceneManager::init(TextureManager &textureManager, Renderer &renderer)
{
	this->gameWorldPaletteID = ArenaLevelUtils::getGameWorldPaletteID(ArenaPaletteName::Default, textureManager);
	ObjectTextureID gameWorldPaletteTextureID = ArenaLevelUtils::allocGameWorldPaletteTexture(this->gameWorldPaletteID, textureManager, renderer);
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
	uint8_t *nightTexels = nightLockedTexture.getTexels8().begin();
	uint8_t *fogTexels = fogLockedTexture.getTexels8().begin();

	const int y = lightTableHeight - 1;
	for (int x = 0; x < lightTableWidth; x++)
	{
		const int dstIndex = x + (y * lightTableWidth);
		nightTexels[dstIndex] = ArenaRenderUtils::PALETTE_INDEX_DRY_CHASM_COLOR;
		fogTexels[dstIndex] = ArenaRenderUtils::PALETTE_INDEX_SKY_COLOR_FOG; // @todo: overwrite the dry chasm color in the palette (index 112) with fog when fog is active
	}

	this->normalLightTableNightTextureRef.unlockTexels();
	this->fogLightTableTextureRef.unlockTexels();

	const ObjectTextureID noneDitherTextureID = RendererUtils::allocDitherTexture(DitheringMode::None, renderer);
	const ObjectTextureID classicDitherTextureID = RendererUtils::allocDitherTexture(DitheringMode::Classic, renderer);
	const ObjectTextureID modernDitherTextureID = RendererUtils::allocDitherTexture(DitheringMode::Modern, renderer);
	this->noneDitherTextureRef.init(noneDitherTextureID, renderer);
	this->classicDitherTextureRef.init(classicDitherTextureID, renderer);
	this->modernDitherTextureRef.init(modernDitherTextureID, renderer);
}

void SceneManager::shutdown(Renderer &renderer)
{
	this->renderVoxelChunkManager.shutdown(renderer);
	this->renderEntityManager.shutdown(renderer);
	this->renderSkyManager.shutdown(renderer);
	this->renderWeatherManager.shutdown(renderer);
	this->renderLightManager.shutdown(renderer);
}

void SceneManager::updateGameWorldPalette(bool isInterior, WeatherType weatherType, bool isFoggy, double dayPercent, TextureManager &textureManager)
{
	constexpr int paletteLength = PaletteLength;
	LockedTexture lockedTexture = this->gameWorldPaletteTextureRef.lockTexels();
	if (!lockedTexture.isValid())
	{
		DebugLogError("Couldn't lock game world palette texture for updating.");
		return;
	}

	Span<uint32_t> gameWorldTexels(lockedTexture.getTexels32().begin(), paletteLength);

	// Refill entire palette due to how locking clears texels in some APIs.
	const Palette &gameWorldPalette = textureManager.getPaletteHandle(this->gameWorldPaletteID);
	std::transform(gameWorldPalette.begin(), gameWorldPalette.end(), gameWorldTexels.begin(),
		[](const Color &paletteColor)
	{
		return paletteColor.toRGBA();
	});

	// Update sky gradient. Write to palette indices 1-8 using one of the three palettes.
	const std::string *skyGradientFilename = &ArenaPaletteName::Default;
	std::optional<int> daytimePaletteIndexOffset;
	if (!isInterior)
	{
		skyGradientFilename = (weatherType == WeatherType::Clear) ? &ArenaPaletteName::Daytime : &ArenaPaletteName::Dreary;

		const double dayPercent6AM = Clock(6, 0, 0).getDayPercent();
		const double dayPercent6PM = Clock(18, 0, 0).getDayPercent();
		const double skyGradientDaytimePercent = (dayPercent - dayPercent6AM) / (dayPercent6PM - dayPercent6AM);
		daytimePaletteIndexOffset = std::clamp(static_cast<int>(skyGradientDaytimePercent * static_cast<double>(paletteLength)), 0, paletteLength - 1);
	}

	const std::optional<PaletteID> skyGradientPaletteID = textureManager.tryGetPaletteID(skyGradientFilename->c_str());
	if (!skyGradientPaletteID.has_value())
	{
		DebugLogErrorFormat("Couldn't get palette ID for sky gradient \"%s\".", skyGradientFilename->c_str());
		return;
	}

	const Palette &skyGradientPalette = textureManager.getPaletteHandle(*skyGradientPaletteID);
	const Span<const Color> skyGradientPaletteTexels(skyGradientPalette);
	DebugAssert((this->gameWorldPaletteTextureRef.getWidth() * this->gameWorldPaletteTextureRef.getHeight()) == skyGradientPaletteTexels.getCount());

	// Update sky gradient depending on time of day.
	int srcTexelsIndexStart = daytimePaletteIndexOffset.has_value() ? *daytimePaletteIndexOffset : 1;
	const int skyGradientColorCount = static_cast<int>(std::size(ArenaRenderUtils::PALETTE_INDICES_SKY_COLOR));
	Span<uint32_t> gameWorldSkyGradientTexels(gameWorldTexels.begin() + 1, skyGradientColorCount);
	for (int i = 0; i < skyGradientColorCount; i++)
	{
		const int srcIndex = (srcTexelsIndexStart + i) % paletteLength;
		gameWorldSkyGradientTexels[i] = skyGradientPaletteTexels[srcIndex].toRGBA();
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
		const ClockLibrary &clockLibrary = ClockLibrary::getInstance();

		// Use transition colors if during sunrise/sunset.
		const Clock &startBrighteningClock = clockLibrary.getClock(ArenaClockUtils::AmbientBrighteningStart);
		const Clock &endBrighteningClock = clockLibrary.getClock(ArenaClockUtils::AmbientBrighteningEnd);
		const Clock &startDimmingClock = clockLibrary.getClock(ArenaClockUtils::AmbientDimmingStart);
		const Clock &endDimmingClock = clockLibrary.getClock(ArenaClockUtils::AmbientDimmingEnd);
		const double startBrighteningPercent = startBrighteningClock.getDayPercent();
		const double endBrighteningPercent = endBrighteningClock.getDayPercent();
		const double startDimmingPercent = startDimmingClock.getDayPercent();
		const double endDimmingPercent = endDimmingClock.getDayPercent();
		const bool isDuringSunrise = (dayPercent >= startBrighteningPercent) && (dayPercent < endBrighteningPercent);
		const bool isDuringSunset = (dayPercent >= startDimmingPercent) && (dayPercent < endDimmingPercent);
		const bool isDuringNight = (dayPercent >= endDimmingPercent) || (dayPercent < startBrighteningPercent);

		const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
		const ExeData &exeData = binaryAssetLibrary.getExeData();
		Span<const uint8_t> windowColorBytes = exeData.light.windowTwilightColors;

		auto getWindowRGBForTransitionPercent = [windowColorBytes](double percent)
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
			const uint8_t windowRScaled = windowR * componentMultiplier;
			const uint8_t windowGScaled = windowG * componentMultiplier;
			const uint8_t windowBScaled = windowB * componentMultiplier;
			return Color(windowRScaled, windowGScaled, windowBScaled).toRGBA();
		};

		if (isDuringSunrise)
		{
			const double transitionPercent = std::clamp((dayPercent - startBrighteningPercent) / (endBrighteningPercent - startBrighteningPercent), 0.0, 1.0);
			windowColor = getWindowRGBForTransitionPercent(transitionPercent);
		}
		else if (isDuringSunset)
		{
			const double transitionPercent = std::clamp(1.0 - ((dayPercent - startDimmingPercent) / (endDimmingPercent - startDimmingPercent)), 0.0, 1.0);
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

void SceneManager::endFrame(JPH::PhysicsSystem &physicsSystem, Renderer &renderer)
{
	this->chunkManager.endFrame();
	this->voxelChunkManager.endFrame();
	this->entityChunkManager.endFrame(physicsSystem, renderer);
	this->voxelFaceCombineChunkManager.endFrame();
	this->renderVoxelChunkManager.endFrame();
	this->renderEntityManager.endFrame();
}
