#ifndef CHOOSE_RACE_UI_STATE_H
#define CHOOSE_RACE_UI_STATE_H

#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

enum class MouseButtonType;

struct ChooseRaceUiState
{
	Game *game;
	UiContextInstanceID contextInstID;
	UiContextInstanceID initialPopUpContextInstID;
	UiContextInstanceID provinceConfirmContextInstID;
	UiContextInstanceID provinceConfirmed1ContextInstID;
	UiContextInstanceID provinceConfirmed2ContextInstID;
	UiContextInstanceID provinceConfirmed3ContextInstID;
	UiContextInstanceID provinceConfirmed4ContextInstID;

	ChooseRaceUiState();

	void init(Game &game);
};

namespace ChooseRaceUI
{
	DECLARE_UI_CONTEXT(ChooseRace);

	void onMouseButtonChanged(Game &game, MouseButtonType mouseButtonType, const Int2 &position, bool pressed);

	void onBackInputAction(const InputActionCallbackValues &values);

	void onInitialPopUpBackButtonSelected(MouseButtonType mouseButtonType);
	void onInitialPopUpBackInputAction(const InputActionCallbackValues &values);

	void onProvinceConfirmPopUpAcceptButtonSelected(MouseButtonType mouseButtonType);
	void onProvinceConfirmPopUpCancelButtonSelected(MouseButtonType mouseButtonType);
	void onProvinceConfirmPopUpBackInputAction(const InputActionCallbackValues &values);

	void onProvinceConfirmed1PopUpBackButtonSelected(MouseButtonType mouseButtonType);
	void onProvinceConfirmed2PopUpBackButtonSelected(MouseButtonType mouseButtonType);
	void onProvinceConfirmed3PopUpBackButtonSelected(MouseButtonType mouseButtonType);
	void onProvinceConfirmed4PopUpBackButtonSelected(MouseButtonType mouseButtonType);
	void onProvinceConfirmed1PopUpBackInputAction(const InputActionCallbackValues &values);
	void onProvinceConfirmed2PopUpBackInputAction(const InputActionCallbackValues &values);
	void onProvinceConfirmed3PopUpBackInputAction(const InputActionCallbackValues &values);
	void onProvinceConfirmed4PopUpBackInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		{ "", UiButtonDefinitionCallback() }
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(ChooseRaceUI, onBackInputAction)
	};
}

#endif
