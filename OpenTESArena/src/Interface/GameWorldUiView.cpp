#include "GameWorldPanel.h"
#include "GameWorldUiModel.h"
#include "GameWorldUiView.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/CFAFile.h"
#include "../Assets/CIFFile.h"
#include "../Game/Game.h"
#include "../Math/Constants.h"
#include "../UI/FontUtils.h"

#include "components/utilities/String.h"

Rect GameWorldUiView::scaleClassicCursorRectToNative(int rectIndex, double xScale, double yScale)
{
	DebugAssertIndex(GameWorldUiView::CursorRegions, rectIndex);
	const Rect &classicRect = GameWorldUiView::CursorRegions[rectIndex];
	return Rect(
		static_cast<int>(std::ceil(static_cast<double>(classicRect.getLeft()) * xScale)),
		static_cast<int>(std::ceil(static_cast<double>(classicRect.getTop()) * yScale)),
		static_cast<int>(std::ceil(static_cast<double>(classicRect.getWidth()) * xScale)),
		static_cast<int>(std::ceil(static_cast<double>(classicRect.getHeight()) * yScale)));
}

Int2 GameWorldUiView::getStatusPopUpTextCenterPoint(Game &game)
{
	return GameWorldUiView::getInterfaceCenter(game);
}

int GameWorldUiView::getStatusPopUpTextureWidth(int textWidth)
{
	return textWidth + 12;
}

int GameWorldUiView::getStatusPopUpTextureHeight(int textHeight)
{
	return textHeight + 12;
}

Int2 GameWorldUiView::getGameWorldInterfacePosition(int textureHeight)
{
	return Int2(0, ArenaRenderUtils::SCREEN_HEIGHT - textureHeight);
}

Int2 GameWorldUiView::getNoMagicTexturePosition()
{
	return Int2(91, 177);
}

Int2 GameWorldUiView::getTriggerTextPosition(Game &game, int textWidth, int textHeight,
	int gameWorldInterfaceTextureHeight)
{
	const auto &options = game.getOptions();
	const bool modernInterface = options.getGraphics_ModernInterface();

	const int textX = (ArenaRenderUtils::SCREEN_WIDTH / 2) - (textWidth / 2) - 1;

	const int interfaceOffset = modernInterface ? (gameWorldInterfaceTextureHeight / 2) : gameWorldInterfaceTextureHeight;
	const int textY = ArenaRenderUtils::SCREEN_HEIGHT - interfaceOffset - textHeight - 3;

	return Int2(textX, textY);
}

Int2 GameWorldUiView::getActionTextPosition(int textWidth)
{
	const int textX = (ArenaRenderUtils::SCREEN_WIDTH / 2) - (textWidth / 2);
	const int textY = 20;
	return Int2(textX, textY);
}

Int2 GameWorldUiView::getEffectTextPosition()
{
	// @todo
	return Int2();
}

Int2 GameWorldUiView::getTooltipPosition(Game &game, int textureHeight)
{
	DebugAssert(!game.getOptions().getGraphics_ModernInterface());

	auto &textureManager = game.getTextureManager();
	const TextureBuilderID gameWorldInterfaceTextureBuilderID =
		GameWorldUiView::getGameWorldInterfaceTextureBuilderID(textureManager);
	const TextureBuilder &gameWorldInterfaceTextureBuilder =
		textureManager.getTextureBuilderHandle(gameWorldInterfaceTextureBuilderID);

	// @todo: wouldn't need texture height if we could specify the anchor/pivot for the rect to be the bottom left
	const int x = 0;
	const int y = ArenaRenderUtils::SCREEN_HEIGHT - gameWorldInterfaceTextureBuilder.getHeight() - textureHeight;
	return Int2(x, y);
}

Rect GameWorldUiView::getCompassClipRect(Game &game, const VoxelDouble2 &playerDirection, int textureHeight)
{
	const double angle = GameWorldUiModel::getCompassAngle(playerDirection);

	// Offset in the "slider" texture. Due to how SLIDER.IMG is drawn, there's a small "pop-in" when turning from
	// N to NE, because N is drawn in two places, but the second place (offset == 256) has tick marks where "NE"
	// should be.
	const int xOffset = static_cast<int>(240.0 + std::round(256.0 * (angle / (2.0 * Constants::Pi)))) % 256;
	return Rect(xOffset, 0, 32, textureHeight);
}

