#include "CommonUiView.h"
#include "WorldMapPanel.h"
#include "WorldMapUiController.h"
#include "WorldMapUiModel.h"
#include "WorldMapUiView.h"
#include "../Assets/WorldMapMask.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../Rendering/ArenaRenderUtils.h"

#include "components/debug/Debug.h"

WorldMapPanel::WorldMapPanel(Game &game)
	: Panel(game) { }

WorldMapPanel::~WorldMapPanel()
{
	auto &inputManager = this->getGame().inputManager;
	inputManager.setInputActionMapActive(InputActionMapName::WorldMap, false);
}

bool WorldMapPanel::init()
{
	auto &game = this->getGame();
	auto &inputManager = game.inputManager;
	inputManager.setInputActionMapActive(InputActionMapName::WorldMap, true);

	const Rect fullscreenRect(
		0,
		0,
		ArenaRenderUtils::SCREEN_WIDTH,
		ArenaRenderUtils::SCREEN_HEIGHT);

	auto backToGameFunc = WorldMapUiController::onBackToGameButtonSelected;

	this->addButtonProxy(MouseButtonType::Left, fullscreenRect,
		[this, &game, backToGameFunc]()
	{
		const auto &inputManager = game.inputManager;
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 classicPosition = game.window.nativeToOriginal(mousePosition);

		for (int i = 0; i < WorldMapUiModel::MASK_COUNT; i++)
		{
			const WorldMapMask &mask = WorldMapUiModel::getMask(game, i);
			const Rect &maskRect = mask.getRect();
			if (maskRect.contains(classicPosition))
			{
				const bool success = mask.get(classicPosition.x, classicPosition.y);

				if (success)
				{
					if (i < WorldMapUiModel::EXIT_BUTTON_MASK_ID)
					{
						WorldMapUiController::onProvinceButtonSelected(game, i);
					}
					else
					{
						backToGameFunc(game);
					}

					break;
				}
			}
		}
	});

	auto backToGameInputActionFunc = [backToGameFunc](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			backToGameFunc(values.game);
		}
	};

	this->addInputActionListener(InputActionName::Back, backToGameInputActionFunc);
	this->addInputActionListener(InputActionName::WorldMap, backToGameInputActionFunc);

	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;

	const UiTextureID backgroundTextureID = WorldMapUiView::allocBackgroundTexture(textureManager, renderer);
	this->backgroundTextureRef.init(backgroundTextureID, renderer);

	UiDrawCallInitInfo bgDrawCallInitInfo;
	bgDrawCallInitInfo.textureID = this->backgroundTextureRef.get();
	bgDrawCallInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	this->addDrawCall(bgDrawCallInitInfo);

	const auto &gameState = game.gameState;
	const int provinceID = gameState.getProvinceDefinition().getRaceID();
	const Int2 provinceNameOffset = WorldMapUiView::getProvinceNameOffset(provinceID, textureManager);
	const UiTextureID highlightedTextTextureID = WorldMapUiView::allocHighlightedTextTexture(provinceID, textureManager, renderer);
	this->highlightedTextTextureRef.init(highlightedTextTextureID, renderer);

	UiDrawCallInitInfo highlightedTextDrawCallInitInfo;
	highlightedTextDrawCallInitInfo.textureID = this->highlightedTextTextureRef.get();
	highlightedTextDrawCallInitInfo.position = provinceNameOffset;
	highlightedTextDrawCallInitInfo.size = this->highlightedTextTextureRef.getDimensions();
	this->addDrawCall(highlightedTextDrawCallInitInfo);

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), CommonUiView::DefaultCursorPivotType);

	return true;
}
