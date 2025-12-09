#ifndef UI_LIBRARY_H
#define UI_LIBRARY_H

#include <vector>

#include "components/utilities/Singleton.h"

enum class MouseButtonType;
enum class UiContextType;

// @todo these defs will replace InitInfos eventually
struct UiElementDefinition
{

};

struct UiImageDefinition
{
	UiElementDefinition element;

	// @todo TextureAsset
};

struct UiTextBoxDefinition
{
	UiElementDefinition element;

};

using UiButtonDefinitionCallback = void(*)(MouseButtonType);

struct UiButtonDefinition
{
	UiElementDefinition element;
	UiButtonDefinitionCallback callback;
};

struct UiContextDefinition
{
	UiContextType type;
	std::vector<UiImageDefinition> imageDefs;
	std::vector<UiTextBoxDefinition> textBoxDefs;
	std::vector<UiButtonDefinition> buttonDefs;

	UiContextDefinition();
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
