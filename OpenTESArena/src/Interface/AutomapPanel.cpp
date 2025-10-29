#include <cmath>
#include <unordered_map>
#include <vector>

#include "AutomapPanel.h"
#include "AutomapUiController.h"
#include "AutomapUiModel.h"
#include "AutomapUiView.h"
#include "GameWorldPanel.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/TextureManager.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Input/InputActionMapName.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"
#include "../UI/TextRenderUtils.h"
#include "../Utilities/Color.h"
#include "../World/ArenaWildUtils.h"
#include "../World/MapType.h"

#include "components/debug/Debug.h"

AutomapPanel::AutomapPanel(Game &game)
	: Panel(game) { }

AutomapPanel::~AutomapPanel()
{
	auto &game = this->getGame();
	auto &inputManager = game.inputManager;
	inputManager.setInputActionMapActive(InputActionMapName::Automap, false);
}

bool AutomapPanel::init(const CoordDouble3 &playerCoord, const VoxelDouble2 &playerDirection,
	const VoxelChunkManager &voxelChunkManager, const std::string &locationName)
{
	auto &game = this->getGame();
	
	const auto &fontLibrary = FontLibrary::getInstance();
	const TextBoxInitInfo locationTextBoxInitInfo = AutomapUiView::getLocationTextBoxInitInfo(locationName, fontLibrary);
	if (!this->locationTextBox.init(locationTextBoxInitInfo, game.renderer))
	{
		DebugLogError("Couldn't init location text box.");
		return false;
	}

	this->locationTextBox.setText(locationName);

	this->backToGameButton = Button<Game&>(
		AutomapUiView::BackToGameButtonCenterPoint,
		AutomapUiView::BackToGameButtonWidth,
		AutomapUiView::BackToGameButtonHeight,
		AutomapUiController::onBackToGameButtonSelected);

	this->addButtonProxy(MouseButtonType::Left, this->backToGameButton.getRect(),
		[&game]() { AutomapUiController::onBackToGameButtonSelected(game); });

	auto &inputManager = game.inputManager;
	inputManager.setInputActionMapActive(InputActionMapName::Automap, true);

	auto backToGameInputActionFunc = AutomapUiController::onBackToGameInputAction;
	this->addInputActionListener(AutomapUiController::getInputActionName(), backToGameInputActionFunc);
	this->addInputActionListener(AutomapUiController::getBackToGameInputActionName(), backToGameInputActionFunc);

	this->addMouseButtonHeldListener(
		[this](Game &game, MouseButtonType buttonType, const Int2 &position, double dt)
	{
		AutomapUiController::onMouseButtonHeld(game, buttonType, position, dt, &this->automapOffset);
	});

	const GameState &gameState = game.gameState;
	const double ceilingScale = gameState.getActiveCeilingScale();
	const VoxelInt2 playerVoxelXZ = VoxelUtils::pointToVoxel(playerCoord.point.getXZ());
	const CoordInt2 playerCoordXZ(playerCoord.chunk, playerVoxelXZ);
	
	Renderer &renderer = game.renderer;
	const UiTextureID mapTextureID = AutomapUiView::allocMapTexture(gameState, playerCoordXZ, playerDirection, voxelChunkManager, renderer);
	this->mapTextureRef.init(mapTextureID, renderer);

	TextureManager &textureManager = game.textureManager;
	const UiTextureID backgroundTextureID = AutomapUiView::allocBgTexture(textureManager, renderer);
	this->backgroundTextureRef.init(backgroundTextureID, renderer);

	UiDrawCallInitInfo bgDrawCallInitInfo;
	bgDrawCallInitInfo.textureID = this->backgroundTextureRef.get();
	bgDrawCallInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	this->addDrawCall(bgDrawCallInitInfo);

	UiDrawCallInitInfo automapDrawCallInitInfo;
	automapDrawCallInitInfo.textureID = this->mapTextureRef.get();
	automapDrawCallInitInfo.positionFunc = [this, &game]()
	{
		constexpr double pixelSizeReal = static_cast<double>(AutomapUiView::PixelSize);
		const int offsetX = static_cast<int>(std::floor(this->automapOffset.x * pixelSizeReal));
		const int offsetY = static_cast<int>(std::floor(this->automapOffset.y * pixelSizeReal));
		
		constexpr Rect drawingArea = AutomapUiView::DrawingArea;
		const int mapX = (drawingArea.getLeft() + (drawingArea.width / 2)) + offsetX;
		const int mapY = (drawingArea.getTop() + (drawingArea.height / 2)) + offsetY;
		return Int2(mapX, mapY);
	};

	automapDrawCallInitInfo.sizeFunc = [this, &game]()
	{
		auto &renderer = game.renderer;
		const std::optional<Int2> dims = renderer.tryGetUiTextureDims(this->mapTextureRef.get());
		if (!dims.has_value())
		{
			DebugCrash("Couldn't get automap texture dimensions.");
		}

		return *dims;
	};

	automapDrawCallInitInfo.clipRect = AutomapUiView::DrawingArea;
	this->addDrawCall(automapDrawCallInitInfo);

	const Rect locationTextBoxRect = this->locationTextBox.getRect();
	UiDrawCallInitInfo locationTextDrawCallInitInfo;
	locationTextDrawCallInitInfo.textureID = this->locationTextBox.getTextureID();
	locationTextDrawCallInitInfo.position = locationTextBoxRect.getTopLeft();
	locationTextDrawCallInitInfo.size = locationTextBoxRect.getSize();
	this->addDrawCall(locationTextDrawCallInitInfo);

	const UiTextureID cursorTextureID = AutomapUiView::allocCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), PivotType::BottomLeft);
	
	this->automapOffset = AutomapUiModel::makeAutomapOffset(playerCoordXZ.voxel);
	return true;
}