Int2 GameWorldUiView::getCompassSliderPosition(int clipWidth, int clipHeight)
{
	// Top-left corner of the slider in 320x200 space.
	const int sliderX = (ArenaRenderUtils::SCREEN_WIDTH / 2) - (clipWidth / 2);
	const int sliderY = clipHeight;
	return Int2(sliderX, sliderY);
}

Int2 GameWorldUiView::getCompassFramePosition(int textureWidth)
{
	return Int2((ArenaRenderUtils::SCREEN_WIDTH / 2) - (textureWidth / 2), 0);
}

TextureAssetReference GameWorldUiView::getCompassSliderPaletteTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaPaletteName::Default));
}

Int2 GameWorldUiView::getCurrentWeaponAnimationOffset(Game &game)
{
	const auto &weaponAnimation = game.getGameState().getPlayer().getWeaponAnimation();
	const std::string &weaponFilename = weaponAnimation.getAnimationFilename();
	const int weaponAnimIndex = weaponAnimation.getFrameIndex();

	// @todo: add XY offset support to TextureFileMetadata so it can be cached by texture manager.

	if (!weaponAnimation.isRanged())
	{
		// Melee weapon offsets.
		CIFFile cifFile;
		if (!cifFile.init(weaponFilename.c_str()))
		{
			DebugCrash("Couldn't init .CIF file \"" + weaponFilename + "\".");
		}

		return Int2(cifFile.getXOffset(weaponAnimIndex), cifFile.getYOffset(weaponAnimIndex));
	}
	else
	{
		// Ranged weapon offsets.
		CFAFile cfaFile;
		if (!cfaFile.init(weaponFilename.c_str()))
		{
			DebugCrash("Couldn't init .CFA file \"" + weaponFilename + "\".");
		}

		return Int2(cfaFile.getXOffset(), cfaFile.getYOffset());
	}
}

std::optional<TextureBuilderID> GameWorldUiView::getActiveWeaponAnimationTextureBuilderID(Game &game)
{
	auto &gameState = game.getGameState();
	const auto &player = gameState.getPlayer();
	const auto &weaponAnimation = player.getWeaponAnimation();
	if (weaponAnimation.isSheathed())
	{
		return std::nullopt;
	}

	const std::string &weaponFilename = weaponAnimation.getAnimationFilename();
	const int weaponAnimIndex = weaponAnimation.getFrameIndex();
	return GameWorldUiView::getWeaponTextureBuilderID(game, weaponFilename, weaponAnimIndex);
}

Int2 GameWorldUiView::getInterfaceCenter(Game &game)
{
	const bool modernInterface = game.getOptions().getGraphics_ModernInterface();
	if (modernInterface)
	{
		return Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, ArenaRenderUtils::SCREEN_HEIGHT / 2);
	}
	else
	{
		TextureManager &textureManager = game.getTextureManager();
		const TextureBuilderID textureBuilderID = GameWorldUiView::getGameWorldInterfaceTextureBuilderID(textureManager);
		const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(textureBuilderID);

		return Int2(
			ArenaRenderUtils::SCREEN_WIDTH / 2,
			(ArenaRenderUtils::SCREEN_HEIGHT - textureBuilder.getHeight()) / 2);
	}
}

TextureBuilderID GameWorldUiView::getGameWorldInterfaceTextureBuilderID(TextureManager &textureManager)
{
	const std::string &textureFilename = ArenaTextureName::GameWorldInterface;
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureFilename.c_str());
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + textureFilename + "\".");
	}

	return *textureBuilderID;
}

TextureBuilderID GameWorldUiView::getCompassFrameTextureBuilderID(Game &game)
{
	auto &textureManager = game.getTextureManager();
	const std::string &textureFilename = ArenaTextureName::CompassFrame;
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureFilename.c_str());
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + textureFilename + "\".");
	}

	return *textureBuilderID;
}

TextureBuilderID GameWorldUiView::getCompassSliderTextureBuilderID(Game &game)
{
	auto &textureManager = game.getTextureManager();
	const std::string &textureFilename = ArenaTextureName::CompassSlider;
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureFilename.c_str());
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + textureFilename + "\".");
	}

	return *textureBuilderID;
}

