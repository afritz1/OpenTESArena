#include "AutomapUiMVC.h"
#include "AutomapUiState.h"
#include "GameWorldUiState.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../UI/TextEntry.h"
#include "../UI/UiPivotType.h"

#include "components/utilities/String.h"

namespace
{
	constexpr char AutomapTextureElementName[] = "AutomapTexture";
	constexpr char AutomapExitButtonElementName[] = "AutomapExitButton";

	std::string GetAutomapDirectionsDetailEntryElementName(const DialogueDirectionsDetailEntry &entry)
	{
		return String::format("AutomapDirectionsDetailEntry_%s(%s)", entry.buildingName.c_str(), entry.entranceWorldVoxel.toString().c_str());
	}

	std::string GetAutomapNoteEntryElementName(int entryIndex)
	{
		return String::format("AutomapNoteEntry_%d", entryIndex);
	}

	Int2 MakeAutomapTransformPosition(Double2 offset)
	{
		constexpr double pixelSizeReal = static_cast<double>(AutomapUiView::PixelSize);
		const int offsetX = static_cast<int>(std::floor(offset.x * pixelSizeReal));
		const int offsetY = static_cast<int>(std::floor(offset.y * pixelSizeReal));

		constexpr Rect drawingArea = AutomapUiView::DrawingArea;
		const int positionX = (drawingArea.getLeft() + (drawingArea.width / 2)) + offsetX;
		const int positionY = (drawingArea.getTop() + (drawingArea.height / 2)) + offsetY;
		return Int2(positionX, positionY);
	}

	Int2 GetAutomapTopLeftCornerWorldVoxel(const Player &player, double ceilingScale)
	{
		const WorldInt3 playerWorldVoxel = VoxelUtils::pointToVoxel(player.getEyePosition(), ceilingScale);
		const CoordInt3 playerVoxelCoord = VoxelUtils::worldVoxelToCoord(playerWorldVoxel);
		const CoordInt2 automapTopLeftCornerCoord(playerVoxelCoord.chunk + (ChunkInt2(-1, 1)), VoxelInt2(0, ChunkUtils::CHUNK_DIM));
		const WorldInt2 automapTopLeftCornerWorldVoxel = VoxelUtils::coordToWorldVoxel(automapTopLeftCornerCoord);
		return automapTopLeftCornerWorldVoxel;
	}

	Int2 WorldVoxelToUI(WorldInt2 worldVoxel, Int2 mapTransformPosition, Int2 automapTopLeftCornerWorldVoxel)
	{
		const Int2 automapTopLeftCornerToEntryDiff = worldVoxel - automapTopLeftCornerWorldVoxel;
		const Int2 automapTopLeftCornerToEntryPixelDiff = automapTopLeftCornerToEntryDiff * AutomapUiView::PixelSize;
		const Int2 entryTransformPosition = mapTransformPosition + Int2(-automapTopLeftCornerToEntryPixelDiff.y, automapTopLeftCornerToEntryPixelDiff.x);
		return entryTransformPosition;
	}

	Int2 UiToWorldVoxel(Int2 uiPosition, Int2 mapTransformPosition, Int2 automapTopLeftCornerWorldVoxel)
	{
		const Int2 entryToMapTransformPixelDiff = mapTransformPosition - uiPosition;
		const Int2 entryToMapTransformVoxelDiff = entryToMapTransformPixelDiff / AutomapUiView::PixelSize;
		const Int2 entryWorldVoxel = automapTopLeftCornerWorldVoxel + Int2(-entryToMapTransformVoxelDiff.y, entryToMapTransformVoxelDiff.x);
		return entryWorldVoxel;
	}
}

AutomapNoteEntry::AutomapNoteEntry()
{
	this->inGameWorld = false;
}

