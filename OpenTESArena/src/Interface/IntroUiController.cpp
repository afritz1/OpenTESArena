#include "CinematicUiState.h"
#include "ImageUiState.h"
#include "ImageSequenceUiState.h"
#include "IntroUiController.h"
#include "IntroUiView.h"
#include "MainMenuUiState.h"
#include "../Game/Game.h"

void IntroUiController::onIntroBookFinished(Game &game)
{
	const TextureAsset paletteTextureAsset = IntroUiView::getIntroTitlePaletteTextureAsset();
	const TextureAsset textureAsset = IntroUiView::getIntroTitleTextureAsset();

	ImageUiInitInfo &imageInitInfo = ImageUI::state.initInfo;
	imageInitInfo.init(paletteTextureAsset.filename, textureAsset.filename, IntroUiView::IntroTitleSeconds, [&game]() { IntroUiController::onIntroTitleFinished(game); });
	game.setNextContext(ImageUI::ContextName);
}

void IntroUiController::onIntroTitleFinished(Game &game)
{
	const TextureAsset paletteTextureAsset = IntroUiView::getIntroQuotePaletteTextureAsset();
	const TextureAsset textureAsset = IntroUiView::getIntroQuoteTextureAsset();

	ImageUiInitInfo &imageInitInfo = ImageUI::state.initInfo;
	imageInitInfo.init(paletteTextureAsset.filename, textureAsset.filename, IntroUiView::IntroQuoteSeconds, [&game]() { IntroUiController::onIntroQuoteFinished(game); });
	game.setNextContext(ImageUI::ContextName);
}

void IntroUiController::onIntroQuoteFinished(Game &game)
{
	const std::string paletteFilename = IntroUiView::getOpeningScrollPaletteFilename();
	const std::string sequenceFilename = IntroUiView::getOpeningScrollSequenceFilename();

	TextureManager &textureManager = game.textureManager;
	const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(sequenceFilename.c_str());
	if (!metadataID.has_value())
	{
		DebugLogErrorFormat("Couldn't get texture file metadata for opening scroll animation \"%s\".", sequenceFilename.c_str());
		return;
	}

	const TextureFileMetadata &metadata = textureManager.getMetadataHandle(*metadataID);

	CinematicUiInitInfo &cinematicInitInfo = CinematicUI::state.initInfo;
	cinematicInitInfo.init(paletteFilename, sequenceFilename, metadata.getSecondsPerFrame(), [&game]() { IntroUiController::onOpeningScrollFinished(game); });
	game.setNextContext(CinematicUI::ContextName);
}

void IntroUiController::onOpeningScrollFinished(Game &game)
{
	ImageSequenceUiInitInfo &imageSequenceInitInfo = ImageSequenceUI::state.initInfo;
	imageSequenceInitInfo.init(IntroUiView::getIntroStoryPaletteNames(), IntroUiView::getIntroStoryTextureNames(), IntroUiView::getIntroStoryImageDurations(), [&game]() { IntroUiController::onIntroStoryFinished(game); });
	game.setNextContext(ImageSequenceUI::ContextName);
}

void IntroUiController::onIntroStoryFinished(Game &game)
{
	game.setNextContext(MainMenuUI::ContextName);
}
