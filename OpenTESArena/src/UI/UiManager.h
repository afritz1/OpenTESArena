#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <optional>
#include <unordered_map>
#include <vector>

#include "TextRenderUtils.h"
#include "UiButton.h"
#include "UiElement.h"
#include "UiImage.h"
#include "UiTextBox.h"
#include "UiTransform.h"
#include "../Rendering/Renderer.h"

#include "components/utilities/KeyValuePool.h"

class Game;
class TextureManager;

enum class UiContextType;

struct UiCommandList;
struct UiContextElements;

using UiContextCallback = void(*)(Game &game);
using UiContextUpdateCallback = void(*)(double dt, Game &game);

class UiManager
{
private:
	KeyValuePool<UiTransformInstanceID, UiTransform> transforms;
	KeyValuePool<UiElementInstanceID, UiElement> elements;
	KeyValuePool<UiImageInstanceID, UiImage> images;
	KeyValuePool<UiTextBoxInstanceID, UiTextBox> textBoxes;
	KeyValuePool<UiButtonInstanceID, UiButton> buttons;

	std::unordered_map<UiContextType, std::vector<UiContextCallback>> beginContextCallbackLists;
	std::unordered_map<UiContextType, std::vector<UiContextUpdateCallback>> updateContextCallbackLists;
	std::unordered_map<UiContextType, std::vector<UiContextCallback>> endContextCallbackLists;
	std::optional<UiContextType> activeContextType;

	std::vector<RenderElement2D> renderElementsCache; // To be drawn. Updated every frame.
public:
	bool init(const char *folderPath, TextureManager &textureManager, Renderer &renderer);
	void shutdown(Renderer &renderer);

	void setElementActive(UiElementInstanceID elementInstID, bool active);

	Rect getTransformGlobalRect(UiElementInstanceID elementInstID) const; // Includes local-to-parent transform.
	void setTransformPosition(UiElementInstanceID elementInstID, Int2 position);
	void setTransformSize(UiElementInstanceID elementInstID, Int2 size);

	const UiButtonCallback &getButtonCallback(UiElementInstanceID elementInstID) const;
	bool isMouseButtonValidForButton(MouseButtonType mouseButtonType, UiElementInstanceID elementInstID) const;
	
	std::vector<UiElementInstanceID> getActiveButtonInstIDs() const;

	UiElementInstanceID createImage(const UiElementInitInfo &initInfo, UiTextureID textureID, UiContextElements &contextElements);
	void setImageTexture(UiElementInstanceID elementInstID, UiTextureID textureID);
	void freeImage(UiElementInstanceID elementInstID);

	UiElementInstanceID createTextBox(const UiElementInitInfo &initInfo, const UiTextBoxInitInfo &textBoxInitInfo, UiContextElements &contextElements, Renderer &renderer);
	void setTextBoxText(UiElementInstanceID elementInstID, const char *str);
	void freeTextBox(UiElementInstanceID elementInstID, Renderer &renderer);

	UiElementInstanceID createButton(const UiElementInitInfo &initInfo, const UiButtonInitInfo &buttonInitInfo, UiContextElements &contextElements);
	void freeButton(UiElementInstanceID elementInstID);

	void addBeginContextCallback(UiContextType contextType, const UiContextCallback &callback);
	void addUpdateContextCallback(UiContextType contextType, const UiContextUpdateCallback &callback);
	void addEndContextCallback(UiContextType contextType, const UiContextCallback &callback);
	void clearContextCallbacks(UiContextType contextType);

	void beginContext(UiContextType contextType, Game &game);
	void endContext(UiContextType contextType, Game &game);
	bool isContextActive(UiContextType contextType) const;

	void populateCommandList(UiCommandList &commandList);

	void update(double dt, Game &game);
};

#endif
