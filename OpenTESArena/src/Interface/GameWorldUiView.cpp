#include <algorithm>
#include <cmath>

#include "GameWorldPanel.h"
#include "GameWorldUiModel.h"
#include "GameWorldUiView.h"
#include "InventoryUiView.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaPortraitUtils.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Collision/RayCastTypes.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Game/Game.h"
#include "../Math/Constants.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../Stats/CharacterRaceLibrary.h"
#include "../UI/ArenaFontName.h"
#include "../UI/FontLibrary.h"
#include "../UI/GuiUtils.h"
#include "../UI/Surface.h"
#include "../UI/UiCommand.h"

#include "components/utilities/String.h"

namespace
{
	UiTextureID AllocStatusBarTexture(const Rect &rect, const Color &color, Renderer &renderer)
	{
		const int width = rect.width;
		const int height = rect.height;

		const UiTextureID textureID = renderer.createUiTexture(width, height);
		if (textureID < 0)
		{
			DebugLogErrorFormat("Couldn't create status bar texture with color (%s).", color.toString().c_str());
			return -1;
		}

		LockedTexture lockedTexture = renderer.lockUiTexture(textureID);
		if (!lockedTexture.isValid())
		{
			DebugLogErrorFormat("Couldn't lock status bar texture with color (%s).", color.toString().c_str());
			return textureID;
		}

		Span2D<uint32_t> texelsView(reinterpret_cast<uint32_t*>(lockedTexture.texels.begin()), width, height);
		const uint32_t texelRGBA = color.toRGBA();
		texelsView.fill(texelRGBA);
		renderer.unlockUiTexture(textureID);

		return textureID;
	}
}

DebugVoxelVisibilityQuadtreeState::DebugVoxelVisibilityQuadtreeState()
{
	std::fill(std::begin(this->textureIDs), std::end(this->textureIDs), -1);
	std::fill(std::begin(this->drawPositionYs), std::end(this->drawPositionYs), 0);
}

void DebugVoxelVisibilityQuadtreeState::populateCommandList(Game &game, UiCommandList &commandList)
{
	const SceneManager &sceneManager = game.sceneManager;
	const Player &player = game.player;
	const CoordDouble3 playerCoord = player.getEyeCoord();
	const VoxelInt2 playerVoxelXZ = VoxelUtils::pointToVoxel(playerCoord.point.getXZ());
	const VoxelFrustumCullingChunkManager &voxelFrustumCullingChunkManager = sceneManager.voxelFrustumCullingChunkManager;
	const VoxelFrustumCullingChunk *playerVoxelFrustumCullingChunk = voxelFrustumCullingChunkManager.findChunkAtPosition(playerCoord.chunk);
	if (playerVoxelFrustumCullingChunk == nullptr)
	{
		return;
	}

	constexpr uint8_t alpha = 192;
	constexpr uint32_t visibleColor = Color(0, 255, 0, alpha).toRGBA();
	constexpr uint32_t partiallyVisibleColor = Color(255, 255, 0, alpha).toRGBA();
	constexpr uint32_t invisibleColor = Color(255, 0, 0, alpha).toRGBA();
	constexpr uint32_t playerColor = Color(255, 255, 255, alpha).toRGBA();

	Renderer &renderer = game.renderer;

	for (int treeLevelIndex = 0; treeLevelIndex < VoxelFrustumCullingChunk::TREE_LEVEL_COUNT; treeLevelIndex++)
	{
		const UiTextureID quadtreeTextureID = this->textureIDs[treeLevelIndex];
		const Int2 quadtreeTextureDims = this->textureDimsList[treeLevelIndex];
		const int quadtreeSideLength = VoxelFrustumCullingChunk::NODES_PER_SIDE[treeLevelIndex];

		LockedTexture lockedTexture = renderer.lockUiTexture(quadtreeTextureID);
		Span2D<uint32_t> quadtreeTexels = lockedTexture.getTexels32();

		for (int y = 0; y < quadtreeTextureDims.y; y++)
		{
			for (int x = 0; x < quadtreeTextureDims.x; x++)
			{
				VisibilityType visibilityType = VisibilityType::Outside;
				const bool isLeaf = treeLevelIndex == VoxelFrustumCullingChunk::TREE_LEVEL_INDEX_LEAF;
				if (isLeaf)
				{
					const int leafNodeIndex = y + (x * quadtreeTextureDims.y);
					DebugAssertIndex(playerVoxelFrustumCullingChunk->leafNodeFrustumTests, leafNodeIndex);
					visibilityType = playerVoxelFrustumCullingChunk->leafNodeFrustumTests[leafNodeIndex] ? VisibilityType::Inside : VisibilityType::Outside;
				}
				else
				{
					const int globalNodeOffset = VoxelFrustumCullingChunk::GLOBAL_NODE_OFFSETS[treeLevelIndex];
					const int internalNodeIndex = globalNodeOffset + (y + (x * quadtreeTextureDims.y));
					DebugAssertIndex(playerVoxelFrustumCullingChunk->internalNodeVisibilityTypes, internalNodeIndex);
					visibilityType = playerVoxelFrustumCullingChunk->internalNodeVisibilityTypes[internalNodeIndex];
				}

				const int dstX = (quadtreeTextureDims.x - 1) - x;
				const int dstY = y;
				uint32_t color = 0;

				const bool inPlayerVoxel = isLeaf && (y == playerVoxelXZ.x) && (x == playerVoxelXZ.y);
				if (inPlayerVoxel)
				{
					color = playerColor;
				}
				else
				{
					switch (visibilityType)
					{
					case VisibilityType::Outside:
						color = invisibleColor;
						break;
					case VisibilityType::Partial:
						color = partiallyVisibleColor;
						break;
					case VisibilityType::Inside:
						color = visibleColor;
						break;
					}
				}

				quadtreeTexels.set(dstX, dstY, color);
			}
		}

		renderer.unlockUiTexture(quadtreeTextureID);

		const Int2 position(ArenaRenderUtils::SCREEN_WIDTH, this->drawPositionYs[treeLevelIndex]);
		const Int2 size = quadtreeTextureDims;
		const Window &window = game.window;
		const Int2 windowDims = window.getPixelDimensions();
		const Rect letterboxRect = window.getLetterboxRect();

		RenderElement2D &renderElement = this->renderElements[treeLevelIndex];
		renderElement.id = this->textureIDs[treeLevelIndex];
		renderElement.rect = GuiUtils::makeWindowSpaceRect(position.x, position.y, size.x, size.y, PivotType::TopRight, UiRenderSpace::Classic, windowDims.x, windowDims.y, letterboxRect);
	}

	commandList.addElements(this->renderElements);
}

