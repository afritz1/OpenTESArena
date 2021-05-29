#include "GameWorldPanel.h"
#include "GameWorldUiView.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/CFAFile.h"
#include "../Assets/CIFFile.h"
#include "../Game/Game.h"

Int2 GameWorldUiView::getStatusPopUpTextCenterPoint(Game &game)
{
	return GameWorldUiView::getInterfaceCenter(game);
}

int GameWorldUiView::getStatusPopUpTextureWidth(int textWidth)
{
	return textWidth + 12;
}

int GameWorldUiView::getStatusPopUpTextureHeight(int textHeight)
{
	return textHeight + 12;
}

Int2 GameWorldUiView::getCurrentWeaponAnimationOffset(Game &game)
{
	const auto &weaponAnimation = game.getGameState().getPlayer().getWeaponAnimation();
	const std::string &weaponFilename = weaponAnimation.getAnimationFilename();
	const int weaponAnimIndex = weaponAnimation.getFrameIndex();

	// @todo: add XY offset support to TextureFileMetadata so it can be cached by texture manager.

	if (!weaponAnimation.isRanged())
	{
		// Melee weapon offsets.
		CIFFile cifFile;
		if (!cifFile.init(weaponFilename.c_str()))
		{
			DebugCrash("Couldn't init .CIF file \"" + weaponFilename + "\".");
		}

		return Int2(cifFile.getXOffset(weaponAnimIndex), cifFile.getYOffset(weaponAnimIndex));
	}
	else
	{
		// Ranged weapon offsets.
		CFAFile cfaFile;
		if (!cfaFile.init(weaponFilename.c_str()))
		{
			DebugCrash("Couldn't init .CFA file \"" + weaponFilename + "\".");
		}

		return Int2(cfaFile.getXOffset(), cfaFile.getYOffset());
	}
}

Int2 GameWorldUiView::getInterfaceCenter(Game &game)
{
	const bool modernInterface = game.getOptions().getGraphics_ModernInterface();
	if (modernInterface)
	{
		return Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, ArenaRenderUtils::SCREEN_HEIGHT / 2);
	}
	else
	{
		TextureManager &textureManager = game.getTextureManager();
		const TextureBuilderID textureBuilderID = GameWorldUiView::getGameWorldInterfaceTextureBuilderID(textureManager);
		const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(textureBuilderID);

		return Int2(
			ArenaRenderUtils::SCREEN_WIDTH / 2,
			(ArenaRenderUtils::SCREEN_HEIGHT - textureBuilder.getHeight()) / 2);
	}
}

TextureBuilderID GameWorldUiView::getGameWorldInterfaceTextureBuilderID(TextureManager &textureManager)
{
	const std::string &textureFilename = ArenaTextureName::GameWorldInterface;
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureFilename.c_str());
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + textureFilename + "\".");
	}

	return *textureBuilderID;
}

TextureBuilderID GameWorldUiView::getCompassFrameTextureBuilderID(Game &game)
{
	auto &textureManager = game.getTextureManager();
	const std::string &textureFilename = ArenaTextureName::CompassFrame;
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureFilename.c_str());
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + textureFilename + "\".");
	}

	return *textureBuilderID;
}

TextureBuilderID GameWorldUiView::getCompassSliderTextureBuilderID(Game &game)
{
	auto &textureManager = game.getTextureManager();
	const std::string &textureFilename = ArenaTextureName::CompassSlider;
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureFilename.c_str());
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + textureFilename + "\".");
	}

	return *textureBuilderID;
}

TextureBuilderID GameWorldUiView::getPlayerPortraitTextureBuilderID(Game &game,
	const std::string &portraitsFilename, int portraitID)
{
	auto &textureManager = game.getTextureManager();
	const std::optional<TextureBuilderIdGroup> textureBuilderIDs = textureManager.tryGetTextureBuilderIDs(portraitsFilename.c_str());
	if (!textureBuilderIDs.has_value())
	{
		DebugCrash("Couldn't get texture builder IDs for \"" + portraitsFilename + "\".");
	}

	return textureBuilderIDs->getID(portraitID);
}

TextureBuilderID GameWorldUiView::getStatusGradientTextureBuilderID(Game &game, int gradientID)
{
	auto &textureManager = game.getTextureManager();
	const std::string &statusGradientsFilename = ArenaTextureName::StatusGradients;
	const std::optional<TextureBuilderIdGroup> textureBuilderIDs = textureManager.tryGetTextureBuilderIDs(statusGradientsFilename.c_str());
	if (!textureBuilderIDs.has_value())
	{
		DebugCrash("Couldn't get texture builder IDs for \"" + statusGradientsFilename + "\".");
	}

	return textureBuilderIDs->getID(gradientID);
}

TextureBuilderID GameWorldUiView::getNoSpellTextureBuilderID(Game &game)
{
	auto &textureManager = game.getTextureManager();
	const std::string &textureFilename = ArenaTextureName::NoSpell;
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureFilename.c_str());
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + textureFilename + "\".");
	}

	return *textureBuilderID;
}

TextureBuilderID GameWorldUiView::getWeaponTextureBuilderID(Game &game, const std::string &weaponFilename, int index)
{
	auto &textureManager = game.getTextureManager();
	const std::optional<TextureBuilderIdGroup> textureBuilderIDs = textureManager.tryGetTextureBuilderIDs(weaponFilename.c_str());
	if (!textureBuilderIDs.has_value())
	{
		DebugCrash("Couldn't get texture builder IDs for \"" + weaponFilename + "\".");
	}

	return textureBuilderIDs->getID(index);
}
