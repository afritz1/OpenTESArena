#include "CommonUiView.h"
#include "FastTravelSubPanel.h"
#include "WorldMapUiController.h"
#include "WorldMapUiModel.h"
#include "WorldMapUiView.h"
#include "../Game/Game.h"

FastTravelSubPanel::FastTravelSubPanel(Game &game)
	: Panel(game) { }

bool FastTravelSubPanel::init()
{
	this->currentSeconds = 0.0;
	this->totalSeconds = 0.0;

	// Determine how long the animation should run until switching to the game world.
	auto &game = this->getGame();
	const auto &gameState = game.gameState;
	const ProvinceMapUiModel::TravelData *travelDataPtr = gameState.getTravelData();
	DebugAssert(travelDataPtr != nullptr);
	const ProvinceMapUiModel::TravelData &travelData = *travelDataPtr;
	this->targetSeconds = std::max(FastTravelUiModel::AnimationMinSeconds,
		static_cast<double>(travelData.travelDays) * FastTravelUiView::AnimationSecondsPerFrame);

	this->frameIndex = 0;

	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;

	const TextureAsset paletteTextureAsset = FastTravelUiView::getPaletteTextureAsset();
	const std::string animFilename = FastTravelUiView::getAnimationFilename();

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
		const TextureAsset textureAsset = TextureAsset(std::string(animFilename), i);

		UiTextureID textureID;
		if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
		{
			DebugLogError("Couldn't create UI texture for sequence \"" + animFilename + "\" frame " + std::to_string(i) + ".");
			return false;
		}

		this->animTextureRefs.set(i, ScopedUiTextureRef(textureID, renderer));
	}

	UiDrawCallInitInfo drawCallInitInfo;
	drawCallInitInfo.textureFunc = [this]()
	{
		const ScopedUiTextureRef &textureRef = this->animTextureRefs.get(this->frameIndex);
		return textureRef.get();
	};

	drawCallInitInfo.position = FastTravelUiView::getAnimationTextureCenter();
	drawCallInitInfo.size = Int2(textureFileMetadata.getWidth(0), textureFileMetadata.getHeight(0));
	drawCallInitInfo.pivotType = UiPivotType::Middle;
	this->addDrawCall(drawCallInitInfo);

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
	while (this->currentSeconds >= FastTravelUiView::AnimationSecondsPerFrame)
	{
		this->currentSeconds -= FastTravelUiView::AnimationSecondsPerFrame;
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
		const auto &gameState = game.gameState;
		const ProvinceMapUiModel::TravelData *travelDataPtr = gameState.getTravelData();
		DebugAssert(travelDataPtr != nullptr);
		const ProvinceMapUiModel::TravelData &travelData = *travelDataPtr;

		FastTravelUiController::onAnimationFinished(game, travelData.provinceID,
			travelData.locationID, travelData.travelDays);
	}
}
