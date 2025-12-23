#ifndef UI_ELEMENT_H
#define UI_ELEMENT_H

#include "UiTransform.h"
#include "../Math/Rect.h"

class UiManager;

enum class UiContextType;
enum class UiPivotType;
enum class UiRenderSpace;
enum class UiTransformSizeType;

// All UI elements (images, text boxes, etc.) come with a base handle.
using UiElementInstanceID = int;

using UiImageInstanceID = int;
using UiTextBoxInstanceID = int;
using UiButtonInstanceID = int;

enum class UiElementType
{
	Image,
	TextBox,
	Button
};

struct UiElementInitInfo
{
	std::string name;
	Int2 position;
	UiTransformSizeType sizeType;
	Int2 size;
	UiPivotType pivotType;
	Rect clipRect;
	int drawOrder;
	UiRenderSpace renderSpace;

	UiElementInitInfo();
};

// Base instance for a drawable UI component.
struct UiElement
{
	char name[96];
	UiContextType contextType;
	Rect clipRect; // Window area this element is visible inside of. Valid if non-empty.
	int drawOrder; // Higher is drawn later.
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

	void initImage(const char *name, UiContextType contextType, Rect clipRect, int drawOrder, UiRenderSpace renderSpace, UiTransformInstanceID transformInstID, UiImageInstanceID instID);
	void initTextBox(const char *name, UiContextType contextType, Rect clipRect, int drawOrder, UiRenderSpace renderSpace, UiTransformInstanceID transformInstID, UiTextBoxInstanceID instID);
	void initButton(const char *name, UiContextType contextType, Rect clipRect, int drawOrder, UiRenderSpace renderSpace, UiTransformInstanceID transformInstID, UiButtonInstanceID instID);
};

#endif