AutomapUiState::AutomapUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	this->mapTextureID = -1;
	this->cursorTextureID = -1;
	this->currentNoteElementInstID = -1;
	this->isCurrentNoteInGameWorld = false;
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
	this->mapTextureID = AutomapUiView::allocMapTexture(game, playerCoordXZ, player.getGroundDirectionXZ(), voxelChunkManager, renderer);

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

	uiManager.addMouseButtonChangedListener(AutomapUI::onMouseButtonChanged, contextDef.name.c_str(), inputManager);
	uiManager.addMouseButtonHeldListener(AutomapUI::onMouseButtonHeld, contextDef.name.c_str(), inputManager);
	uiManager.addTextInputListener(AutomapUI::onTextInput, contextDef.name.c_str(), inputManager);

	UiElementInitInfo mapImageElementInitInfo;
	mapImageElementInitInfo.name = AutomapTextureElementName;
	mapImageElementInitInfo.drawOrder = 1;
	mapImageElementInitInfo.clipRect = AutomapUiView::DrawingArea;
	uiManager.createImage(mapImageElementInitInfo, state.mapTextureID, state.contextInstID, renderer);

	GameState &gameState = game.gameState;
	const LocationDefinition &locationDef = gameState.getLocationDefinition();
	const LocationInstance &locationInst = gameState.getLocationInstance();
	const SceneManager &sceneManager = game.sceneManager;
	const LocationDefinitionType locationDefType = locationDef.type;
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

	for (const DialogueDirectionsDetailEntry &entry : gameState.getAutomapDirectionsDetailEntries())
	{
		UiElementInitInfo directionsTextBoxElementInitInfo;
		directionsTextBoxElementInitInfo.name = GetAutomapDirectionsDetailEntryElementName(entry);
		directionsTextBoxElementInitInfo.drawOrder = AutomapUiView::DialogueDirectionsDetailDrawOrder;
		directionsTextBoxElementInitInfo.clipRect = AutomapUiView::DrawingArea;

		UiTextBoxInitInfo directionsTextBoxInitInfo;
		directionsTextBoxInitInfo.text = entry.buildingName;
		directionsTextBoxInitInfo.fontName = ArenaFontName::S;
		directionsTextBoxInitInfo.defaultColor = AutomapUiView::ColorNote;
		uiManager.createTextBox(directionsTextBoxElementInitInfo, directionsTextBoxInitInfo, state.contextInstID, renderer);
	}

	Span<const AutomapNoteEntry> existingNoteEntries = gameState.getAutomapNoteEntries();
	for (int i = 0; i < existingNoteEntries.getCount(); i++)
	{
		const AutomapNoteEntry &entry = existingNoteEntries[i];

		UiElementInitInfo noteTextBoxElementInitInfo;
		noteTextBoxElementInitInfo.name = GetAutomapNoteEntryElementName(i);
		noteTextBoxElementInitInfo.position = entry.position;
		noteTextBoxElementInitInfo.drawOrder = AutomapUiView::NoteDrawOrder;

		if (entry.inGameWorld)
		{
			noteTextBoxElementInitInfo.clipRect = AutomapUiView::DrawingArea;
		}

		UiTextBoxInitInfo noteTextBoxInitInfo;
		noteTextBoxInitInfo.text = entry.text;
		noteTextBoxInitInfo.fontName = ArenaFontName::S;
		noteTextBoxInitInfo.defaultColor = AutomapUiView::ColorNote;
		const UiElementInstanceID noteTextBoxElementInstID = uiManager.createTextBox(noteTextBoxElementInitInfo, noteTextBoxInitInfo, state.contextInstID, renderer);

		if (entry.inGameWorld)
		{
			state.gameWorldNotes.emplace(noteTextBoxElementInstID, i);
		}
	}

	const UiCursorOverrideState cursorOverrideState(state.cursorTextureID, UiPivotType::BottomLeft);
	game.setCursorOverride(cursorOverrideState);

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

	state.currentNoteElementInstID = -1;
	state.currentNote.clear();
	state.gameWorldNotes.clear();

	game.setCursorOverride(std::nullopt);

	DebugAssert(!inputManager.isInTextEntryMode());
	inputManager.setInputActionMapActive(InputActionMapName::Automap, false);
}

void AutomapUI::update(double dt)
{
	const AutomapUiState &state = AutomapUI::state;

	const Int2 mapTransformPosition = MakeAutomapTransformPosition(state.automapOffset);

	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID mapImageElementInstID = uiManager.getElementByName(AutomapTextureElementName);
	uiManager.setTransformPosition(mapImageElementInstID, mapTransformPosition);

	const GameState &gameState = game.gameState;
	const Player &player = game.player;
	const WorldInt2 automapTopLeftCornerWorldVoxel = GetAutomapTopLeftCornerWorldVoxel(player, gameState.getActiveCeilingScale());

	for (const DialogueDirectionsDetailEntry &entry : gameState.getAutomapDirectionsDetailEntries())
	{
		const std::string entryElementName = GetAutomapDirectionsDetailEntryElementName(entry);
		const UiElementInstanceID entryElementInstID = uiManager.getElementByName(entryElementName.c_str());
		const Int2 entryTransformPosition = WorldVoxelToUI(entry.entranceWorldVoxel.getXZ(), mapTransformPosition, automapTopLeftCornerWorldVoxel);
		uiManager.setTransformPosition(entryElementInstID, entryTransformPosition);
	}

	for (const std::pair<UiElementInstanceID, int> &pair : state.gameWorldNotes)
	{
		const UiElementInstanceID entryElementInstID = pair.first;
		const AutomapNoteEntry &entry = gameState.getAutomapNoteEntries()[pair.second];

		if (entry.inGameWorld)
		{
			const Int2 entryTransformPosition = WorldVoxelToUI(entry.position, mapTransformPosition, automapTopLeftCornerWorldVoxel);
			uiManager.setTransformPosition(entryElementInstID, entryTransformPosition);
		}
	}
}

