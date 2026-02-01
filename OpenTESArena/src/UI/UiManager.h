#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "TextRenderUtils.h"
#include "UiButton.h"
#include "UiContext.h"
#include "UiElement.h"
#include "UiImage.h"
#include "UiListBox.h"
#include "UiTextBox.h"
#include "UiTransform.h"
#include "../Assets/TextureAsset.h"
#include "../Rendering/Renderer.h"

#include "components/utilities/KeyValuePool.h"

class Game;
class InputManager;
class TextureManager;

struct UiCommandList;
struct UiContextDefinition;
struct Window;

struct LoadedUiTexture
{
	// @todo use context type as well so it's not taking up space forever
	TextureAsset textureAsset;
	TextureAsset paletteAsset;
	UiTextureID textureID;

	LoadedUiTexture();
};

struct GeneratedUiTexture
{
	// @todo use context type as well so it's not taking up space forever
	UiTexturePatternType patternType;
	int width;
	int height;
	UiTextureID textureID;

	GeneratedUiTexture();
};

class UiManager
{
private:
	KeyValuePool<UiContextInstanceID, UiContext> contexts;
	KeyValuePool<UiTransformInstanceID, UiTransform> transforms;
	KeyValuePool<UiElementInstanceID, UiElement> elements;
	KeyValuePool<UiImageInstanceID, UiImage> images;
	KeyValuePool<UiTextBoxInstanceID, UiTextBox> textBoxes;
	KeyValuePool<UiListBoxInstanceID, UiListBox> listBoxes;
	KeyValuePool<UiButtonInstanceID, UiButton> buttons;

	std::vector<LoadedUiTexture> loadedTextures;
	std::vector<GeneratedUiTexture> generatedTextures;

	std::unordered_map<std::string, UiContextBeginCallback> beginContextCallbacks;
	std::unordered_map<std::string, UiContextUpdateCallback> updateContextCallbacks;
	std::unordered_map<std::string, UiContextEndCallback> endContextCallbacks;

	std::vector<RenderElement2D> renderElementsCache; // To be drawn. Updated every frame.

	UiContextInstanceID getContextByName(const char *name) const;

	UiTextureID getOrAddTexture(const TextureAsset &textureAsset, const TextureAsset &paletteAsset, TextureManager &textureManager, Renderer &renderer);
	UiTextureID getOrAddTexture(UiTexturePatternType patternType, int width, int height, TextureManager &textureManager, Renderer &renderer);

	// Context callbacks are necessary for bootstrapping the begin/end context API.
	void setBeginContextCallback(const char *contextName, const UiContextBeginCallback &callback);
	void setEndContextCallback(const char *contextName, const UiContextEndCallback &callback);
	void setUpdateContextCallback(const char *contextName, const UiContextUpdateCallback &callback);
public:
	bool init();
	void shutdown(Renderer &renderer);

	UiElementInstanceID getElementByName(const char *name) const;
	void setElementActive(UiElementInstanceID elementInstID, bool active);
	std::vector<UiElementInstanceID> getTopMostActiveElementsOfType(UiElementType elementType) const;

	Rect getTransformGlobalRect(UiElementInstanceID elementInstID) const; // Includes local-to-parent transform.
	void setTransformPosition(UiElementInstanceID elementInstID, Int2 position);
	void setTransformSize(UiElementInstanceID elementInstID, Int2 size);
	void setTransformPivot(UiElementInstanceID elementInstID, UiPivotType pivotType);

	const UiButtonCallback &getButtonCallback(UiElementInstanceID elementInstID) const;
	bool isMouseButtonValidForButton(MouseButtonType mouseButtonType, UiElementInstanceID elementInstID) const;	

	UiElementInstanceID createImage(const UiElementInitInfo &initInfo, UiTextureID textureID, UiContextInstanceID contextInstID);
	void setImageTexture(UiElementInstanceID elementInstID, UiTextureID textureID);
	void freeImage(UiElementInstanceID elementInstID);

	UiElementInstanceID createTextBox(const UiElementInitInfo &initInfo, const UiTextBoxInitInfo &textBoxInitInfo, UiContextInstanceID contextInstID, Renderer &renderer);
	void setTextBoxText(UiElementInstanceID elementInstID, const char *str);
	void freeTextBox(UiElementInstanceID elementInstID, Renderer &renderer);

	UiElementInstanceID createListBox(const UiElementInitInfo &initInfo, const UiListBoxInitInfo &listBoxInitInfo, UiContextInstanceID contextInstID, Renderer &renderer);
	int getListBoxItemCount(UiElementInstanceID elementInstID) const;
	Rect getListBoxItemGlobalRect(UiElementInstanceID elementInstID, int itemIndex) const;
	const UiListBoxItemCallback &getListBoxItemCallback(UiElementInstanceID elementInstID, int itemIndex) const;
	int getListBoxHoveredItemIndex(UiElementInstanceID elementInstID, const InputManager &inputManager, const Window &window) const;
	void insertListBoxItem(UiElementInstanceID elementInstID, int index, UiListBoxItem &&item);
	void insertBackListBoxItem(UiElementInstanceID elementInstID, UiListBoxItem &&item);
	void eraseListBoxItem(UiElementInstanceID elementInstID, int index);
	void scrollListBoxDown(UiElementInstanceID elementInstID);
	void scrollListBoxUp(UiElementInstanceID elementInstID);
	void freeListBox(UiElementInstanceID elementInstID, Renderer &renderer);

	UiElementInstanceID createButton(const UiElementInitInfo &initInfo, const UiButtonInitInfo &buttonInitInfo, UiContextInstanceID contextInstID);
	void freeButton(UiElementInstanceID elementInstID);

	void addInputActionListener(const char *actionName, const InputActionCallback &callback, UiContextInstanceID contextInstID, InputManager &inputManager);
	void addMouseButtonChangedListener(const MouseButtonChangedCallback &callback, UiContextInstanceID contextInstID, InputManager &inputManager);
	void addMouseButtonHeldListener(const MouseButtonHeldCallback &callback, UiContextInstanceID contextInstID, InputManager &inputManager);
	void addMouseScrollChangedListener(const MouseScrollChangedCallback &callback, UiContextInstanceID contextInstID, InputManager &inputManager);
	void addMouseMotionListener(const MouseMotionCallback &callback, UiContextInstanceID contextInstID, InputManager &inputManager);
	void addApplicationExitListener(const ApplicationExitCallback &callback, UiContextInstanceID contextInstID, InputManager &inputManager);
	void addWindowResizedListener(const WindowResizedCallback &callback, UiContextInstanceID contextInstID, InputManager &inputManager);
	void addRenderTargetsResetListener(const RenderTargetsResetCallback &callback, UiContextInstanceID contextInstID, InputManager &inputManager);
	void addTextInputListener(const TextInputCallback &callback, UiContextInstanceID contextInstID, InputManager &inputManager);
	
	UiContextInstanceID createContext(const UiContextInitInfo &initInfo);
	UiContextInstanceID createContext(const UiContextDefinition &contextDef, InputManager &inputManager, TextureManager &textureManager, Renderer &renderer);
	
	bool isContextActive(const char *contextName) const;
	void setContextActive(UiContextInstanceID contextInstID, bool active);
	UiContextInstanceID getTopMostActiveContext() const;
	void freeContext(UiContextInstanceID contextInstID, InputManager &inputManager, Renderer &renderer);

	// Intended for text file contexts registered at manager startup.
	void beginContext(const char *contextName, Game &game);
	void endContext(const char *contextName, Game &game);

	void populateCommandList(UiCommandList &commandList);

	void update(double dt, Game &game);
};

#endif
