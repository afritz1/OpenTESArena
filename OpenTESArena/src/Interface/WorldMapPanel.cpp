#include "SDL.h"

#include "WorldMapPanel.h"
#include "WorldMapUiController.h"
#include "WorldMapUiModel.h"
#include "WorldMapUiView.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/CursorData.h"

#include "components/debug/Debug.h"

WorldMapPanel::WorldMapPanel(Game &game)
	: Panel(game) { }

WorldMapPanel::~WorldMapPanel()
{
	auto &inputManager = this->getGame().getInputManager();
	inputManager.setInputActionMapActive(InputActionMapName::WorldMap, false);
}

bool WorldMapPanel::init()
{
	auto &game = this->getGame();
	auto &inputManager = game.getInputManager();
	inputManager.setInputActionMapActive(InputActionMapName::WorldMap, true);

	const Rect fullscreenRect(
		0,
		0,
		ArenaRenderUtils::SCREEN_WIDTH,
		ArenaRenderUtils::SCREEN_HEIGHT);

	auto backToGameFunc = WorldMapUiController::onBackToGameButtonSelected;

	this->addButtonProxy(MouseButtonType::Left, fullscreenRect,
		[this, &game, backToGameFunc]()
	{
		const auto &inputManager = game.getInputManager();
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 classicPosition = game.getRenderer().nativeToOriginal(mousePosition);

		for (int i = 0; i < WorldMapUiModel::MASK_COUNT; i++)
		{
			const WorldMapMask &mask = WorldMapUiModel::getMask(game, i);
			const Rect &maskRect = mask.getRect();
			if (maskRect.contains(classicPosition))
			{
				const bool success = mask.get(classicPosition.x, classicPosition.y);

				if (success)
				{
					if (i < WorldMapUiModel::EXIT_BUTTON_MASK_ID)
					{
						WorldMapUiController::onProvinceButtonSelected(game, i);
					}
					else
					{
						backToGameFunc(game);
					}

					break;
				}
			}
		}
	});

	auto backToGameInputActionFunc = [backToGameFunc](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			backToGameFunc(values.game);
		}
	};

	this->addInputActionListener(InputActionName::Back, backToGameInputActionFunc);
	this->addInputActionListener(InputActionName::WorldMap, backToGameInputActionFunc);

	// Load province name offsets.
	auto &textureManager = game.getTextureManager();
	const std::string provinceNameOffsetFilename = WorldMapUiModel::getProvinceNameOffsetFilename();
	const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(provinceNameOffsetFilename.c_str());
	if (!metadataID.has_value())
	{
		DebugLogError("Couldn't get texture file metadata for \"" + provinceNameOffsetFilename + "\".");
		return false;
	}

	const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);
	this->provinceNameOffsets.init(textureFileMetadata.getTextureCount());
	for (int i = 0; i < textureFileMetadata.getTextureCount(); i++)
	{
		this->provinceNameOffsets.set(i, textureFileMetadata.getOffset(i));
	}

	return true;
}

std::optional<CursorData> WorldMapPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void WorldMapPanel::render(Renderer &renderer)
{
	DebugAssert(this->getGame().gameStateIsActive());

	// Clear full screen.
	renderer.clear();

	auto &textureManager = this->getGame().getTextureManager();
	const TextureAssetReference worldMapPaletteTextureAssetRef = WorldMapUiView::getWorldMapPaletteTextureAssetReference();
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(worldMapPaletteTextureAssetRef);
	if (!paletteID.has_value())
	{
		DebugLogError("Couldn't get palette ID for \"" + worldMapPaletteTextureAssetRef.filename + "\".");
		return;
	}

	// Draw world map background. This one has "Exit" at the bottom right.
	const TextureAssetReference worldMapTextureAssetRef = WorldMapUiView::getWorldMapTextureAssetReference();
	const std::optional<TextureBuilderID> mapBackgroundTextureBuilderID =
		textureManager.tryGetTextureBuilderID(worldMapTextureAssetRef);
	if (!mapBackgroundTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get map background texture builder ID for \"" + worldMapTextureAssetRef.filename + "\".");
		return;
	}
	
	renderer.drawOriginal(*mapBackgroundTextureBuilderID, *paletteID, textureManager);

	// Draw yellow text over current province name.
	const std::string provinceNamesFilename = WorldMapUiView::getProvinceNamesFilename();
	const std::optional<TextureBuilderIdGroup> provinceTextTextureBuilderIDs =
		textureManager.tryGetTextureBuilderIDs(provinceNamesFilename.c_str());
	if (!provinceTextTextureBuilderIDs.has_value())
	{
		DebugLogError("Couldn't get province text texture builder IDs for \"" + provinceNamesFilename + "\".");
		return;
	}

	const auto &gameState = this->getGame().getGameState();
	const int provinceID = gameState.getProvinceDefinition().getRaceID();
	const TextureBuilderID provinceTextTextureBuilderID = provinceTextTextureBuilderIDs->getID(provinceID);
	const Int2 &nameOffset = this->provinceNameOffsets.get(provinceID);
	renderer.drawOriginal(provinceTextTextureBuilderID, *paletteID, nameOffset.x, nameOffset.y, textureManager);
}