void AutomapUI::beginNote(Int2 originalPosition)
{
	AutomapUiState &state = AutomapUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	DebugAssert(!inputManager.isInTextEntryMode());
	inputManager.setTextInputMode(true);

	DebugAssert(state.currentNoteElementInstID < 0);

	GameState &gameState = game.gameState;
	const int existingNoteCount = gameState.getAutomapNoteEntries().getCount();
	const bool inGameWorld = AutomapUiView::DrawingArea.contains(originalPosition);

	UiElementInitInfo entryElementInitInfo;
	entryElementInitInfo.name = GetAutomapNoteEntryElementName(existingNoteCount);
	entryElementInitInfo.position = originalPosition;
	entryElementInitInfo.drawOrder = AutomapUiView::NoteDrawOrder;

	if (inGameWorld)
	{
		entryElementInitInfo.clipRect = AutomapUiView::DrawingArea;
	}

	UiTextBoxInitInfo entryTextBoxInitInfo;
	entryTextBoxInitInfo.worstCaseText = TextRenderUtils::makeWorstCaseText(AutomapNoteEntry::MAX_CHARACTERS);
	entryTextBoxInitInfo.fontName = ArenaFontName::S;
	entryTextBoxInitInfo.defaultColor = AutomapUiView::ColorNoteEditing;
	entryTextBoxInitInfo.hasCursor = true;
	state.currentNoteElementInstID = uiManager.createTextBox(entryElementInitInfo, entryTextBoxInitInfo, state.contextInstID, game.renderer);

	state.currentNote.clear();
	state.isCurrentNoteInGameWorld = inGameWorld;

	uiManager.setElementActive(game.cursorImageElementInstID, false);
}

void AutomapUI::confirmNote()
{
	AutomapUiState &state = AutomapUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	DebugAssert(inputManager.isInTextEntryMode());
	inputManager.setTextInputMode(false);

	DebugAssert(state.currentNoteElementInstID >= 0);

	if (!state.currentNote.empty())
	{
		uiManager.setTextBoxColor(state.currentNoteElementInstID, AutomapUiView::ColorNote);
		uiManager.setTextBoxHasCursor(state.currentNoteElementInstID, false);

		GameState &gameState = game.gameState;
		const Rect noteTextBoxRect = uiManager.getTransformGlobalRect(state.currentNoteElementInstID);
		Int2 notePosition = noteTextBoxRect.getTopLeft();
		if (state.isCurrentNoteInGameWorld)
		{
			const Player &player = game.player;
			const Int2 automapTransformPosition = MakeAutomapTransformPosition(state.automapOffset);
			const WorldInt2 automapTopLeftCornerWorldVoxel = GetAutomapTopLeftCornerWorldVoxel(player, gameState.getActiveCeilingScale());
			notePosition = UiToWorldVoxel(noteTextBoxRect.getTopLeft(), automapTransformPosition, automapTopLeftCornerWorldVoxel);

			state.gameWorldNotes.emplace(state.currentNoteElementInstID, gameState.getAutomapNoteEntries().getCount());
		}
		
		gameState.addAutomapNoteEntry(notePosition, state.currentNote, state.isCurrentNoteInGameWorld);
	}
	else
	{
		uiManager.freeTextBox(state.currentNoteElementInstID, game.renderer);
	}

	state.currentNoteElementInstID = -1;
	state.currentNote.clear();
	state.isCurrentNoteInGameWorld = false;

	uiManager.setElementActive(game.cursorImageElementInstID, true);
}

void AutomapUI::cancelNote()
{
	AutomapUiState &state = AutomapUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	DebugAssert(inputManager.isInTextEntryMode());
	inputManager.setTextInputMode(false);

	uiManager.freeTextBox(state.currentNoteElementInstID, game.renderer);
	state.currentNoteElementInstID = -1;
	state.currentNote.clear();
	state.isCurrentNoteInGameWorld = false;

	uiManager.setElementActive(game.cursorImageElementInstID, true);
}

