#ifndef CHARACTER_EQUIPMENT_UI_STATE_H
#define CHARACTER_EQUIPMENT_UI_STATE_H

#include "../Rendering/RenderTextureUtils.h"
#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

enum class MouseButtonType;

struct CharacterEquipmentUiState
{
	Game *game;
	UiContextInstanceID contextInstID;

	UiTextureID bodyTextureID;
	UiTextureID pantsTextureID;
	UiTextureID headTextureID;
	UiTextureID shirtTextureID;

	CharacterEquipmentUiState();

	void init(Game &game);
	void freeTextures(Renderer &renderer);
};

namespace CharacterEquipmentUI
{
	DECLARE_UI_CONTEXT(CharacterEquipment);

	void onMouseScrollChanged(Game &game, MouseWheelScrollType type, const Int2 &position);

	void onInventoryListBoxUpButtonSelected(MouseButtonType mouseButtonType);
	void onInventoryListBoxDownButtonSelected(MouseButtonType mouseButtonType);
	void onExitButtonSelected(MouseButtonType mouseButtonType);
	void onSpellbookButtonSelected(MouseButtonType mouseButtonType);
	void onDropButtonSelected(MouseButtonType mouseButtonType);

	void onBackInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		DECLARE_UI_FUNC(CharacterEquipmentUI, onInventoryListBoxUpButtonSelected),
		DECLARE_UI_FUNC(CharacterEquipmentUI, onInventoryListBoxDownButtonSelected),
		DECLARE_UI_FUNC(CharacterEquipmentUI, onExitButtonSelected),
		DECLARE_UI_FUNC(CharacterEquipmentUI, onSpellbookButtonSelected),
		DECLARE_UI_FUNC(CharacterEquipmentUI, onDropButtonSelected)
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(CharacterEquipmentUI, onBackInputAction)
	};
}

#endif