void DebugVoxelVisibilityQuadtreeState::free(Renderer &renderer)
{
	for (UiTextureID &textureID : this->textureIDs)
	{
		if (textureID >= 0)
		{
			renderer.freeUiTexture(textureID);
			textureID = -1;
		}
	}
}

Rect GameWorldUiView::scaleClassicCursorRectToNative(int rectIndex, double xScale, double yScale)
{
	DebugAssertIndex(GameWorldUiView::CursorRegions, rectIndex);
	const Rect &classicRect = GameWorldUiView::CursorRegions[rectIndex];
	return Rect(
		static_cast<int>(std::ceil(static_cast<double>(classicRect.getLeft()) * xScale)),
		static_cast<int>(std::ceil(static_cast<double>(classicRect.getTop()) * yScale)),
		static_cast<int>(std::ceil(static_cast<double>(classicRect.width) * xScale)),
		static_cast<int>(std::ceil(static_cast<double>(classicRect.height) * yScale)));
}

TextBoxInitInfo GameWorldUiView::getPlayerNameTextBoxInitInfo(const std::string_view text,
	const FontLibrary &fontLibrary)
{
	return TextBoxInitInfo::makeWithXY(
		text,
		GameWorldUiView::PlayerNameTextBoxX,
		GameWorldUiView::PlayerNameTextBoxY,
		GameWorldUiView::PlayerNameFontName,
		GameWorldUiView::PlayerNameTextColor,
		GameWorldUiView::PlayerNameTextAlignment,
		fontLibrary);
}

Rect GameWorldUiView::getCharacterSheetButtonRect()
{
	return Rect(14, 166, 40, 29);
}

Rect GameWorldUiView::getPlayerPortraitRect()
{
	return GameWorldUiView::getCharacterSheetButtonRect();
}

Rect GameWorldUiView::getWeaponSheathButtonRect()
{
	return Rect(88, 151, 29, 22);
}

Rect GameWorldUiView::getStealButtonRect()
{
	return Rect(147, 151, 29, 22);
}

Rect GameWorldUiView::getStatusButtonRect()
{
	return Rect(177, 151, 29, 22);
}

Rect GameWorldUiView::getMagicButtonRect()
{
	return Rect(88, 175, 29, 22);
}

Rect GameWorldUiView::getLogbookButtonRect()
{
	return Rect(118, 175, 29, 22);
}

