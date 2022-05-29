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
		std::unique_ptr<MainMenuPanel> panel = std::make_unique<MainMenuPanel>(game);
		if (!panel->init())
		{
			DebugLogError("Couldn't init start-up MainMenuPanel.");
			return nullptr;
		}

		return panel;
	}

	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	const bool isFloppyVersion = exeData.isFloppyVersion();
	if (isFloppyVersion)
	{
		const TextureAsset paletteTextureAsset = IntroUiView::getIntroTitlePaletteTextureAsset();
		const TextureAsset textureAsset = IntroUiView::getIntroTitleTextureAsset();
		std::unique_ptr<ImagePanel> panel = std::make_unique<ImagePanel>(game);
		if (!panel->init(paletteTextureAsset.filename, textureAsset.filename, IntroUiView::IntroTitleSeconds,
			IntroUiController::onIntroTitleFinished))
		{
			DebugLogError("Couldn't init start-up ImagePanel.");
			return nullptr;
		}

		return panel;
	}
	else
	{
		std::unique_ptr<CinematicPanel> panel = std::make_unique<CinematicPanel>(game);
		if (!panel->init(IntroUiView::getIntroBookPaletteFilename(), IntroUiView::getIntroBookSequenceFilename(),
			1.0 / IntroUiView::IntroBookFramesPerSecond, IntroUiController::onIntroBookFinished))
		{
			DebugLogError("Couldn't init start-up CinematicPanel.");
			return nullptr;
		}

		return panel;
	}
}
