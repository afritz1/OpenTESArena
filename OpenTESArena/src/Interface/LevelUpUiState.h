#pragma once

#include "../Rendering/RenderTextureUtils.h"
#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;
class Random;

enum class MouseButtonType;

struct LevelUpUiState
{
	Game *game;
	UiContextInstanceID contextInstID;
	UiContextInstanceID remainingPointsPopUpContextInstID;

	int selectedAttributeIndex;

	LevelUpUiState();

	void init(Game &game);
};

namespace LevelUpUI
{
	DECLARE_UI_CONTEXT(LevelUp);

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

	void onDoneButtonSelected(MouseButtonType mouseButtonType);

	void onBackInputAction(const InputActionCallbackValues &values);

	void onRemainingPointsPopUpBackButtonSelected(MouseButtonType mouseButtonType);
	void onRemainingPointsPopUpBackInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		DECLARE_UI_FUNC(LevelUpUI, onStrengthButtonSelected),
		DECLARE_UI_FUNC(LevelUpUI, onIntelligenceButtonSelected),
		DECLARE_UI_FUNC(LevelUpUI, onWillpowerButtonSelected),
		DECLARE_UI_FUNC(LevelUpUI, onAgilityButtonSelected),
		DECLARE_UI_FUNC(LevelUpUI, onSpeedButtonSelected),
		DECLARE_UI_FUNC(LevelUpUI, onEnduranceButtonSelected),
		DECLARE_UI_FUNC(LevelUpUI, onPersonalityButtonSelected),
		DECLARE_UI_FUNC(LevelUpUI, onLuckButtonSelected),
		DECLARE_UI_FUNC(LevelUpUI, onStrengthUpButtonSelected),
		DECLARE_UI_FUNC(LevelUpUI, onStrengthDownButtonSelected),
		DECLARE_UI_FUNC(LevelUpUI, onIntelligenceUpButtonSelected),
		DECLARE_UI_FUNC(LevelUpUI, onIntelligenceDownButtonSelected),
		DECLARE_UI_FUNC(LevelUpUI, onWillpowerUpButtonSelected),
		DECLARE_UI_FUNC(LevelUpUI, onWillpowerDownButtonSelected),
		DECLARE_UI_FUNC(LevelUpUI, onAgilityUpButtonSelected),
		DECLARE_UI_FUNC(LevelUpUI, onAgilityDownButtonSelected),
		DECLARE_UI_FUNC(LevelUpUI, onSpeedUpButtonSelected),
		DECLARE_UI_FUNC(LevelUpUI, onSpeedDownButtonSelected),
		DECLARE_UI_FUNC(LevelUpUI, onEnduranceUpButtonSelected),
		DECLARE_UI_FUNC(LevelUpUI, onEnduranceDownButtonSelected),
		DECLARE_UI_FUNC(LevelUpUI, onPersonalityUpButtonSelected),
		DECLARE_UI_FUNC(LevelUpUI, onPersonalityDownButtonSelected),
		DECLARE_UI_FUNC(LevelUpUI, onLuckUpButtonSelected),
		DECLARE_UI_FUNC(LevelUpUI, onLuckDownButtonSelected),
		DECLARE_UI_FUNC(LevelUpUI, onDoneButtonSelected)
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(LevelUpUI, onBackInputAction)
	};
}
