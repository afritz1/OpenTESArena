#include "SDL.h"

#include "WorldMapPanel.h"
#include "WorldMapUiController.h"
#include "WorldMapUiModel.h"
#include "WorldMapUiView.h"
#include "../Game/Game.h"
#include "../UI/CursorData.h"

#include "components/debug/Debug.h"

WorldMapPanel::WorldMapPanel(Game &game)
	: Panel(game) { }

bool WorldMapPanel::init()
{
	this->backToGameButton = Button<Game&>(
		WorldMapUiView::BackToGameButtonCenterPoint,
		WorldMapUiView::BackToGameButtonWidth,
		WorldMapUiView::BackToGameButtonHeight,
		WorldMapUiController::onBackToGameButtonSelected);
	this->provinceButton = Button<Game&, int>(WorldMapUiController::onProvinceButtonSelected);

	// Load province name offsets.
	auto &game = this->getGame();
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

void WorldMapPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	const bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	const bool mPressed = inputManager.keyPressed(e, SDLK_m);

	if (escapePressed || mPressed)
	{
		this->backToGameButton.click(this->getGame());
	}

	const bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 originalPoint = this->getGame().getRenderer().nativeToOriginal(mousePosition);

		// Listen for clicks on the map and exit button.
		const auto &worldMapMasks = this->getGame().getBinaryAssetLibrary().getWorldMapMasks();
		const int maskCount = static_cast<int>(worldMapMasks.size());
		for (int maskID = 0; maskID < maskCount; maskID++)
		{
			const WorldMapMask &mapMask = worldMapMasks.at(maskID);
			const Rect &maskRect = mapMask.getRect();

			if (maskRect.contains(originalPoint))
			{
				// See if the clicked pixel is set in the bitmask.
				const bool success = mapMask.get(originalPoint.x, originalPoint.y);

				if (success)
				{
					// Mask IDs 0 through 8 are provinces, and 9 is the "Exit" button.
					if (maskID < 9)
					{
						// Go to the selected province panel.
						this->provinceButton.click(this->getGame(), maskID);
					}
					else
					{
						// Exit the world map panel.
						this->backToGameButton.click(this->getGame());
					}
					
					break;
				}
			}
		}
	}	
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
