#include "MainMenuUiState.h"
#include "MainMenuUiView.h"
#include "../Game/Game.h"
#include "../Rendering/Renderer.h"

MainMenuUiState::MainMenuUiState()
{
	this->bgTextureID = -1;
	this->testArrowsTextureID = -1;
	this->testButtonTextureID = -1;
	this->testType = -1;
	this->testIndex = -1;
	this->testIndex2 = -1;
	this->testWeather = -1;
}

void MainMenuUiState::allocate(UiManager &uiManager, TextureManager &textureManager, Renderer &renderer)
{
	this->bgTextureID = MainMenuUiView::allocBackgroundTexture(textureManager, renderer);
	this->testArrowsTextureID = MainMenuUiView::allocTestArrowsTexture(textureManager, renderer);
	this->testButtonTextureID = MainMenuUiView::allocTestButtonTexture(textureManager, renderer);
}

void MainMenuUiState::free(UiManager &uiManager, Renderer &renderer)
{
	this->elements.free(uiManager);

	if (this->bgTextureID >= 0)
	{
		renderer.freeUiTexture(this->bgTextureID);
		this->bgTextureID = -1;
	}

	if (this->testArrowsTextureID >= 0)
	{
		renderer.freeUiTexture(this->testArrowsTextureID);
		this->testArrowsTextureID = -1;
	}

	if (this->testButtonTextureID >= 0)
	{
		renderer.freeUiTexture(this->testButtonTextureID);
		this->testButtonTextureID = -1;
	}
}

void MainMenuUI::create(Game &game)
{
	UiManager &uiManager = game.uiManager;
	MainMenuUiState &state = MainMenuUI::state;
	state.allocate(uiManager, game.textureManager, game.renderer);

	UiElementInitInfo bgImageElementInitInfo;
	bgImageElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	bgImageElementInitInfo.contextType = UiContextType::MainMenu;
	const UiElementInstanceID bgImageElementInstID = uiManager.createImage(bgImageElementInitInfo, state.bgTextureID);
	state.elements.imageElementInstIDs.emplace_back(bgImageElementInstID);

	state.testType = 0;
	state.testIndex = 0;
	state.testIndex2 = 1;
	state.testWeather = 0;

	const UiTextureID cursorTextureID = game.defaultCursorTextureID;

	const Renderer &renderer = game.renderer;
	const std::optional<Int2> dims = renderer.tryGetUiTextureDims(cursorTextureID);
	DebugAssert(dims.has_value());

	const Options &options = game.options;
	const double scale = options.getGraphics_CursorScale();
	const Int2 cursorSize(
		static_cast<int>(static_cast<double>(dims->x) * scale),
		static_cast<int>(static_cast<double>(dims->y) * scale));
	uiManager.setTransformSize(game.cursorImageElementInstID, cursorSize);
	uiManager.setImageTexture(game.cursorImageElementInstID, cursorTextureID);
}

void MainMenuUI::destroy(Game &game)
{
	MainMenuUiState &state = MainMenuUI::state;
	state.free(game.uiManager, game.renderer);

	state.testType = -1;
	state.testIndex = -1;
	state.testIndex2 = -1;
	state.testWeather = -1;
}
