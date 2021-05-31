#include "CinematicPanel.h"
#include "ImagePanel.h"
#include "IntroUiController.h"
#include "IntroUiModel.h"
#include "IntroUiView.h"
#include "MainMenuPanel.h"
#include "../Game/Game.h"

std::unique_ptr<Panel> IntroUiModel::makeStartupPanel(Game &game)
{
	// If not showing the intro, then jump to the main menu.
	if (!game.getOptions().getMisc_ShowIntro())
	{
		return std::make_unique<MainMenuPanel>(game);
	}

	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	const bool isFloppyVersion = exeData.isFloppyVersion();
	if (isFloppyVersion)
	{
		const TextureAssetReference paletteTextureAssetRef = IntroUiView::getIntroTitlePaletteTextureAssetReference();
		const TextureAssetReference textureAssetRef = IntroUiView::getIntroTitleTextureAssetReference();
		return std::make_unique<ImagePanel>(
			game,
			paletteTextureAssetRef.filename,
			textureAssetRef.filename,
			IntroUiView::IntroTitleSeconds,
			IntroUiController::onIntroTitleFinished);
	}
	else
	{
		return std::make_unique<CinematicPanel>(
			game,
			IntroUiView::getIntroBookPaletteFilename(),
			IntroUiView::getIntroBookSequenceFilename(),
			1.0 / IntroUiView::IntroBookFramesPerSecond,
			IntroUiController::onIntroBookFinished);
	}
}
