#ifndef UI_ELEMENT_H
#define UI_ELEMENT_H

#include "UiButton.h"
#include "UiImage.h"
#include "UiTextBox.h"
#include "UiTransform.h"

class UiManager;

enum class PivotType;
enum class UiContextType;
enum class UiRenderSpace;
enum class UiTransformSizeType;

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
	UiTransformSizeType sizeType;
	PivotType pivotType;
	UiContextType contextType;
	int drawOrder;
	UiRenderSpace renderSpace;

	UiElementInitInfo();
};

// Base instance for a drawable UI component.
struct UiElement
{
	UiContextType contextType;
	int drawOrder; // Higher is drawn last.
	UiRenderSpace renderSpace;

	UiTransformInstanceID transformInstID; // Points to transform used with position + size on-screen for rendering.
	bool active;

	UiElementType type;

	union
	{
		UiImageInstanceID imageInstID;
		UiTextBoxInstanceID textBoxInstID;
		UiButtonInstanceID buttonInstID;
	};

	UiElement();

	void initImage(UiContextType contextType, int drawOrder, UiRenderSpace renderSpace, UiTransformInstanceID transformInstID, UiImageInstanceID instID);
	void initTextBox(UiContextType contextType, int drawOrder, UiRenderSpace renderSpace, UiTransformInstanceID transformInstID, UiTextBoxInstanceID instID);
	void initButton(UiContextType contextType, int drawOrder, UiRenderSpace renderSpace, UiTransformInstanceID transformInstID, UiButtonInstanceID instID);
};

#endif