void AutomapUI::onMouseButtonChanged(Game &game, MouseButtonType type, Int2 position, bool pressed)
{
	AutomapUiState &state = AutomapUI::state;
	InputManager &inputManager = game.inputManager;
	UiManager &uiManager = game.uiManager;
	if (!pressed)
	{
		return;
	}

	if (type == MouseButtonType::Left)
	{
		if (inputManager.isInTextEntryMode())
		{
			return;
		}

		const UiElementInstanceID exitButtonElementInstID = uiManager.getElementByName(AutomapExitButtonElementName);

		const Rect disallowedRegions[] =
		{
			AutomapUiView::CompassUpRegion,
			AutomapUiView::CompassDownRegion,
			AutomapUiView::CompassLeftRegion,
			AutomapUiView::CompassRightRegion,
			uiManager.getTransformGlobalRect(exitButtonElementInstID)
		};

		const Int2 originalPosition = game.window.nativeToOriginal(position);

		bool isPositionValidForCreatingNote = true;
		for (const Rect rect : disallowedRegions)
		{
			if (rect.contains(originalPosition))
			{
				isPositionValidForCreatingNote = false;
				break;
			}
		}

		if (!isPositionValidForCreatingNote)
		{
			return;
		}

		AutomapUI::beginNote(originalPosition);

	}
	else if (type == MouseButtonType::Right)
	{
		if (inputManager.isInTextEntryMode())
		{
			AutomapUI::cancelNote();
		}
	}
}

void AutomapUI::onMouseButtonHeld(Game &game, MouseButtonType buttonType, Int2 position, double dt)
{
	AutomapUiState &state = AutomapUI::state;
	const InputManager &inputManager = game.inputManager;
	if (inputManager.isInTextEntryMode())
	{
		return;
	}

	if (buttonType == MouseButtonType::Left)
	{
		const Int2 originalPoint = game.window.nativeToOriginal(position);
		const double scrollSpeed = AutomapUiView::ScrollSpeed * dt;

		if (AutomapUiView::CompassRightRegion.contains(originalPoint))
		{
			state.automapOffset = state.automapOffset - (Double2::UnitX * scrollSpeed);
		}
		else if (AutomapUiView::CompassLeftRegion.contains(originalPoint))
		{
			state.automapOffset = state.automapOffset + (Double2::UnitX * scrollSpeed);
		}
		else if (AutomapUiView::CompassUpRegion.contains(originalPoint))
		{
			state.automapOffset = state.automapOffset + (Double2::UnitY * scrollSpeed);
		}
		else if (AutomapUiView::CompassDownRegion.contains(originalPoint))
		{
			state.automapOffset = state.automapOffset - (Double2::UnitY * scrollSpeed);
		}
	}
}

void AutomapUI::onTextInput(const std::string_view text)
{
	AutomapUiState &state = AutomapUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;

	auto isCharAccepted = [](const char c)
	{
		return c >= ' ' && c <= '~';
	};

	const bool isTextChanged = TextEntry::append(state.currentNote, text, isCharAccepted, AutomapNoteEntry::MAX_CHARACTERS);
	if (isTextChanged)
	{
		uiManager.setTextBoxText(state.currentNoteElementInstID, state.currentNote.c_str());
	}
}

void AutomapUI::onAcceptInputAction(const InputActionCallbackValues &values)
{
	AutomapUiState &state = AutomapUI::state;
	Game &game = *state.game;
	const InputManager &inputManager = game.inputManager;
	if (!inputManager.isInTextEntryMode())
	{
		return;
	}

	AutomapUI::confirmNote();
}

void AutomapUI::onExitButtonSelected(MouseButtonType mouseButtonType)
{
	AutomapUiState &state = AutomapUI::state;
	Game &game = *state.game;
	const InputManager &inputManager = game.inputManager;
	if (inputManager.isInTextEntryMode())
	{
		return;
	}

	game.setNextContext(GameWorldUI::ContextName);
}

void AutomapUI::onBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		InputManager &inputManager = values.game.inputManager;
		if (inputManager.isInTextEntryMode())
		{
			AutomapUI::cancelNote();
		}
		else
		{
			AutomapUI::onExitButtonSelected(MouseButtonType::Left);
		}
	}
}

void AutomapUI::onBackspaceInputAction(const InputActionCallbackValues &values)
{
	AutomapUiState &state = AutomapUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	if (!inputManager.isInTextEntryMode())
	{
		return;
	}

	if (values.performed)
	{
		const bool isTextChanged = TextEntry::backspace(state.currentNote);
		if (isTextChanged)
		{
			uiManager.setTextBoxText(state.currentNoteElementInstID, state.currentNote.c_str());
		}
	}
}