Rect GameWorldUiView::getUseItemButtonRect()
{
	return Rect(147, 175, 29, 22);
}

Rect GameWorldUiView::getCampButtonRect()
{
	return Rect(177, 175, 29, 22);
}

Rect GameWorldUiView::getScrollUpButtonRect()
{
	return Rect(208, ArenaRenderUtils::SCENE_VIEW_HEIGHT + 3, 9, 9);
}

Rect GameWorldUiView::getScrollDownButtonRect()
{
	return Rect(208, ArenaRenderUtils::SCENE_VIEW_HEIGHT + 44, 9, 9);
}

Rect GameWorldUiView::getMapButtonRect()
{
	return Rect(118, 151, 29, 22);
}

Rect GameWorldUiView::getButtonRect(GameWorldUiModel::ButtonType buttonType)
{
	switch (buttonType)
	{
	case GameWorldUiModel::ButtonType::CharacterSheet:
		return GameWorldUiView::getCharacterSheetButtonRect();
	case GameWorldUiModel::ButtonType::ToggleWeapon:
		return GameWorldUiView::getWeaponSheathButtonRect();
	case GameWorldUiModel::ButtonType::Map:
		return GameWorldUiView::getMapButtonRect();
	case GameWorldUiModel::ButtonType::Steal:
		return GameWorldUiView::getStealButtonRect();
	case GameWorldUiModel::ButtonType::Status:
		return GameWorldUiView::getStatusButtonRect();
	case GameWorldUiModel::ButtonType::Magic:
		return GameWorldUiView::getMagicButtonRect();
	case GameWorldUiModel::ButtonType::Logbook:
		return GameWorldUiView::getLogbookButtonRect();
	case GameWorldUiModel::ButtonType::UseItem:
		return GameWorldUiView::getUseItemButtonRect();
	case GameWorldUiModel::ButtonType::Camp:
		return GameWorldUiView::getCampButtonRect();
	default:
		DebugUnhandledReturnMsg(Rect, std::to_string(static_cast<int>(buttonType)));
	}
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

Int2 GameWorldUiView::getGameWorldInterfacePosition()
{
	return Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, ArenaRenderUtils::SCREEN_HEIGHT);
}

int GameWorldUiView::getStatusBarCurrentHeight(int maxHeight, double currentValue, double maxValue)
{
	const double percent = currentValue / maxValue;
	const double currentHeightReal = std::round(static_cast<double>(maxHeight) * percent);
	return std::clamp(static_cast<int>(currentHeightReal), 0, maxHeight);
}

Int2 GameWorldUiView::getNoMagicTexturePosition()
{
	return Int2(91, 177);
}

int GameWorldUiView::getKeyTextureCount(TextureManager &textureManager)
{
	const TextureAsset &textureAsset = GameWorldUiView::getKeyTextureAsset(0);
	std::optional<TextureFileMetadataID> textureFileMetadataID = textureManager.tryGetMetadataID(textureAsset.filename.c_str());
	if (!textureFileMetadataID.has_value())
	{
		DebugLogErrorFormat("Couldn't get texture file metadata ID for key textures \"%s\".", textureAsset.filename.c_str());
		return 0;
	}

	const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*textureFileMetadataID);
	return textureFileMetadata.getTextureCount();
}

Int2 GameWorldUiView::getKeyPosition(int keyIndex)
{
	return Int2(8, 16 + (10 * keyIndex));
}

Int2 GameWorldUiView::getTriggerTextPosition(Game &game, int gameWorldInterfaceTextureHeight)
{
	const auto &options = game.options;
	const bool modernInterface = options.getGraphics_ModernInterface();

	const int textX = ArenaRenderUtils::SCREEN_WIDTH / 2;

	const int interfaceOffsetY = modernInterface ? (gameWorldInterfaceTextureHeight / 2) : gameWorldInterfaceTextureHeight;
	const int textY = ArenaRenderUtils::SCREEN_HEIGHT - interfaceOffsetY - 3;

	return Int2(textX, textY);
}

Int2 GameWorldUiView::getActionTextPosition()
{
	const int textX = ArenaRenderUtils::SCREEN_WIDTH / 2;
	const int textY = 20;
	return Int2(textX, textY);
}

Int2 GameWorldUiView::getEffectTextPosition()
{
	// @todo
	return Int2::Zero;
}

double GameWorldUiView::getTriggerTextSeconds(const std::string_view text)
{
	return std::max(2.50, static_cast<double>(text.size()) * 0.050);
}

double GameWorldUiView::getActionTextSeconds(const std::string_view text)
{
	return std::max(2.25, static_cast<double>(text.size()) * 0.050);
}

