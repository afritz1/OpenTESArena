#include "CommonUiView.h"
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

	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();

	const TextureAssetReference paletteTextureAssetRef = WorldMapUiView::getFastTravelPaletteTextureAssetRef();
	const std::string animFilename = WorldMapUiView::getFastTravelAnimationFilename();

	const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(animFilename.c_str());
	if (!metadataID.has_value())
	{
		DebugLogError("Couldn't get texture file metadata for \"" + animFilename + "\".");
		return false;
	}

	const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);
	DebugAssert(textureFileMetadata.getTextureCount() > 0);

	this->animTextureRefs.init(textureFileMetadata.getTextureCount());
	for (int i = 0; i < textureFileMetadata.getTextureCount(); i++)
	{
		const TextureAssetReference textureAssetRef = TextureAssetReference(std::string(animFilename), i);

		UiTextureID textureID;
		if (!TextureUtils::tryAllocUiTexture(textureAssetRef, paletteTextureAssetRef, textureManager, renderer, &textureID))
		{
			DebugLogError("Couldn't create UI texture for sequence \"" + animFilename + "\" frame " + std::to_string(i) + ".");
			return false;
		}

		this->animTextureRefs.set(i, ScopedUiTextureRef(textureID, renderer));
	}

	UiDrawCall::TextureFunc animTextureFunc = [this]()
	{
		const ScopedUiTextureRef &textureRef = this->animTextureRefs.get(this->frameIndex);
		return textureRef.get();
	};

	this->addDrawCall(
		animTextureFunc,
		WorldMapUiView::getFastTravelAnimationTextureCenter(),
		Int2(textureFileMetadata.getWidth(0), textureFileMetadata.getHeight(0)),
		PivotType::Middle);

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), CommonUiView::DefaultCursorPivotType);

	return true;
}

void FastTravelSubPanel::tick(double dt)
{
	auto &game = this->getGame();

	// Update horse animation.
	this->currentSeconds += dt;
	while (this->currentSeconds >= WorldMapUiView::FastTravelAnimationSecondsPerFrame)
	{
		this->currentSeconds -= WorldMapUiView::FastTravelAnimationSecondsPerFrame;
		this->frameIndex++;

		if (this->frameIndex == this->animTextureRefs.getCount())
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
