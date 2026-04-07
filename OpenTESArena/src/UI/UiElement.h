#pragma once

#include "UiTransform.h"
#include "../Math/Rect.h"

enum class UiPivotType;
enum class UiRenderSpace;
enum class UiTransformSizeType;

// All UI elements (images, text boxes, etc.) come with a base handle.
using UiElementInstanceID = int;

using UiImageInstanceID = int;
using UiTextBoxInstanceID = int;
using UiListBoxInstanceID = int;
using UiButtonInstanceID = int;

enum class UiElementType
{
	Image,
	TextBox,
	ListBox,
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
	char contextName[64];
	Rect clipRect; // Pixel area this element is visible inside of. Valid if non-empty.
	int drawOrder; // Higher is drawn later.
	UiRenderSpace renderSpace;

	UiTransformInstanceID transformInstID; // Points to transform used with position + size on-screen for rendering.
	bool active;

	UiElementType type;

	union
	{
		UiImageInstanceID imageInstID;
		UiTextBoxInstanceID textBoxInstID;
		UiListBoxInstanceID listBoxInstID;
		UiButtonInstanceID buttonInstID;
	};

	UiElement();

	void initInternal(const char *name, const char *contextName, Rect clipRect, int drawOrder, UiRenderSpace renderSpace, UiTransformInstanceID transformInstID);

	void initImage(const char *name, const char *contextName, Rect clipRect, int drawOrder, UiRenderSpace renderSpace, UiTransformInstanceID transformInstID, UiImageInstanceID instID);
	void initTextBox(const char *name, const char *contextName, Rect clipRect, int drawOrder, UiRenderSpace renderSpace, UiTransformInstanceID transformInstID, UiTextBoxInstanceID instID);
	void initListBox(const char *name, const char *contextName, Rect clipRect, int drawOrder, UiRenderSpace renderSpace, UiTransformInstanceID transformInstID, UiListBoxInstanceID instID);
	void initButton(const char *name, const char *contextName, Rect clipRect, int drawOrder, UiRenderSpace renderSpace, UiTransformInstanceID transformInstID, UiButtonInstanceID instID);
};