double GameWorldUiView::getEffectTextSeconds(const std::string_view text)
{
	return std::max(2.50, static_cast<double>(text.size()) * 0.050);
}

TextBoxInitInfo GameWorldUiView::getTriggerTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	constexpr int maxNewLines = 6;

	std::string dummyText;
	for (int i = 0; i < maxNewLines; i++)
	{
		std::string dummyLine(40, TextRenderUtils::LARGEST_CHAR); // Arbitrary worst-case line size.
		dummyText += dummyLine + '\n';
	}

	const TextRenderShadowInfo shadow(
		GameWorldUiView::TriggerTextShadowOffsetX,
		GameWorldUiView::TriggerTextShadowOffsetY,
		GameWorldUiView::TriggerTextShadowColor);

	return TextBoxInitInfo::makeWithCenter(
		dummyText,
		Int2::Zero, // @todo: needs to be a variable due to classic/modern mode. Maybe make two text boxes?
		GameWorldUiView::TriggerTextFontName,
		GameWorldUiView::TriggerTextColor,
		GameWorldUiView::TriggerTextAlignment,
		shadow,
		GameWorldUiView::TriggerTextLineSpacing,
		fontLibrary);
}

TextBoxInitInfo GameWorldUiView::getActionTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	std::string dummyText;
	for (int i = 0; i < 2; i++)
	{
		std::string dummyLine(35, TextRenderUtils::LARGEST_CHAR); // Arbitrary worst-case line size.
		dummyText += dummyLine + '\n';
	}

	const TextRenderShadowInfo shadow(
		GameWorldUiView::ActionTextShadowOffsetX,
		GameWorldUiView::ActionTextShadowOffsetY,
		GameWorldUiView::ActionTextShadowColor);

	return TextBoxInitInfo::makeWithCenter(
		dummyText,
		Int2::Zero, // @todo: needs to be a variable due to classic/modern mode. Maybe make two text boxes?
		GameWorldUiView::ActionTextFontName,
		GameWorldUiView::ActionTextColor,
		GameWorldUiView::ActionTextAlignment,
		shadow,
		0,
		fontLibrary);
}

TextBoxInitInfo GameWorldUiView::getEffectTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	DebugNotImplemented();
	return TextBoxInitInfo();
}

ListBoxProperties GameWorldUiView::getLootListBoxProperties()
{
	const FontLibrary &fontLibrary = FontLibrary::getInstance();

	constexpr const char *fontName = ArenaFontName::Teeny;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName, &fontDefIndex))
	{
		DebugCrash("Couldn't get loot list box font \"" + std::string(fontName) + "\".");
	}

	constexpr int maxDisplayedItemCount = 7;
	std::string dummyText;
	for (int i = 0; i < maxDisplayedItemCount; i++)
	{
		if (i > 0)
		{
			dummyText += '\n';
		}

		std::string dummyLine(24, TextRenderUtils::LARGEST_CHAR); // Arbitrary worst-case line size.
		dummyText += dummyLine;
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(dummyText, fontDef);

	const Color itemColor = InventoryUiView::PlayerInventoryEquipmentColor;
	constexpr double scrollScale = 1.0;
	return ListBoxProperties(fontDefIndex, textureGenInfo, fontDef.getCharacterHeight(), itemColor, scrollScale);
}

Int2 GameWorldUiView::getTooltipPosition(Game &game)
{
	DebugAssert(!game.options.getGraphics_ModernInterface());

	const int x = 0;
	const int y = ArenaRenderUtils::SCREEN_HEIGHT - GameWorldUiView::UiBottomRegion.height;
	return Int2(x, y);
}

Rect GameWorldUiView::getCompassClipRect()
{
	constexpr int width = 32;
	constexpr int height = 7;
	return Rect(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - (width / 2),
		height,
		width,
		height);
}

Int2 GameWorldUiView::getCompassSliderPosition(Game &game, const VoxelDouble2 &playerDirection)
{
	const double angle = GameWorldUiModel::getCompassAngle(playerDirection);

	// Offset in the "slider" texture. Due to how SLIDER.IMG is drawn, there's a small "pop-in" when turning from
	// N to NE, because N is drawn in two places, but the second place (offset == 256) has tick marks where "NE"
	// should be.
	const int xOffset = static_cast<int>(240.0 + std::round(256.0 * (angle / (2.0 * Constants::Pi)))) % 256;
	const Rect clipRect = GameWorldUiView::getCompassClipRect();
	return clipRect.getTopLeft() - Int2(xOffset, 0);
}

