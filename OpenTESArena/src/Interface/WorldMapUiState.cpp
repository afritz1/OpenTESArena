#include "GameWorldUiState.h"
#include "ProvinceMapUiState.h"
#include "WorldMapUiController.h"
#include "WorldMapUiModel.h"
#include "WorldMapUiState.h"
#include "WorldMapUiView.h"
#include "../Assets/WorldMapMask.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"

namespace
{
	constexpr char ElementName_FastTravelPopUpImage[] = "WorldMapFastTravelPopUpImage";

	Buffer<UiTextureID> AllocFastTravelAnimTextureIDs(TextureManager &textureManager, Renderer &renderer)
	{
		const TextureAsset paletteTextureAsset = FastTravelUiView::getPaletteTextureAsset();
		const std::string animFilename = FastTravelUiView::getAnimationFilename();

		const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(animFilename.c_str());
		if (!metadataID.has_value())
		{
			DebugLogErrorFormat("Couldn't get texture file metadata for fast travel animation \"%s\".", animFilename.c_str());
			return Buffer<UiTextureID>();
		}

		const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);
		const int textureCount = textureFileMetadata.getTextureCount();
		if (textureCount == 0)
		{
			DebugLogErrorFormat("No textures in fast travel animation \"%s\".", animFilename.c_str());
			return Buffer<UiTextureID>();
		}

		Buffer<UiTextureID> animTextureIDs(textureCount);
		animTextureIDs.fill(-1);

		for (int i = 0; i < textureCount; i++)
		{
			const TextureAsset textureAsset = TextureAsset(std::string(animFilename), i);

			UiTextureID textureID;
			if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
			{
				DebugLogErrorFormat("Couldn't create UI texture for fast travel animation \"%s\" frame %d.", animFilename.c_str(), i);
				continue;
			}

			animTextureIDs.set(i, textureID);
		}

		return animTextureIDs;
	}
}

WorldMapUiState::WorldMapUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	//this->fastTravelContextInstID = -1;
	this->highlightedProvinceTextureID = -1;

	this->isFastTravelling = false;
	this->fastTravelCurrentSeconds = 0.0;
	this->fastTravelTotalSeconds = 0.0;
	this->fastTravelTargetSeconds = 0.0;
	this->fastTravelAnimFrameIndex = -1;
}

void WorldMapUiState::init(Game &game)
{
	this->game = &game;

	const GameState &gameState = game.gameState;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	const int provinceID = gameState.getProvinceDefinition().getRaceID();
	this->highlightedProvinceTextureID = WorldMapUiView::allocHighlightedTextTexture(provinceID, textureManager, renderer);

	this->fastTravelAnimTextureIDs = AllocFastTravelAnimTextureIDs(textureManager, renderer);
	this->fastTravelCurrentSeconds = 0.0;
	this->fastTravelTotalSeconds = 0.0;
	this->fastTravelTargetSeconds = 0.0;
	this->fastTravelAnimFrameIndex = 0;

	// Fast travel bool is set ahead of time by province map logic.
	if (this->isFastTravelling)
	{
		const ProvinceMapUiModel::TravelData *travelData = gameState.getTravelData();
		DebugAssert(travelData != nullptr);
		this->fastTravelTargetSeconds = std::max(FastTravelUiModel::AnimationMinSeconds, static_cast<double>(travelData->travelDays) * FastTravelUiView::AnimationSecondsPerFrame);
	}
}