TextureBuilderID GameWorldUiView::getPlayerPortraitTextureBuilderID(Game &game,
	const std::string &portraitsFilename, int portraitID)
{
	auto &textureManager = game.getTextureManager();
	const std::optional<TextureBuilderIdGroup> textureBuilderIDs = textureManager.tryGetTextureBuilderIDs(portraitsFilename.c_str());
	if (!textureBuilderIDs.has_value())
	{
		DebugCrash("Couldn't get texture builder IDs for \"" + portraitsFilename + "\".");
	}

	return textureBuilderIDs->getID(portraitID);
}

TextureBuilderID GameWorldUiView::getStatusGradientTextureBuilderID(Game &game, int gradientID)
{
	auto &textureManager = game.getTextureManager();
	const std::string &statusGradientsFilename = ArenaTextureName::StatusGradients;
	const std::optional<TextureBuilderIdGroup> textureBuilderIDs = textureManager.tryGetTextureBuilderIDs(statusGradientsFilename.c_str());
	if (!textureBuilderIDs.has_value())
	{
		DebugCrash("Couldn't get texture builder IDs for \"" + statusGradientsFilename + "\".");
	}

	return textureBuilderIDs->getID(gradientID);
}

TextureBuilderID GameWorldUiView::getNoSpellTextureBuilderID(Game &game)
{
	auto &textureManager = game.getTextureManager();
	const std::string &textureFilename = ArenaTextureName::NoSpell;
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureFilename.c_str());
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + textureFilename + "\".");
	}

	return *textureBuilderID;
}

TextureBuilderID GameWorldUiView::getWeaponTextureBuilderID(Game &game, const std::string &weaponFilename, int index)
{
	auto &textureManager = game.getTextureManager();
	const std::optional<TextureBuilderIdGroup> textureBuilderIDs = textureManager.tryGetTextureBuilderIDs(weaponFilename.c_str());
	if (!textureBuilderIDs.has_value())
	{
		DebugCrash("Couldn't get texture builder IDs for \"" + weaponFilename + "\".");
	}

	return textureBuilderIDs->getID(index);
}

TextureAssetReference GameWorldUiView::getDefaultPaletteTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaPaletteName::Default));
}

// @temp: keep until 3D-DDA ray casting is fully correct (i.e. entire ground is red dots for
// levels where ceilingScale < 1.0, and same with ceiling blue dots).
void GameWorldUiView::DEBUG_ColorRaycastPixel(Game &game)
{
	auto &renderer = game.getRenderer();
	const int selectionDim = 3;
	const Int2 windowDims = renderer.getWindowDimensions();

	constexpr int xOffset = 16;
	constexpr int yOffset = 16;

	auto &gameState = game.getGameState();

	const auto &options = game.getOptions();
	const double verticalFOV = options.getGraphics_VerticalFOV();
	const bool pixelPerfect = options.getInput_PixelPerfectSelection();

	const auto &player = gameState.getPlayer();
	const CoordDouble3 &rayStart = player.getPosition();
	const VoxelDouble3 &cameraDirection = player.getDirection();
	const int viewWidth = windowDims.x;
	const int viewHeight = renderer.getViewHeight();
	const double viewAspectRatio = static_cast<double>(viewWidth) / static_cast<double>(viewHeight);

	const MapInstance &mapInst = gameState.getActiveMapInst();
	const LevelInstance &levelInst = mapInst.getActiveLevel();
	const ChunkManager &chunkManager = levelInst.getChunkManager();
	const EntityManager &entityManager = levelInst.getEntityManager();
	const double ceilingScale = levelInst.getCeilingScale();

	const std::string &paletteFilename = ArenaPaletteName::Default;
	auto &textureManager = game.getTextureManager();
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteFilename.c_str());
	if (!paletteID.has_value())
	{
		DebugCrash("Couldn't get palette ID for \"" + paletteFilename + "\".");
	}

	const Palette &palette = textureManager.getPaletteHandle(*paletteID);

	for (int y = 0; y < windowDims.y; y += yOffset)
	{
		for (int x = 0; x < windowDims.x; x += xOffset)
		{
			const Double3 rayDirection = GameWorldUiModel::screenToWorldRayDirection(game, Int2(x, y));

			// Not registering entities with ray cast hits for efficiency since this debug visualization
			// is for voxels.
			constexpr bool includeEntities = false;
			Physics::Hit hit;
			const bool success = Physics::rayCast(rayStart, rayDirection, ceilingScale, cameraDirection,
				pixelPerfect, palette, includeEntities, levelInst, game.getEntityDefinitionLibrary(),
				renderer, hit);

			if (success)
			{
				Color color;
				switch (hit.getType())
				{
				case Physics::Hit::Type::Voxel:
				{
					const std::array<Color, 5> colors =
					{
						Color::Red, Color::Green, Color::Blue, Color::Cyan, Color::Yellow
					};

					const VoxelInt3 &voxel = hit.getVoxelHit().voxel;
					const int colorsIndex = std::min(voxel.y, 4);
					color = colors[colorsIndex];
					break;
				}
				case Physics::Hit::Type::Entity:
				{
					color = Color::Yellow;
					break;
				}
				}

				renderer.drawRect(color, x, y, selectionDim, selectionDim);
			}
		}
	}
}

