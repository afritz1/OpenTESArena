#include "CinematicPanel.h"
#include "CinematicUiState.h"
#include "ImagePanel.h"
#include "IntroUiController.h"
#include "IntroUiModel.h"
#include "IntroUiView.h"
#include "MainMenuPanel.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Game/Game.h"

std::unique_ptr<Panel> IntroUiModel::makeStartupPanel(Game &game)
{
	// If not showing the intro, then jump to the main menu.
	if (!game.options.getMisc_ShowIntro())
	{
		std::unique_ptr<MainMenuPanel> panel = std::make_unique<MainMenuPanel>(game);
		if (!panel->init())
		{
			DebugLogError("Couldn't init start-up MainMenuPanel.");
			return nullptr;
		}

		return panel;
	}

	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	const bool isFloppyVersion = exeData.isFloppyVersion;
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
		const std::string paletteFilename = IntroUiView::getIntroBookPaletteFilename();
		const std::string sequenceFilename = IntroUiView::getIntroBookSequenceFilename();

		TextureManager &textureManager = game.textureManager;
		const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(sequenceFilename.c_str());
		if (!metadataID.has_value())
		{
			DebugLogErrorFormat("Couldn't get texture file metadata for start-up cinematic \"%s\".", sequenceFilename.c_str());
			return nullptr;
		}

		const TextureFileMetadata &metadata = textureManager.getMetadataHandle(*metadataID);

		CinematicUiInitInfo &cinematicInitInfo = CinematicUI::state.initInfo;
		cinematicInitInfo.paletteName = paletteFilename;
		cinematicInitInfo.sequenceName = sequenceFilename;
		cinematicInitInfo.secondsPerImage = metadata.getSecondsPerFrame();
		cinematicInitInfo.callback = [&game]() { IntroUiController::onIntroBookFinished(game); };

		std::unique_ptr<CinematicPanel> panel = std::make_unique<CinematicPanel>(game);
		if (!panel->init())
		{
			DebugLogError("Couldn't init start-up CinematicPanel.");
			return nullptr;
		}

		return panel;
	}
}
