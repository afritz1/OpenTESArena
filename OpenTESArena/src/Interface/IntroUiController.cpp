#include "CinematicPanel.h"
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
