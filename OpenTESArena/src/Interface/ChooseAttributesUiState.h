#pragma once

#include "../Rendering/RenderTextureUtils.h"
#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class ArenaRandom;
class Game;
class Random;

enum class MouseButtonType;

struct CharacterCreationState;

struct ChooseAttributesUiState
{
	Game *game;
	UiContextInstanceID contextInstID;
	UiContextInstanceID initialPopUpContextInstID;
	UiContextInstanceID saveRerollContextInstID;
	UiContextInstanceID remainingPointsPopUpContextInstID;
	UiContextInstanceID portraitPopUpContextInstID;

	Buffer<UiTextureID> headTextureIDs; // Owned by UI manager.
	
	bool attributesAreSaved; // Whether points have been saved and the portrait can now be changed.
	int selectedAttributeIndex;

	ChooseAttributesUiState();

	void init(Game &game);
};

namespace ChooseAttributesUI
{
	DECLARE_UI_CONTEXT(ChooseAttributes);

	void randomizeStats(CharacterCreationState &charCreationState, Random &random, ArenaRandom &arenaRandom);
	void updatePrimaryAttributes();
	void updateDerivedAttributes();
	void updateGold();
	void updateBonusPoints();

	void onStrengthButtonSelected(MouseButtonType mouseButtonType);
	void onIntelligenceButtonSelected(MouseButtonType mouseButtonType);
	void onWillpowerButtonSelected(MouseButtonType mouseButtonType);
	void onAgilityButtonSelected(MouseButtonType mouseButtonType);
	void onSpeedButtonSelected(MouseButtonType mouseButtonType);
	void onEnduranceButtonSelected(MouseButtonType mouseButtonType);
	void onPersonalityButtonSelected(MouseButtonType mouseButtonType);
	void onLuckButtonSelected(MouseButtonType mouseButtonType);

	void onStrengthUpButtonSelected(MouseButtonType mouseButtonType);
	void onStrengthDownButtonSelected(MouseButtonType mouseButtonType);
	void onIntelligenceUpButtonSelected(MouseButtonType mouseButtonType);
	void onIntelligenceDownButtonSelected(MouseButtonType mouseButtonType);
	void onWillpowerUpButtonSelected(MouseButtonType mouseButtonType);
	void onWillpowerDownButtonSelected(MouseButtonType mouseButtonType);
	void onAgilityUpButtonSelected(MouseButtonType mouseButtonType);
	void onAgilityDownButtonSelected(MouseButtonType mouseButtonType);
	void onSpeedUpButtonSelected(MouseButtonType mouseButtonType);
	void onSpeedDownButtonSelected(MouseButtonType mouseButtonType);
	void onEnduranceUpButtonSelected(MouseButtonType mouseButtonType);
	void onEnduranceDownButtonSelected(MouseButtonType mouseButtonType);
	void onPersonalityUpButtonSelected(MouseButtonType mouseButtonType);
	void onPersonalityDownButtonSelected(MouseButtonType mouseButtonType);
	void onLuckUpButtonSelected(MouseButtonType mouseButtonType);
	void onLuckDownButtonSelected(MouseButtonType mouseButtonType);

	void onPortraitButtonSelected(MouseButtonType mouseButtonType);
	void onDoneButtonSelected(MouseButtonType mouseButtonType);

	void onBackInputAction(const InputActionCallbackValues &values);

	void onInitialPopUpBackButtonSelected(MouseButtonType mouseButtonType);
	void onInitialPopUpBackInputAction(const InputActionCallbackValues &values);

	void onSaveRerollSaveButtonSelected(MouseButtonType mouseButtonType);
	void onSaveRerollSaveInputAction(const InputActionCallbackValues &values);
	void onSaveRerollRerollButtonSelected(MouseButtonType mouseButtonType);
	void onSaveRerollRerollInputAction(const InputActionCallbackValues &values);
	void onSaveRerollBackInputAction(const InputActionCallbackValues &values);

	void onRemainingPointsPopUpBackButtonSelected(MouseButtonType mouseButtonType);
	void onRemainingPointsPopUpBackInputAction(const InputActionCallbackValues &values);
	
	void onPortraitPopUpBackButtonSelected(MouseButtonType mouseButtonType);
	void onPortraitPopUpBackInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		DECLARE_UI_FUNC(ChooseAttributesUI, onStrengthButtonSelected),
		DECLARE_UI_FUNC(ChooseAttributesUI, onIntelligenceButtonSelected),
		DECLARE_UI_FUNC(ChooseAttributesUI, onWillpowerButtonSelected),
		DECLARE_UI_FUNC(ChooseAttributesUI, onAgilityButtonSelected),
		DECLARE_UI_FUNC(ChooseAttributesUI, onSpeedButtonSelected),
		DECLARE_UI_FUNC(ChooseAttributesUI, onEnduranceButtonSelected),
		DECLARE_UI_FUNC(ChooseAttributesUI, onPersonalityButtonSelected),
		DECLARE_UI_FUNC(ChooseAttributesUI, onLuckButtonSelected),
		DECLARE_UI_FUNC(ChooseAttributesUI, onStrengthUpButtonSelected),
		DECLARE_UI_FUNC(ChooseAttributesUI, onStrengthDownButtonSelected),
		DECLARE_UI_FUNC(ChooseAttributesUI, onIntelligenceUpButtonSelected),
		DECLARE_UI_FUNC(ChooseAttributesUI, onIntelligenceDownButtonSelected),
		DECLARE_UI_FUNC(ChooseAttributesUI, onWillpowerUpButtonSelected),
		DECLARE_UI_FUNC(ChooseAttributesUI, onWillpowerDownButtonSelected),
		DECLARE_UI_FUNC(ChooseAttributesUI, onAgilityUpButtonSelected),
		DECLARE_UI_FUNC(ChooseAttributesUI, onAgilityDownButtonSelected),
		DECLARE_UI_FUNC(ChooseAttributesUI, onSpeedUpButtonSelected),
		DECLARE_UI_FUNC(ChooseAttributesUI, onSpeedDownButtonSelected),
		DECLARE_UI_FUNC(ChooseAttributesUI, onEnduranceUpButtonSelected),
		DECLARE_UI_FUNC(ChooseAttributesUI, onEnduranceDownButtonSelected),
		DECLARE_UI_FUNC(ChooseAttributesUI, onPersonalityUpButtonSelected),
		DECLARE_UI_FUNC(ChooseAttributesUI, onPersonalityDownButtonSelected),
		DECLARE_UI_FUNC(ChooseAttributesUI, onLuckUpButtonSelected),
		DECLARE_UI_FUNC(ChooseAttributesUI, onLuckDownButtonSelected),
		DECLARE_UI_FUNC(ChooseAttributesUI, onPortraitButtonSelected),
		DECLARE_UI_FUNC(ChooseAttributesUI, onDoneButtonSelected)
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(ChooseAttributesUI, onBackInputAction)
	};
}
