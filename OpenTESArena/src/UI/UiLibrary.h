#ifndef UI_LIBRARY_H
#define UI_LIBRARY_H

#include <optional>
#include <string>
#include <vector>

#include "TextRenderUtils.h"
#include "../Assets/TextureAsset.h"
#include "../Math/Vector2.h"

#include "components/utilities/Singleton.h"

enum class MouseButtonType;
enum class TextAlignment;
enum class UiContextType;
enum class UiPivotType;
enum class UiRenderSpace;
enum class UiTransformSizeType;

struct InputActionCallbackValues;

// @todo these defs will replace InitInfos eventually
struct UiElementDefinition
{
	std::string name;
	Int2 position;
	UiTransformSizeType sizeType;
	Int2 size;
	UiPivotType pivotType;
	int drawOrder;
	UiRenderSpace renderSpace;

	UiElementDefinition();

	void clear();
};

struct UiImageDefinition
{
	UiElementDefinition element;
	TextureAsset texture;
	TextureAsset palette;

	void clear();
};

struct UiTextBoxDefinition
{
	UiElementDefinition element;
	std::string worstCaseText;
	std::string text;
	std::string fontName;
	Color defaultColor;
	TextAlignment alignment;
	std::optional<TextRenderShadowInfo> shadowInfo;
	int lineSpacing;

	void clear();
};

using UiButtonDefinitionCallback = void(*)(MouseButtonType);

struct UiButtonDefinition
{
	UiElementDefinition element;
	//UiButtonDefinitionCallback callback;
	std::string callback; // @todo look up function ahead of time
	
	void clear();
};

using UiInputListenerDefinitionCallback = void(*)(const InputActionCallbackValues&);

struct UiInputListenerDefinition
{
	std::string name;
	//UiInputListenerDefinitionCallback callback;
	std::string callback; // @todo look up function ahead of time

	void clear();
};

struct UiContextDefinition
{
	UiContextType type;
	std::vector<UiImageDefinition> imageDefs;
	std::vector<UiTextBoxDefinition> textBoxDefs;
	std::vector<UiButtonDefinition> buttonDefs;
	std::vector<UiInputListenerDefinition> inputListenerDefs;

	UiContextDefinition();

	void clear();
};

class UiLibrary : public Singleton<UiLibrary>
{
private:
	std::vector<UiContextDefinition> contextDefs; // To be instantiated by UI manager.
public:
	bool init(const char *folderPath);

	const UiContextDefinition &getDefinition(UiContextType contextType) const;
};

#endif
