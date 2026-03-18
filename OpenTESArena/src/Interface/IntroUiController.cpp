#include "CinematicPanel.h"
#include "CinematicUiState.h"
#include "ImagePanel.h"
#include "ImageSequencePanel.h"
#include "IntroUiController.h"
#include "IntroUiView.h"
#include "MainMenuPanel.h"
#include "../Game/Game.h"

void IntroUiController::onIntroBookFinished(Game &game)
{
	const TextureAsset paletteTextureAsset = IntroUiView::getIntroTitlePaletteTextureAsset();
	const TextureAsset textureAsset = IntroUiView::getIntroTitleTextureAsset();
	game.setPanel<ImagePanel>(
		paletteTextureAsset.filename,
		textureAsset.filename,
		IntroUiView::IntroTitleSeconds,
		IntroUiController::onIntroTitleFinished);
}

void IntroUiController::onIntroTitleFinished(Game &game)
{
	const TextureAsset paletteTextureAsset = IntroUiView::getIntroQuotePaletteTextureAsset();
	const TextureAsset textureAsset = IntroUiView::getIntroQuoteTextureAsset();
	game.setPanel<ImagePanel>(
		paletteTextureAsset.filename,
		textureAsset.filename,
		IntroUiView::IntroQuoteSeconds,
		IntroUiController::onIntroQuoteFinished);
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
	cinematicInitInfo.paletteName = paletteFilename;
	cinematicInitInfo.sequenceName = sequenceFilename;
	cinematicInitInfo.secondsPerImage = metadata.getSecondsPerFrame();
	cinematicInitInfo.callback = [&game]() { IntroUiController::onOpeningScrollFinished(game); };

	game.setPanel<CinematicPanel>();
}

void IntroUiController::onOpeningScrollFinished(Game &game)
{
	game.setPanel<ImageSequencePanel>(
		IntroUiView::getIntroStoryPaletteNames(),
		IntroUiView::getIntroStoryTextureNames(),
		IntroUiView::getIntroStoryImageDurations(),
		IntroUiController::onIntroStoryFinished);
}

void IntroUiController::onIntroStoryFinished(Game &game)
{
	game.setPanel<MainMenuPanel>();
}
