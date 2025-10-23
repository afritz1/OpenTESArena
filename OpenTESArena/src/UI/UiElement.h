#ifndef UI_ELEMENT_H
#define UI_ELEMENT_H

#include "UiButton.h"
#include "UiImage.h"
#include "UiTextBox.h"
#include "UiTransform.h"

enum class PivotType;
enum class UiScope;

// All UI elements (images, text boxes, etc.) come with a base handle.
using UiElementInstanceID = int;

enum class UiElementType
{
	Image,
	TextBox,
	Button
};

// Base instance for a drawable UI component.
struct UiElement
{
	UiScope scope;
	UiTransformInstanceID transformInstID;
	UiElementType type;

	union
	{
		UiImageInstanceID imageInstID;
		UiTextBoxInstanceID textBoxInstID;
		UiButtonInstanceID buttonInstID;
	};

	UiElement();

	void initImage(UiScope scope, UiTransformInstanceID transformInstID, UiImageInstanceID instID);
	void initTextBox(UiScope scope, UiTransformInstanceID transformInstID, UiTextBoxInstanceID instID);
	void initButton(UiScope scope, UiTransformInstanceID transformInstID, UiButtonInstanceID instID);
};

#endif
