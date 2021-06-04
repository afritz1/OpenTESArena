#include "SDL.h"

#include "WorldMapPanel.h"
#include "WorldMapUiController.h"
#include "WorldMapUiModel.h"
#include "WorldMapUiView.h"
#include "../Assets/CIFFile.h"
#include "../Game/Game.h"

#include "components/debug/Debug.h"

WorldMapPanel::WorldMapPanel(Game &game, std::unique_ptr<ProvinceMapUiModel::TravelData> travelData)
	: Panel(game), travelData(std::move(travelData))
{
	this->backToGameButton = Button<Game&>(
		WorldMapUiView::BackToGameButtonCenterPoint,
		WorldMapUiView::BackToGameButtonWidth,
		WorldMapUiView::BackToGameButtonHeight,
		WorldMapUiController::onBackToGameButtonSelected);
	this->provinceButton = Button<Game&, int, std::unique_ptr<ProvinceMapUiModel::TravelData>>(
		WorldMapUiController::onProvinceButtonSelected);

	// Load province name offsets.
	// @todo: TextureFileMetadata support
	const std::string cifName = WorldMapUiModel::getProvinceNameOffsetFilename();
	CIFFile cif;
	if (!cif.init(cifName.c_str()))
	{
		DebugCrash("Could not init .CIF file \"" + cifName + "\".");
	}

	for (int i = 0; i < static_cast<int>(this->provinceNameOffsets.size()); i++)
	{
		this->provinceNameOffsets[i] = Int2(cif.getXOffset(i), cif.getYOffset(i));
	}
}

std::optional<Panel::CursorData> WorldMapPanel::getCurrentCursor() const
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
						this->provinceButton.click(this->getGame(), maskID, std::move(this->travelData));
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
	const Int2 &nameOffset = this->provinceNameOffsets.at(provinceID);
	renderer.drawOriginal(provinceTextTextureBuilderID, *paletteID, nameOffset.x, nameOffset.y, textureManager);
}