// @temp: keep until 3D-DDA ray casting is fully correct (i.e. entire ground is red dots for
// levels where ceilingScale < 1.0, and same with ceiling blue dots).
void GameWorldUiView::DEBUG_PhysicsRaycast(Game &game)
{
	// ray cast out from center and display hit info (faster/better than console logging).
	GameWorldUiView::DEBUG_ColorRaycastPixel(game);

	auto &gameState = game.getGameState();
	const auto &options = game.getOptions();
	const auto &player = gameState.getPlayer();
	const Double3 &cameraDirection = player.getDirection();

	auto &renderer = game.getRenderer();
	const Int2 windowDims = renderer.getWindowDimensions();
	const int viewWidth = windowDims.x;
	const int viewHeight = renderer.getViewHeight();
	const Int2 viewCenterPoint(viewWidth / 2, viewHeight / 2);

	const CoordDouble3 rayStart = player.getPosition();
	const VoxelDouble3 rayDirection = GameWorldUiModel::screenToWorldRayDirection(game, viewCenterPoint);

	const MapInstance &mapInst = gameState.getActiveMapInst();
	const LevelInstance &levelInst = mapInst.getActiveLevel();
	const ChunkManager &chunkManager = levelInst.getChunkManager();
	const EntityManager &entityManager = levelInst.getEntityManager();
	const double ceilingScale = levelInst.getCeilingScale();

	const std::string &paletteFilename = ArenaPaletteName::Default;
	auto &textureManager = game.getTextureManager();
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteFilename.c_str());
	if (!paletteID.has_value())
	{
		DebugCrash("Couldn't get palette ID for \"" + paletteFilename + "\".");
	}

	const Palette &palette = textureManager.getPaletteHandle(*paletteID);

	constexpr bool includeEntities = true;
	Physics::Hit hit;
	const bool success = Physics::rayCast(rayStart, rayDirection, ceilingScale, cameraDirection,
		options.getInput_PixelPerfectSelection(), palette, includeEntities, levelInst,
		game.getEntityDefinitionLibrary(), renderer, hit);

	std::string text;
	if (success)
	{
		switch (hit.getType())
		{
		case Physics::Hit::Type::Voxel:
		{
			const ChunkInt2 chunk = hit.getCoord().chunk;
			const Chunk *chunkPtr = chunkManager.tryGetChunk(chunk);
			DebugAssert(chunkPtr != nullptr);

			const Physics::Hit::VoxelHit &voxelHit = hit.getVoxelHit();
			const VoxelInt3 &voxel = voxelHit.voxel;
			const Chunk::VoxelID voxelID = chunkPtr->getVoxel(voxel.x, voxel.y, voxel.z);
			const VoxelDefinition &voxelDef = chunkPtr->getVoxelDef(voxelID);

			text = "Voxel: (" + voxel.toString() + "), " + std::to_string(static_cast<int>(voxelDef.type)) +
				' ' + std::to_string(hit.getT());
			break;
		}
		case Physics::Hit::Type::Entity:
		{
			const Physics::Hit::EntityHit &entityHit = hit.getEntityHit();
			const auto &exeData = game.getBinaryAssetLibrary().getExeData();

			// Try inspecting the entity (can be from any distance). If they have a display name,
			// then show it.
			ConstEntityRef entityRef = entityManager.getEntityRef(entityHit.id, entityHit.type);
			DebugAssert(entityRef.getID() != EntityManager::NO_ID);

			const EntityDefinition &entityDef = entityManager.getEntityDef(
				entityRef.get()->getDefinitionID(), game.getEntityDefinitionLibrary());
			const auto &charClassLibrary = game.getCharacterClassLibrary();

			std::string entityName;
			if (EntityUtils::tryGetDisplayName(entityDef, charClassLibrary, &entityName))
			{
				text = std::move(entityName);
			}
			else
			{
				// Placeholder text for testing.
				text = "Entity " + std::to_string(entityHit.id);
			}

			text.append(' ' + std::to_string(hit.getT()));
			break;
		}
		default:
			text.append("Unknown hit type");
			break;
		}
	}
	else
	{
		text = "No hit";
	}

	const auto &fontLibrary = game.getFontLibrary();
	const RichTextString richText(
		text,
		FontName::Arena,
		Color::White,
		TextAlignment::Left,
		fontLibrary);

	const TextBox textBox(0, 0, richText, fontLibrary, renderer);
	const int originalX = ArenaRenderUtils::SCREEN_WIDTH / 2;
	const int originalY = (ArenaRenderUtils::SCREEN_HEIGHT / 2) + 10;
	renderer.drawOriginal(textBox.getTexture(), originalX, originalY);
}

