#include "AutomapUiController.h"
#include "AutomapUiModel.h"
#include "AutomapUiState.h"
#include "AutomapUiView.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"

namespace
{
	constexpr char AutomapTextureElementName[] = "AutomapTexture";
}

AutomapUiState::AutomapUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	this->mapTextureID = -1;
	this->cursorTextureID = -1;
}

void AutomapUiState::init(Game &game)
{
	this->game = &game;

	const Player &player = game.player;
	const CoordDouble3 playerCoord = player.getEyeCoord();
	const VoxelInt2 playerVoxelXZ = VoxelUtils::pointToVoxel(playerCoord.point.getXZ());
	const CoordInt2 playerCoordXZ(playerCoord.chunk, playerVoxelXZ);
	this->automapOffset = AutomapUiModel::makeAutomapOffset(playerCoordXZ.voxel);

	const SceneManager &sceneManager = game.sceneManager;
	const VoxelChunkManager &voxelChunkManager = sceneManager.voxelChunkManager;
	Renderer &renderer = game.renderer;
	this->mapTextureID = AutomapUiView::allocMapTexture(game.gameState, playerCoordXZ, player.getGroundDirectionXZ(), voxelChunkManager, renderer);

	TextureManager &textureManager = game.textureManager;
	this->cursorTextureID = AutomapUiView::allocCursorTexture(textureManager, renderer);
}

void AutomapUI::create(Game &game)
{
	AutomapUiState &state = AutomapUI::state;
	state.init(game);

	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(AutomapUI::ContextName);
	state.contextInstID = uiManager.createContext(contextDef, inputManager, textureManager, renderer);

	uiManager.addMouseButtonHeldListener(AutomapUI::onMouseButtonHeld, state.contextInstID, inputManager);

	UiElementInitInfo mapImageElementInitInfo;
	mapImageElementInitInfo.name = AutomapTextureElementName;
	mapImageElementInitInfo.drawOrder = 1;
	mapImageElementInitInfo.clipRect = AutomapUiView::DrawingArea;
	uiManager.createImage(mapImageElementInitInfo, state.mapTextureID, state.contextInstID);

	GameState &gameState = game.gameState;
	const LocationDefinition &locationDef = gameState.getLocationDefinition();
	const LocationInstance &locationInst = gameState.getLocationInstance();
	const SceneManager &sceneManager = game.sceneManager;
	const LocationDefinitionType locationDefType = locationDef.getType();
	const bool isCity = locationDefType == LocationDefinitionType::City;
	const bool isMainQuestDungeon = locationDefType == LocationDefinitionType::MainQuestDungeon;

	// Some places like named/wild dungeons don't display a name on the automap.
	const bool shouldDisplayLocationName = isCity || isMainQuestDungeon;

	std::string automapLocationName;
	if (shouldDisplayLocationName)
	{
		automapLocationName = locationInst.getName(locationDef);
	}

	const UiElementInstanceID locationTextBoxElementInstID = uiManager.getElementByName("AutomapLocationTextBox");
	uiManager.setTextBoxText(locationTextBoxElementInstID, automapLocationName.c_str());
	 
	const std::optional<Int2> cursorDims = renderer.tryGetUiTextureDims(state.cursorTextureID);
	DebugAssert(cursorDims.has_value());

	const Options &options = game.options;
	const double cursorScale = options.getGraphics_CursorScale();
	const Int2 cursorSize(
		static_cast<int>(static_cast<double>(cursorDims->x) * cursorScale),
		static_cast<int>(static_cast<double>(cursorDims->y) * cursorScale));

	uiManager.setTransformSize(game.cursorImageElementInstID, cursorSize);
	uiManager.setTransformPivot(game.cursorImageElementInstID, UiPivotType::BottomLeft);
	uiManager.setImageTexture(game.cursorImageElementInstID, state.cursorTextureID);

	inputManager.setInputActionMapActive(InputActionMapName::Automap, true);
}

void AutomapUI::destroy()
{
	AutomapUiState &state = AutomapUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	Renderer &renderer = game.renderer;

	if (state.contextInstID >= 0)
	{
		uiManager.freeContext(state.contextInstID, inputManager, renderer);
		state.contextInstID = -1;
	}

	if (state.mapTextureID >= 0)
	{
		renderer.freeUiTexture(state.mapTextureID);
		state.mapTextureID = -1;
	}

	if (state.cursorTextureID >= 0)
	{
		renderer.freeUiTexture(state.cursorTextureID);
		state.cursorTextureID = -1;
	}

	// @todo need to pop some stack instead of assuming top left, or just have game.setCursorToDefault(), or just never do anything about the cursor on context destroy
	uiManager.setTransformPivot(game.cursorImageElementInstID, UiPivotType::TopLeft);
	// @todo set cursor size back to arrow cursor * cursorScale
	uiManager.setImageTexture(game.cursorImageElementInstID, game.defaultCursorTextureID);

	inputManager.setInputActionMapActive(InputActionMapName::Automap, false);
}

void AutomapUI::update(double dt)
{
	const AutomapUiState &state = AutomapUI::state;

	constexpr double pixelSizeReal = static_cast<double>(AutomapUiView::PixelSize);
	const int offsetX = static_cast<int>(std::floor(state.automapOffset.x * pixelSizeReal));
	const int offsetY = static_cast<int>(std::floor(state.automapOffset.y * pixelSizeReal));

	constexpr Rect drawingArea = AutomapUiView::DrawingArea;
	const int mapX = (drawingArea.getLeft() + (drawingArea.width / 2)) + offsetX;
	const int mapY = (drawingArea.getTop() + (drawingArea.height / 2)) + offsetY;
	const Int2 mapPosition(mapX, mapY);

	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID mapImageElementInstID = uiManager.getElementByName(AutomapTextureElementName);
	uiManager.setTransformPosition(mapImageElementInstID, mapPosition);
}

void AutomapUI::onMouseButtonHeld(Game &game, MouseButtonType buttonType, const Int2 &position, double dt)
{
	AutomapUiState &state = AutomapUI::state;
	AutomapUiController::onMouseButtonHeld(game, buttonType, position, dt, &state.automapOffset);
}

void AutomapUI::onExitButtonSelected(MouseButtonType mouseButtonType)
{
	AutomapUiState &state = AutomapUI::state;
	AutomapUiController::onBackToGameButtonSelected(*state.game);
}

void AutomapUI::onExitInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		AutomapUI::onExitButtonSelected(MouseButtonType::Left);
	}
}
