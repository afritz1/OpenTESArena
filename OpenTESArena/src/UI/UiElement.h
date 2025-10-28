#ifndef UI_ELEMENT_H
#define UI_ELEMENT_H

#include "UiButton.h"
#include "UiImage.h"
#include "UiTextBox.h"
#include "UiTransform.h"

enum class PivotType;
enum class RenderSpace;
enum class UiScope;

// All UI elements (images, text boxes, etc.) come with a base handle.
using UiElementInstanceID = int;

enum class UiElementType
{
	Image,
	TextBox,
	Button
};

struct UiElementInitInfo
{
	Int2 position;
	Int2 size;
	PivotType pivotType;
	UiScope scope;
	int drawOrder;
	RenderSpace renderSpace;

	UiElementInitInfo();
};

// Base instance for a drawable UI component.
struct UiElement
{
	UiScope scope;
	int drawOrder; // Higher is drawn last.
	RenderSpace renderSpace;

	UiTransformInstanceID transformInstID;
	bool active;

	UiElementType type;

	union
	{
		UiImageInstanceID imageInstID;
		UiTextBoxInstanceID textBoxInstID;
		UiButtonInstanceID buttonInstID;
	};

	UiElement();

	void initImage(UiScope scope, int drawOrder, RenderSpace renderSpace, UiTransformInstanceID transformInstID, UiImageInstanceID instID);
	void initTextBox(UiScope scope, int drawOrder, RenderSpace renderSpace, UiTransformInstanceID transformInstID, UiTextBoxInstanceID instID);
	void initButton(UiScope scope, int drawOrder, RenderSpace renderSpace, UiTransformInstanceID transformInstID, UiButtonInstanceID instID);
};

#endif
