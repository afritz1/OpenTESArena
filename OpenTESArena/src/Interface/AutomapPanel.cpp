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
#include "../Game/CardinalDirection.h"
#include "../Game/CardinalDirectionName.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Input/InputActionMapName.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../UI/CursorAlignment.h"
#include "../UI/CursorData.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"
#include "../UI/TextRenderUtils.h"
#include "../Utilities/Color.h"
#include "../Voxels/VoxelFacing2D.h"
#include "../World/ArenaWildUtils.h"
#include "../World/MapType.h"

#include "components/debug/Debug.h"

AutomapPanel::AutomapPanel(Game &game)
	: Panel(game) { }

AutomapPanel::~AutomapPanel()
{
	auto &game = this->getGame();
	auto &inputManager = game.getInputManager();
	inputManager.setInputActionMapActive(InputActionMapName::Automap, false);
}

bool AutomapPanel::init(const CoordDouble3 &playerCoord, const VoxelDouble2 &playerDirection,
	const ChunkManager &chunkManager, const std::string &locationName)
{
	auto &game = this->getGame();
	
	const auto &fontLibrary = game.getFontLibrary();
	const TextBox::InitInfo locationTextBoxInitInfo = AutomapUiView::getLocationTextBoxInitInfo(locationName, fontLibrary);
	if (!this->locationTextBox.init(locationTextBoxInitInfo, game.getRenderer()))
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

	auto &inputManager = game.getInputManager();
	inputManager.setInputActionMapActive(InputActionMapName::Automap, true);

	auto backToGameInputActionFunc = AutomapUiController::onBackToGameInputAction;
	this->addInputActionListener(AutomapUiController::getInputActionName(), backToGameInputActionFunc);
	this->addInputActionListener(AutomapUiController::getBackToGameInputActionName(), backToGameInputActionFunc);

	this->addMouseButtonHeldListener(
		[this](Game &game, MouseButtonType buttonType, const Int2 &position, double dt)
	{
		AutomapUiController::onMouseButtonHeld(game, buttonType, position, dt, &this->automapOffset);
	});

	auto &renderer = game.getRenderer();
	const VoxelInt3 playerVoxel = VoxelUtils::pointToVoxel(playerCoord.point);
	const CoordInt2 playerCoordXZ(playerCoord.chunk, VoxelInt2(playerVoxel.x, playerVoxel.z));
	const UiTextureID mapTextureID = AutomapUiView::allocMapTexture(
		game.getGameState(), playerCoordXZ, playerDirection, chunkManager, renderer);
	this->mapTextureRef.init(mapTextureID, renderer);

	auto &textureManager = game.getTextureManager();
	const UiTextureID backgroundTextureID = AutomapUiView::allocBgTexture(textureManager, renderer);
	this->backgroundTextureRef.init(backgroundTextureID, renderer);

	this->addDrawCall(
		this->backgroundTextureRef.get(),
		Int2::Zero,
		Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT),
		PivotType::TopLeft);

	UiDrawCall::TextureFunc automapTextureFunc = [this]()
	{
		return this->mapTextureRef.get();
	};

	UiDrawCall::PositionFunc automapPositionFunc = [this, &game]()
	{
		constexpr double pixelSizeReal = static_cast<double>(AutomapUiView::PixelSize);
		const int offsetX = static_cast<int>(std::floor(this->automapOffset.x * pixelSizeReal));
		const int offsetY = static_cast<int>(std::floor(this->automapOffset.y * pixelSizeReal));
		
		const Rect &drawingArea = AutomapUiView::DrawingArea;
		const int mapX = (drawingArea.getLeft() + (drawingArea.getWidth() / 2)) + offsetX;
		const int mapY = (drawingArea.getTop() + (drawingArea.getHeight() / 2)) + offsetY;
		return Int2(mapX, mapY);
	};

	UiDrawCall::SizeFunc automapSizeFunc = [this, &game]()
	{
		auto &renderer = game.getRenderer();
		const std::optional<Int2> dims = renderer.tryGetUiTextureDims(this->mapTextureRef.get());
		if (!dims.has_value())
		{
			DebugCrash("Couldn't get automap texture dimensions.");
		}

		return *dims;
	};

	UiDrawCall::PivotFunc automapPivotFunc = []()
	{
		return PivotType::TopLeft;
	};

	const std::optional<Rect> automapClipRect = AutomapUiView::DrawingArea;

	this->addDrawCall(
		automapTextureFunc,
		automapPositionFunc,
		automapSizeFunc,
		automapPivotFunc,
		UiDrawCall::defaultActiveFunc,
		automapClipRect);

	const Rect &locationTextBoxRect = this->locationTextBox.getRect();
	this->addDrawCall(
		this->locationTextBox.getTextureID(),
		locationTextBoxRect.getTopLeft(),
		Int2(locationTextBoxRect.getWidth(), locationTextBoxRect.getHeight()),
		PivotType::TopLeft);

	const UiTextureID cursorTextureID = AutomapUiView::allocCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), PivotType::BottomLeft);
	
	this->automapOffset = AutomapUiModel::makeAutomapOffset(playerCoordXZ.voxel);
	return true;
}
