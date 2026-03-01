#include "GameWorldPanel.h"
#include "ProvinceMapPanel.h"
#include "ProvinceMapUiState.h"
#include "WorldMapUiController.h"
#include "WorldMapUiModel.h"
#include "WorldMapUiState.h"
#include "WorldMapUiView.h"
#include "../Assets/WorldMapMask.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"

WorldMapUiState::WorldMapUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	this->highlightedProvinceTextureID = -1;
}

void WorldMapUiState::init(Game &game)
{
	this->game = &game;

	const GameState &gameState = game.gameState;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	const int provinceID = gameState.getProvinceDefinition().getRaceID();
	this->highlightedProvinceTextureID = WorldMapUiView::allocHighlightedTextTexture(provinceID, textureManager, renderer);
}

void WorldMapUI::create(Game &game)
{
	WorldMapUiState &state = WorldMapUI::state;
	state.init(game);

	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(WorldMapUI::ContextName);
	state.contextInstID = uiManager.createContext(contextDef, inputManager, textureManager, renderer);

	const GameState &gameState = game.gameState;
	const int provinceID = gameState.getProvinceDefinition().getRaceID();

	UiElementInitInfo highlightedProvinceImageElementInitInfo;
	highlightedProvinceImageElementInitInfo.name = "WorldMapHighlightedProvinceImage";
	highlightedProvinceImageElementInitInfo.position = WorldMapUiView::getProvinceNameOffset(provinceID, textureManager);
	highlightedProvinceImageElementInitInfo.drawOrder = 1;
	uiManager.createImage(highlightedProvinceImageElementInitInfo, state.highlightedProvinceTextureID, state.contextInstID, renderer);

	uiManager.addMouseButtonChangedListener(WorldMapUI::onMouseButtonChanged, WorldMapUI::ContextName, inputManager);

	const UiTextureID cursorTextureID = game.defaultCursorTextureID;
	const std::optional<Int2> cursorDims = renderer.tryGetUiTextureDims(cursorTextureID);
	DebugAssert(cursorDims.has_value());

	const Options &options = game.options;
	const double cursorScale = options.getGraphics_CursorScale();
	const Int2 cursorSize(
		static_cast<int>(static_cast<double>(cursorDims->x) * cursorScale),
		static_cast<int>(static_cast<double>(cursorDims->y) * cursorScale));
	uiManager.setTransformSize(game.cursorImageElementInstID, cursorSize);
	uiManager.setTransformPivot(game.cursorImageElementInstID, UiPivotType::TopLeft);
	uiManager.setImageTexture(game.cursorImageElementInstID, cursorTextureID);	

	inputManager.setInputActionMapActive(InputActionMapName::WorldMap, true);
}

void WorldMapUI::destroy()
{
	WorldMapUiState &state = WorldMapUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	Renderer &renderer = game.renderer;

	if (state.contextInstID >= 0)
	{
		uiManager.freeContext(state.contextInstID, inputManager, renderer);
		state.contextInstID = -1;
	}

	if (state.highlightedProvinceTextureID >= 0)
	{
		renderer.freeUiTexture(state.highlightedProvinceTextureID);
		state.highlightedProvinceTextureID = -1;
	}

	inputManager.setInputActionMapActive(InputActionMapName::WorldMap, false);
}

void WorldMapUI::update(double dt)
{
	// Do nothing.
}

void WorldMapUI::onMouseButtonChanged(Game &game, MouseButtonType type, const Int2 &position, bool pressed)
{
	if (type == MouseButtonType::Left)
	{
		if (pressed)
		{
			const InputManager &inputManager = game.inputManager;
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
							ProvinceMapUI::state.provinceID = i;
							game.setPanel<ProvinceMapPanel>();
						}
						else
						{
							WorldMapUI::onBackButtonSelected(type);
						}

						break;
					}
				}
			}
		}
	}
}

void WorldMapUI::onBackButtonSelected(MouseButtonType mouseButtonType)
{
	const WorldMapUiState &state = WorldMapUI::state;
	Game &game = *state.game;
	
	GameState &gameState = game.gameState;
	gameState.setTravelData(std::nullopt);

	game.setPanel<GameWorldPanel>();
}

void WorldMapUI::onBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		WorldMapUI::onBackButtonSelected(MouseButtonType::Left);
	}
}