void GameWorldUiView::DEBUG_DrawProfiler(Game &game, Renderer &renderer)
{
	const auto &options = game.getOptions();
	const int profilerLevel = options.getMisc_ProfilerLevel();
	if (profilerLevel == Options::MIN_PROFILER_LEVEL)
	{
		return;
	}

	DebugAssert(profilerLevel <= Options::MAX_PROFILER_LEVEL);

	const FPSCounter &fpsCounter = game.getFPSCounter();
	const double fps = fpsCounter.getAverageFPS();
	const double frameTimeMS = 1000.0 / fps;

	const int targetFps = options.getGraphics_TargetFPS();
	const int minFps = Options::MIN_FPS;

	// Draw each profiler level with its own draw call.
	if (profilerLevel >= 1)
	{
		// FPS.
		const std::string fpsText = String::fixedPrecision(fps, 1);
		const std::string frameTimeText = String::fixedPrecision(frameTimeMS, 1);
		const std::string text = "FPS: " + fpsText + " (" + frameTimeText + "ms)";

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			text,
			FontName::D,
			Color::White,
			TextAlignment::Left,
			fontLibrary);

		const int x = 2;
		const int y = 2;
		const TextBox textBox(x, y, richText, fontLibrary, renderer);
		renderer.drawOriginal(textBox.getTexture(), x, y);
	}

	if (profilerLevel >= 2)
	{
		// Screen, renderer, timing, and player info.
		const Int2 windowDims = renderer.getWindowDimensions();

		const Renderer::ProfilerData &profilerData = renderer.getProfilerData();
		const Int2 renderDims(profilerData.width, profilerData.height);
		const double resolutionScale = options.getGraphics_ResolutionScale();

		GameState &gameState = game.getGameState();
		const auto &player = gameState.getPlayer();
		const CoordDouble3 &playerPosition = player.getPosition();
		const Double3 &direction = player.getDirection();

		const std::string windowWidth = std::to_string(windowDims.x);
		const std::string windowHeight = std::to_string(windowDims.y);

		const std::string renderWidth = std::to_string(renderDims.x);
		const std::string renderHeight = std::to_string(renderDims.y);
		const std::string renderResScale = String::fixedPrecision(resolutionScale, 2);
		const std::string renderThreadCount = std::to_string(profilerData.threadCount);

		const std::string chunkStr = playerPosition.chunk.toString();
		const std::string chunkPosX = String::fixedPrecision(playerPosition.point.x, 2);
		const std::string chunkPosY = String::fixedPrecision(playerPosition.point.y, 2);
		const std::string chunkPosZ = String::fixedPrecision(playerPosition.point.z, 2);
		const std::string dirX = String::fixedPrecision(direction.x, 2);
		const std::string dirY = String::fixedPrecision(direction.y, 2);
		const std::string dirZ = String::fixedPrecision(direction.z, 2);

		std::string text =
			"Screen: " + windowWidth + "x" + windowHeight + '\n' +
			"Render: " + renderWidth + "x" + renderHeight + " (" + renderResScale + "), " +
			renderThreadCount + " thread" + ((profilerData.threadCount > 1) ? "s" : "") + '\n' +
			"Chunk: " + chunkStr + '\n' +
			"Chunk pos: " + chunkPosX + ", " + chunkPosY + ", " + chunkPosZ + '\n' +
			"Dir: " + dirX + ", " + dirY + ", " + dirZ;

		auto &fontLibrary = game.getFontLibrary();
		const FontName fontName = FontName::D;
		const RichTextString richText(
			text,
			fontName,
			Color::White,
			TextAlignment::Left,
			fontLibrary);

		// Get character height of the FPS font so Y position is correct.
		const char *fontNameStr = FontUtils::fromName(fontName);
		int fontIndex;
		if (!fontLibrary.tryGetDefinitionIndex(fontNameStr, &fontIndex))
		{
			DebugLogWarning("Couldn't get font \"" + std::string(fontNameStr) + "\".");
			return;
		}

		const FontDefinition &fontDef = fontLibrary.getDefinition(fontIndex);
		const int yOffset = fontDef.getCharacterHeight();

		const int x = 2;
		const int y = 2 + yOffset;
		const TextBox textBox(x, y, richText, fontLibrary, renderer);
		renderer.drawOriginal(textBox.getTexture(), x, y);
	}

	if (profilerLevel >= 3)
	{
		// Draw frame times and graph.
		const Renderer::ProfilerData &profilerData = renderer.getProfilerData();
		const std::string renderTime = String::fixedPrecision(profilerData.frameTime * 1000.0, 2);

		const std::string text =
			"3D render: " + renderTime + "ms" + "\n" +
			"Vis flats: " + std::to_string(profilerData.visFlatCount) + " (" +
			std::to_string(profilerData.potentiallyVisFlatCount) + ")" +
			", lights: " + std::to_string(profilerData.visLightCount) + "\n" +
			"FPS Graph:" + '\n' +
			"                               " + std::to_string(targetFps) + "\n\n\n\n" +
			"                               " + std::to_string(0);

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			text,
			FontName::D,
			Color::White,
			TextAlignment::Left,
			fontLibrary);

		const int x = 2;
		const int y = 72;
		const TextBox textBox(x, y, richText, fontLibrary, renderer);

		const Texture frameTimesGraph = [&renderer, &game, &fpsCounter, targetFps]()
		{
			// Graph maximum is target FPS, minimum is MIN_FPS.
			const int columnWidth = 1;
			const int width = fpsCounter.getFrameCount() * columnWidth;
			const int height = 32;
			Surface surface = Surface::createWithFormat(
				width, height, Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);
			surface.fill(0, 0, 0, 128);

			const std::array<uint32_t, 3> Colors =
			{
				surface.mapRGBA(255, 0, 0, 128),
				surface.mapRGBA(255, 255, 0, 128),
				surface.mapRGBA(0, 255, 0, 128)
			};

			auto drawGraphColumn = [columnWidth, &surface, &Colors](int x, double percent)
			{
				const uint32_t color = [&Colors, percent]()
				{
					const int colorIndex = [percent]()
					{
						if (percent < (1.0 / 3.0))
						{
							return 0;
						}
						else if (percent < (2.0 / 3.0))
						{
							return 1;
						}
						else
						{
							return 2;
						}
					}();

					return Colors.at(colorIndex);
				}();

				// Height of column in pixels.
				const int height = std::clamp(static_cast<int>(
					percent * static_cast<double>(surface.getHeight())), 0, surface.getHeight());

				const Rect rect(x * columnWidth, surface.getHeight() - height, columnWidth, height);
				surface.fillRect(rect, color);
			};

			// Fill in columns.
			const double targetFpsReal = static_cast<double>(targetFps);
			for (int i = 0; i < fpsCounter.getFrameCount(); i++)
			{
				const double frameTime = fpsCounter.getFrameTime(i);
				const double fps = 1.0 / frameTime;
				const double fpsPercent = std::clamp(fps / targetFpsReal, 0.0, 1.0);
				drawGraphColumn(i, fpsPercent);
			}

			return renderer.createTextureFromSurface(surface);
		}();

		renderer.drawOriginal(textBox.getTexture(), textBox.getX(), textBox.getY());
		renderer.drawOriginal(frameTimesGraph, textBox.getX(), 94);
	}

	// @temp: keep until 3D-DDA ray casting is fully correct (i.e. entire ground is red dots for
	// levels where ceilingScale < 1.0, and same with ceiling blue dots).
	if (profilerLevel == Options::MAX_PROFILER_LEVEL)
	{
		GameWorldUiView::DEBUG_PhysicsRaycast(game);
	}
}