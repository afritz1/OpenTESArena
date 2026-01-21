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
enum class UiPivotType;
enum class UiRenderSpace;
enum class UiTexturePatternType;
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

enum class UiImageDefinitionType
{
	Asset,
	Generated
};

struct UiImageDefinition
{
	UiElementDefinition element;

	UiImageDefinitionType type;

	// Asset
	TextureAsset texture;
	TextureAsset palette;

	// Generated
	UiTexturePatternType patternType;
	int generatedWidth;
	int generatedHeight;

	UiImageDefinition();

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

struct UiListBoxDefinition
{
	UiElementDefinition element;
	int textureWidth;
	int textureHeight;
	int itemPixelSpacing;
	std::string fontName;
	Color defaultTextColor;
	double scrollDeltaScale;

	UiListBoxDefinition();

	void clear();
};

using UiButtonDefinitionCallback = void(*)(MouseButtonType);

struct UiButtonDefinition
{
	UiElementDefinition element;
	UiButtonDefinitionCallback callback;
	std::string contentElementName;

	UiButtonDefinition();
	
	void clear();
};

using UiInputListenerDefinitionCallback = void(*)(const InputActionCallbackValues&);

struct UiInputListenerDefinition
{
	std::string name;
	std::string inputActionName;
	UiInputListenerDefinitionCallback callback;

	UiInputListenerDefinition();

	void clear();
};

struct UiContextDefinition
{
	std::string name;
	std::vector<UiImageDefinition> imageDefs;
	std::vector<UiTextBoxDefinition> textBoxDefs;
	std::vector<UiListBoxDefinition> listBoxDefs;
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
	static constexpr const char GlobalContextName[] = "Global";

	bool init(const char *folderPath);

	const UiContextDefinition &getDefinition(const char *contextName) const;
};

#endif