Int2 GameWorldUiView::getCompassFramePosition()
{
	return Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, 0);
}

Int2 GameWorldUiView::getWeaponAnimationOffset(const std::string &weaponFilename, int frameIndex,
	TextureManager &textureManager)
{
	// @todo: this is obsoleted by WeaponAnimationDefinition

	const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(weaponFilename.c_str());
	if (!metadataID.has_value())
	{
		DebugCrash("Couldn't get weapon animation metadata from \"" + weaponFilename + "\".");
	}

	const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);
	return textureFileMetadata.getOffset(frameIndex);
}

Int2 GameWorldUiView::getInterfaceCenter(Game &game)
{
	const bool modernInterface = game.options.getGraphics_ModernInterface();
	if (modernInterface)
	{
		return Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, ArenaRenderUtils::SCREEN_HEIGHT / 2);
	}
	else
	{
		return Int2(
			ArenaRenderUtils::SCREEN_WIDTH / 2,
			(ArenaRenderUtils::SCREEN_HEIGHT - GameWorldUiView::UiBottomRegion.height) / 2);
	}
}

Int2 GameWorldUiView::getNativeWindowCenter(const Window &window)
{
	const Int2 windowDims = window.getPixelDimensions();
	const Int2 nativeCenter = windowDims / 2;
	return nativeCenter;
}

TextureAsset GameWorldUiView::getPaletteTextureAsset()
{
	return TextureAsset(std::string(ArenaPaletteName::Default));
}

TextureAsset GameWorldUiView::getGameWorldInterfaceTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::GameWorldInterface));
}

TextureAsset GameWorldUiView::getStatusGradientTextureAsset(StatusGradientType gradientType)
{
	const int gradientID = static_cast<int>(gradientType);
	return TextureAsset(std::string(ArenaTextureName::StatusGradients), gradientID);
}

TextureAsset GameWorldUiView::getPlayerPortraitTextureAsset(bool isMale, int raceID, int portraitID)
{
	const CharacterRaceLibrary &characterRaceLibrary = CharacterRaceLibrary::getInstance();
	const CharacterRaceDefinition &characterRaceDefinition = characterRaceLibrary.getDefinition(raceID);

	if (isMale)
	{
		return TextureAsset(std::string(characterRaceDefinition.maleGameUiHeadsFilename), portraitID);
	}
	else
	{
		return TextureAsset(std::string(characterRaceDefinition.femaleGameUiHeadsFilename), portraitID);
	}
}

TextureAsset GameWorldUiView::getNoMagicTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::NoSpell));
}

TextureAsset GameWorldUiView::getWeaponAnimTextureAsset(const std::string &weaponFilename, int index)
{
	// @todo: this is obsoleted by WeaponAnimationDefinition
	return TextureAsset(std::string(weaponFilename), index);
}

TextureAsset GameWorldUiView::getCompassFrameTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::CompassFrame));
}

TextureAsset GameWorldUiView::getCompassSliderTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::CompassSlider));
}

TextureAsset GameWorldUiView::getArrowCursorTextureAsset(int cursorIndex)
{
	return TextureAsset(std::string(ArenaTextureName::ArrowCursors), cursorIndex);
}

TextureAsset GameWorldUiView::getKeyTextureAsset(int keyIndex)
{
	return TextureAsset(std::string(ArenaTextureName::DoorKeys), keyIndex);
}

TextureAsset GameWorldUiView::getContainerInventoryTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::ContainerInventory));
}

UiTextureID GameWorldUiView::allocGameWorldInterfaceTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = GameWorldUiView::getGameWorldInterfaceTextureAsset();
	const TextureAsset paletteTextureAsset = GameWorldUiView::getPaletteTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for game world interface.");
	}

	return textureID;
}

UiTextureID GameWorldUiView::allocHealthBarTexture(TextureManager &textureManager, Renderer &renderer)
{
	return AllocStatusBarTexture(GameWorldUiView::HealthBarRect, GameWorldUiView::HealthBarColor, renderer);
}

UiTextureID GameWorldUiView::allocStaminaBarTexture(TextureManager &textureManager, Renderer &renderer)
{
	return AllocStatusBarTexture(GameWorldUiView::StaminaBarRect, GameWorldUiView::StaminaBarColor, renderer);
}

UiTextureID GameWorldUiView::allocSpellPointsBarTexture(TextureManager &textureManager, Renderer &renderer)
{
	return AllocStatusBarTexture(GameWorldUiView::SpellPointsBarRect, GameWorldUiView::SpellPointsBarColor, renderer);
}

