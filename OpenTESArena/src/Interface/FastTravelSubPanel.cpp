#include <algorithm>

#include "FastTravelSubPanel.h"
#include "WorldMapUiController.h"
#include "WorldMapUiModel.h"
#include "WorldMapUiView.h"
#include "../Game/Game.h"
#include "../UI/CursorData.h"

FastTravelSubPanel::FastTravelSubPanel(Game &game)
	: Panel(game) { }

bool FastTravelSubPanel::init()
{
	this->currentSeconds = 0.0;
	this->totalSeconds = 0.0;

	// Determine how long the animation should run until switching to the game world.
	auto &game = this->getGame();
	const auto &gameState = game.getGameState();
	const ProvinceMapUiModel::TravelData *travelDataPtr = gameState.getTravelData();
	DebugAssert(travelDataPtr != nullptr);
	const ProvinceMapUiModel::TravelData &travelData = *travelDataPtr;
	this->targetSeconds = std::max(WorldMapUiModel::FastTravelAnimationMinSeconds,
		static_cast<double>(travelData.travelDays) * WorldMapUiView::FastTravelAnimationSecondsPerFrame);

	this->frameIndex = 0;

	return true;
}

std::optional<CursorData> FastTravelSubPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void FastTravelSubPanel::tick(double dt)
{
	auto &game = this->getGame();
	auto &textureManager = game.getTextureManager();
	const std::string animFilename = WorldMapUiView::getFastTravelAnimationFilename();
	const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(animFilename.c_str());
	if (!metadataID.has_value())
	{
		DebugLogError("Couldn't get texture file metadata for fast travel animation \"" + animFilename + "\".");
		return;
	}

	const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);

	// Update horse animation.
	this->currentSeconds += dt;
	while (this->currentSeconds >= WorldMapUiView::FastTravelAnimationSecondsPerFrame)
	{
		this->currentSeconds -= WorldMapUiView::FastTravelAnimationSecondsPerFrame;
		this->frameIndex++;

		if (this->frameIndex == textureFileMetadata.getTextureCount())
		{
			this->frameIndex = 0;
		}
	}

	// Update total seconds and see if the animation should be done.
	this->totalSeconds += dt;
	if (this->totalSeconds >= this->targetSeconds)
	{
		const auto &gameState = game.getGameState();
		const ProvinceMapUiModel::TravelData *travelDataPtr = gameState.getTravelData();
		DebugAssert(travelDataPtr != nullptr);
		const ProvinceMapUiModel::TravelData &travelData = *travelDataPtr;

		WorldMapUiController::onFastTravelAnimationFinished(game, travelData.provinceID,
			travelData.locationID, travelData.travelDays);
	}
}

void FastTravelSubPanel::render(Renderer &renderer)
{
	// Draw horse animation.
	auto &game = this->getGame();
	auto &textureManager = game.getTextureManager();
	const TextureAssetReference paletteTextureAssetRef = WorldMapUiView::getFastTravelPaletteTextureAssetRef();
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteTextureAssetRef);
	if (!paletteID.has_value())
	{
		DebugLogError("Couldn't get palette ID for \"" + paletteTextureAssetRef.filename + "\".");
		return;
	}

	const std::string textureFilename = WorldMapUiView::getFastTravelAnimationFilename();
	const std::optional<TextureBuilderIdGroup> textureBuilderIDs =
		textureManager.tryGetTextureBuilderIDs(textureFilename.c_str());
	if (!textureBuilderIDs.has_value())
	{
		DebugCrash("Couldn't get texture builder IDs for \"" + textureFilename + "\".");
	}

	const TextureBuilderID textureBuilderID = textureBuilderIDs->getID(static_cast<int>(this->frameIndex));
	const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(textureBuilderID);
	const int x = WorldMapUiView::getFastTravelAnimationTextureX(textureBuilder.getWidth());
	const int y = WorldMapUiView::getFastTravelAnimationTextureY(textureBuilder.getHeight());
	renderer.drawOriginal(textureBuilderID, *paletteID, x, y, textureManager);
}
