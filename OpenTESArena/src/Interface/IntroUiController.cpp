#include "CinematicPanel.h"
#include "ImagePanel.h"
#include "ImageSequencePanel.h"
#include "IntroUiController.h"
#include "IntroUiView.h"
#include "MainMenuPanel.h"
#include "../Game/Game.h"

void IntroUiController::onIntroBookFinished(Game &game)
{
	const TextureAssetReference paletteTextureAssetRef = IntroUiView::getIntroTitlePaletteTextureAssetReference();
	const TextureAssetReference textureAssetRef = IntroUiView::getIntroTitleTextureAssetReference();
	game.setPanel<ImagePanel>(
		paletteTextureAssetRef.filename,
		textureAssetRef.filename,
		IntroUiView::IntroTitleSeconds,
		IntroUiController::onIntroTitleFinished);
}

void IntroUiController::onIntroTitleFinished(Game &game)
{
	const TextureAssetReference paletteTextureAssetRef = IntroUiView::getIntroQuotePaletteTextureAssetReference();
	const TextureAssetReference textureAssetRef = IntroUiView::getIntroQuoteTextureAssetReference();
	game.setPanel<ImagePanel>(
		paletteTextureAssetRef.filename,
		textureAssetRef.filename,
		IntroUiView::IntroQuoteSeconds,
		IntroUiController::onIntroQuoteFinished);
}

void IntroUiController::onIntroQuoteFinished(Game &game)
{
	game.setPanel<CinematicPanel>(
		IntroUiView::getOpeningScrollPaletteFilename(),
		IntroUiView::getOpeningScrollSequenceFilename(),
		1.0 / IntroUiView::OpeningScrollFramesPerSecond,
		IntroUiController::onOpeningScrollFinished);
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