UiTextureID GameWorldUiView::allocStatusGradientTexture(StatusGradientType gradientType,
	TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = GameWorldUiView::getStatusGradientTextureAsset(gradientType);
	const TextureAsset paletteTextureAsset = GameWorldUiView::getPaletteTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for status gradient " + std::to_string(static_cast<int>(gradientType)) + ".");
	}

	return textureID;
}

UiTextureID GameWorldUiView::allocPlayerPortraitTexture(bool isMale, int raceID, int portraitID,
	TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = GameWorldUiView::getPlayerPortraitTextureAsset(isMale, raceID, portraitID);
	const TextureAsset paletteTextureAsset = GameWorldUiView::getPaletteTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for player portrait (male: " + std::to_string(static_cast<int>(isMale)) +
			", race: " + std::to_string(raceID) + ", portrait: " + std::to_string(portraitID) + ").");
	}

	return textureID;
}

UiTextureID GameWorldUiView::allocNoMagicTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = GameWorldUiView::getNoMagicTextureAsset();
	const TextureAsset paletteTextureAsset = GameWorldUiView::getPaletteTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for no magic icon.");
	}

	return textureID;
}

UiTextureID GameWorldUiView::allocWeaponAnimTexture(const std::string &weaponFilename, int index,
	TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = GameWorldUiView::getWeaponAnimTextureAsset(weaponFilename, index);
	const TextureAsset paletteTextureAsset = GameWorldUiView::getPaletteTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for weapon animation \"" + weaponFilename +
			"\" index " + std::to_string(index) + ".");
	}

	return textureID;
}

UiTextureID GameWorldUiView::allocCompassFrameTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = GameWorldUiView::getCompassFrameTextureAsset();
	const TextureAsset paletteTextureAsset = GameWorldUiView::getPaletteTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for compass frame.");
	}

	return textureID;
}

UiTextureID GameWorldUiView::allocCompassSliderTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = GameWorldUiView::getCompassSliderTextureAsset();
	const TextureAsset paletteTextureAsset = GameWorldUiView::getPaletteTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for compass frame.");
	}

	return textureID;
}

UiTextureID GameWorldUiView::allocTooltipTexture(GameWorldUiModel::ButtonType buttonType,
	const FontLibrary &fontLibrary, Renderer &renderer)
{
	const std::string text = GameWorldUiModel::getButtonTooltip(buttonType);
	const Surface surface = TextureUtils::createTooltip(text, fontLibrary);

	Span2D<const uint32_t> pixels = surface.getPixels();
	const UiTextureID textureID = renderer.createUiTexture(pixels.getWidth(), pixels.getHeight());
	if (textureID < 0)
	{
		DebugLogErrorFormat("Couldn't create tooltip texture for \"%s\".", text.c_str());
		return -1;
	}

	if (!renderer.populateUiTextureNoPalette(textureID, pixels))
	{
		DebugLogErrorFormat("Couldn't populate tooltip texture for \"%s\".", text.c_str());
	}

	return textureID;
}

UiTextureID GameWorldUiView::allocArrowCursorTexture(int cursorIndex, TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = GameWorldUiView::getArrowCursorTextureAsset(cursorIndex);
	const TextureAsset paletteTextureAsset = GameWorldUiView::getPaletteTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for arrow cursor " + std::to_string(cursorIndex) + ".");
	}

	return textureID;
}

UiTextureID GameWorldUiView::allocModernModeReticleTexture(TextureManager &textureManager, Renderer &renderer)
{
	constexpr int width = 7;
	constexpr int height = width;

	const UiTextureID textureID = renderer.createUiTexture(width, height);
	if (textureID < 0)
	{
		DebugCrash("Couldn't create modern mode cursor texture.");
	}

	LockedTexture lockedTexture = renderer.lockUiTexture(textureID);
	Span2D<uint32_t> texelsView = lockedTexture.getTexels32();

	constexpr Color cursorBgColor(0, 0, 0, 0);
	const uint32_t cursorBgRGBA = cursorBgColor.toRGBA();
	texelsView.fill(cursorBgRGBA);

	constexpr Color cursorColor(255, 255, 255, 160);
	const uint32_t cursorColorRGBA = cursorColor.toRGBA();

	constexpr int middleX = width / 2;
	constexpr int middleY = height / 2;

	for (int x = 0; x < (middleX - 1); x++)
	{
		texelsView.set(x, middleY, cursorColorRGBA);
		texelsView.set(width - x - 1, middleY, cursorColorRGBA);
	}

	for (int y = 0; y < (middleY - 1); y++)
	{
		texelsView.set(middleX, y, cursorColorRGBA);
		texelsView.set(middleX, height - y - 1, cursorColorRGBA);
	}

	renderer.unlockUiTexture(textureID);

	return textureID;
}