void WorldMapUiState::freeTextures(Renderer &renderer)
{
	if (this->highlightedProvinceTextureID >= 0)
	{
		renderer.freeUiTexture(this->highlightedProvinceTextureID);
		this->highlightedProvinceTextureID = -1;
	}

	if (this->fastTravelAnimTextureIDs.isValid())
	{
		for (UiTextureID textureID : this->fastTravelAnimTextureIDs)
		{
			renderer.freeUiTexture(textureID);
		}

		this->fastTravelAnimTextureIDs.clear();
	}
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

	/*UiContextInitInfo fastTravelContextInitInfo;
	fastTravelContextInitInfo.name = "WorldMapFastTravelPopUp";
	fastTravelContextInitInfo.drawOrder = 1;
	state.fastTravelContextInstID = uiManager.createContext(fastTravelContextInitInfo);*/

	const GameState &gameState = game.gameState;
	const int provinceID = gameState.getProvinceDefinition().getRaceID();

	UiElementInitInfo highlightedProvinceImageElementInitInfo;
	highlightedProvinceImageElementInitInfo.name = "WorldMapHighlightedProvinceImage";
	highlightedProvinceImageElementInitInfo.position = WorldMapUiView::getProvinceNameOffset(provinceID, textureManager);
	highlightedProvinceImageElementInitInfo.drawOrder = 1;
	uiManager.createImage(highlightedProvinceImageElementInitInfo, state.highlightedProvinceTextureID, state.contextInstID, renderer);

	uiManager.addMouseButtonChangedListener(WorldMapUI::onMouseButtonChanged, WorldMapUI::ContextName, inputManager);

	UiElementInitInfo fastTravelImageElementInitInfo;
	fastTravelImageElementInitInfo.name = ElementName_FastTravelPopUpImage;
	fastTravelImageElementInitInfo.position = FastTravelUiView::getAnimationTextureCenter();
	fastTravelImageElementInitInfo.pivotType = UiPivotType::Middle;
	fastTravelImageElementInitInfo.drawOrder = 2;
	const UiElementInstanceID fastTravelImageElementInstID = uiManager.createImage(fastTravelImageElementInitInfo, state.fastTravelAnimTextureIDs[0], state.contextInstID, renderer);

	if (!state.isFastTravelling)
	{
		uiManager.setElementActive(fastTravelImageElementInstID, false);
	}

	game.setCursorOverride(std::nullopt);

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

	/*if (state.fastTravelContextInstID >= 0)
	{
		uiManager.freeContext(state.fastTravelContextInstID, inputManager, renderer);
		state.fastTravelContextInstID = -1;
	}*/

	state.freeTextures(renderer);
	state.isFastTravelling = false;

	inputManager.setInputActionMapActive(InputActionMapName::WorldMap, false);
}

void WorldMapUI::update(double dt)
{
	WorldMapUiState &state = WorldMapUI::state;
	if (!state.isFastTravelling)
	{
		return;
	}

	Game &game = *state.game;
	const GameState &gameState = game.gameState;
	const ProvinceMapUiModel::TravelData *travelData = gameState.getTravelData();
	DebugAssert(travelData != nullptr);

	state.fastTravelCurrentSeconds += dt;
	while (state.fastTravelCurrentSeconds >= FastTravelUiView::AnimationSecondsPerFrame)
	{
		state.fastTravelCurrentSeconds -= FastTravelUiView::AnimationSecondsPerFrame;
		state.fastTravelAnimFrameIndex++;

		if (state.fastTravelAnimFrameIndex == state.fastTravelAnimTextureIDs.getCount())
		{
			state.fastTravelAnimFrameIndex = 0;
		}
	}

	state.fastTravelTotalSeconds += dt;
	if (state.fastTravelTotalSeconds >= state.fastTravelTargetSeconds)
	{
		FastTravelUiController::onAnimationFinished(game, travelData->provinceID, travelData->locationID, travelData->travelDays);
	}

	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID fastTravelAnimImageElementInstID = uiManager.getElementByName(ElementName_FastTravelPopUpImage);
	const UiTextureID newFastTravelAnimTextureID = state.fastTravelAnimTextureIDs[state.fastTravelAnimFrameIndex];
	uiManager.setImageTexture(fastTravelAnimImageElementInstID, newFastTravelAnimTextureID);
}

void WorldMapUI::onMouseButtonChanged(Game &game, MouseButtonType type, const Int2 &position, bool pressed)
{
	const WorldMapUiState &state = WorldMapUI::state;
	if (state.isFastTravelling)
	{
		return;
	}

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
							game.setNextContext(ProvinceMapUI::ContextName);
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
	if (state.isFastTravelling)
	{
		return;
	}

	Game &game = *state.game;
	GameState &gameState = game.gameState;
	gameState.setTravelData(std::nullopt);

	game.setNextContext(GameWorldUI::ContextName);
}

void WorldMapUI::onBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		WorldMapUI::onBackButtonSelected(MouseButtonType::Left);
	}
}
