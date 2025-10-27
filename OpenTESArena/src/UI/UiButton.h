#ifndef UI_BUTTON_H
#define UI_BUTTON_H

using UiButtonInstanceID = int;

struct UiButton
{
	//Button<> button;
	// @todo buttons should have optional UiTextureID to avoid requiring a separate UiImage component at the same position

	UiButton();

	void init();
};

#endif