UiTextureID GameWorldUiView::allocKeyTexture(int keyIndex, TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = GameWorldUiView::getKeyTextureAsset(keyIndex);
	const TextureAsset paletteTextureAsset = GameWorldUiView::getPaletteTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrashFormat("Couldn't create UI texture for key %d.", keyIndex);
	}

	return textureID;
}

UiTextureID GameWorldUiView::allocContainerInventoryTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = GameWorldUiView::getContainerInventoryTextureAsset();
	const TextureAsset paletteTextureAsset = GameWorldUiView::getPaletteTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for container inventory.");
	}

	return textureID;
}

// @temp: keep until 3D-DDA ray casting is fully correct (i.e. entire ground is red dots for
// levels where ceilingScale < 1.0, and same with ceiling blue dots).
// @todo: As of SDL 2.0.10 which introduced batching, this now behaves like the color is per frame, not per call, which isn't correct, and flushing doesn't help.
void GameWorldUiView::DEBUG_ColorRaycastPixel(Game &game)
{
	const Window &window = game.window;
	auto &renderer = game.renderer;
	const int selectionDim = 3;
	const Int2 windowDims = window.getPixelDimensions();

	constexpr int xOffset = 16;
	constexpr int yOffset = 16;

	const auto &gameState = game.gameState;
	if (!gameState.isActiveMapValid())
	{
		return;
	}

	const auto &player = game.player;
	const CoordDouble3 rayStart = player.getEyeCoord();
	const Double3 &cameraDirection = player.forward;
	const double viewAspectRatio = window.getSceneViewAspectRatio();

	const double ceilingScale = gameState.getActiveCeilingScale();
	const SceneManager &sceneManager = game.sceneManager;
	const VoxelChunkManager &voxelChunkManager = sceneManager.voxelChunkManager;
	const EntityChunkManager &entityChunkManager = sceneManager.entityChunkManager;
	const CollisionChunkManager &collisionChunkManager = sceneManager.collisionChunkManager;

	for (int y = 0; y < windowDims.y; y += yOffset)
	{
		for (int x = 0; x < windowDims.x; x += xOffset)
		{
			const Double3 rayDirection = GameWorldUiModel::screenToWorldRayDirection(game, Int2(x, y));

			// Not registering entities with ray cast hits for efficiency since this debug visualization is for voxels.
			constexpr bool includeEntities = false;
			RayCastHit hit;
			const bool success = Physics::rayCast(rayStart, rayDirection, ceilingScale, cameraDirection,
				includeEntities, voxelChunkManager, entityChunkManager, collisionChunkManager,
				EntityDefinitionLibrary::getInstance(), hit);

			if (success)
			{
				Color color;
				switch (hit.type)
				{
				case RayCastHitType::Voxel:
				{
					constexpr Color colors[] = { Colors::Red, Colors::Green, Colors::Blue, Colors::Cyan, Colors::Yellow };
					const RayCastVoxelHit &voxelHit = hit.voxelHit;
					const VoxelInt3 voxel = voxelHit.voxelCoord.voxel;
					const int colorsIndex = std::clamp<int>(voxel.y, 0, std::size(colors) - 1);
					DebugAssertIndex(colors, colorsIndex);
					color = colors[colorsIndex];
					break;
				}
				case RayCastHitType::Entity:
				{
					color = Colors::Yellow;
					break;
				}
				}

				DebugNotImplemented();
				//renderer.drawRect(color, x, y, selectionDim, selectionDim);
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

	const Options &options = game.options;
	const Player &player = game.player;
	const Double3 &cameraDirection = player.forward;

	const Window &window = game.window;
	const Int2 viewDims = window.getSceneViewDimensions();
	const Int2 viewCenterPoint(viewDims.x / 2, viewDims.y / 2);

	const CoordDouble3 rayStart = player.getEyeCoord();
	const VoxelDouble3 rayDirection = GameWorldUiModel::screenToWorldRayDirection(game, viewCenterPoint);

	const SceneManager &sceneManager = game.sceneManager;
	const VoxelChunkManager &voxelChunkManager = sceneManager.voxelChunkManager;
	const EntityChunkManager &entityChunkManager = sceneManager.entityChunkManager;
	const CollisionChunkManager &collisionChunkManager = sceneManager.collisionChunkManager;

	const GameState &gameState = game.gameState;
	const double ceilingScale = gameState.getActiveCeilingScale();

	EntityDefinitionLibrary &entityDefLibrary = EntityDefinitionLibrary::getInstance();

	constexpr bool includeEntities = true;
	RayCastHit hit;
	const bool success = Physics::rayCast(rayStart, rayDirection, ceilingScale, cameraDirection, includeEntities,
		voxelChunkManager, entityChunkManager, collisionChunkManager, entityDefLibrary, hit);

	std::string text;
	if (success)
	{
		switch (hit.type)
		{
		case RayCastHitType::Voxel:
		{
			const RayCastVoxelHit &voxelHit = hit.voxelHit;
			const ChunkInt2 chunkPos = voxelHit.voxelCoord.chunk;
			const VoxelInt3 voxel = voxelHit.voxelCoord.voxel;

			const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
			const VoxelTraitsDefID voxelTraitsDefID = voxelChunk.traitsDefIDs.get(voxel.x, voxel.y, voxel.z);
			const VoxelTraitsDefinition &voxelTraitsDef = voxelChunk.traitsDefs[voxelTraitsDefID];

			text = "Voxel: (" + voxel.toString() + "), " + std::to_string(static_cast<int>(voxelTraitsDef.type)) + ' ' + std::to_string(hit.t);
			break;
		}
		case RayCastHitType::Entity:
		{
			const RayCastEntityHit &entityHit = hit.entityHit;
			const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();

			// Try inspecting the entity (can be from any distance). If they have a display name, then show it.
			const EntityInstance &entityInst = entityChunkManager.getEntity(entityHit.id);
			const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityInst.defID);
			const auto &charClassLibrary = CharacterClassLibrary::getInstance();

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

			text.append(' ' + std::to_string(hit.t));
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

	Renderer &renderer = game.renderer;

	const TextBoxInitInfo textBoxInitInfo = TextBoxInitInfo::makeWithXY(
		text,
		0,
		0,
		ArenaFontName::Arena,
		Colors::White,
		TextAlignment::TopLeft,
		FontLibrary::getInstance());

	TextBox textBox;
	if (!textBox.init(textBoxInitInfo, text, renderer))
	{
		DebugCrash("Couldn't init physics ray cast text box.");
	}

	const int originalX = ArenaRenderUtils::SCREEN_WIDTH / 2;
	const int originalY = (ArenaRenderUtils::SCREEN_HEIGHT / 2) + 10;
	DebugNotImplemented(); // Disabled for now until I need it again
	//renderer.drawOriginal(textBox.getTextureID(), originalX, originalY);
}

DebugVoxelVisibilityQuadtreeState GameWorldUiView::allocDebugVoxelVisibilityQuadtreeState(Renderer &renderer)
{
	DebugVoxelVisibilityQuadtreeState state;

	for (int treeLevelIndex = 0; treeLevelIndex < VoxelFrustumCullingChunk::TREE_LEVEL_COUNT; treeLevelIndex++)
	{
		DebugAssertIndex(VoxelFrustumCullingChunk::NODES_PER_SIDE, treeLevelIndex);
		const int quadtreeTextureDim = VoxelFrustumCullingChunk::NODES_PER_SIDE[treeLevelIndex];
		const UiTextureID quadtreeTextureID = renderer.createUiTexture(quadtreeTextureDim, quadtreeTextureDim);
		if (quadtreeTextureID < 0)
		{
			DebugLogErrorFormat("Couldn't allocate voxel visibility quadtree debug texture %d.", treeLevelIndex);
			continue;
		}

		state.textureIDs[treeLevelIndex] = quadtreeTextureID;

		const Int2 textureDims = *renderer.tryGetUiTextureDims(quadtreeTextureID);
		state.textureDimsList[treeLevelIndex] = Int2(textureDims.x, textureDims.y);
	}

	for (int treeLevelIndex = 0; treeLevelIndex < VoxelFrustumCullingChunk::TREE_LEVEL_COUNT; treeLevelIndex++)
	{
		int quadtreeDrawPositionY = 0;
		for (int i = treeLevelIndex; i < VoxelFrustumCullingChunk::TREE_LEVEL_INDEX_LEAF; i++)
		{
			const int yDimIndex = VoxelFrustumCullingChunk::TREE_LEVEL_INDEX_LEAF - (i - treeLevelIndex);
			DebugAssertIndex(state.textureDimsList, yDimIndex);
			quadtreeDrawPositionY += state.textureDimsList[yDimIndex].y;
		}

		state.drawPositionYs[treeLevelIndex] = quadtreeDrawPositionY;
	}

	return state;
}
