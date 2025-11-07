#ifndef PANEL_H
#define PANEL_H

#include <string_view>
#include <vector>

#include "../Assets/TextureManager.h"
#include "../Assets/TextureUtils.h"
#include "../Input/InputManager.h"
#include "../Rendering/Renderer.h"
#include "../Math/Vector2.h"
#include "../UI/Button.h"
#include "../UI/UiDrawCall.h"

#include "components/utilities/Span.h"

class Game;

enum class MouseButtonType;

struct Rect;
struct UiCommandList;

// Each panel interprets user input and draws to the screen. There is only one panel active at
// a time, and it is owned by Game, although there can be any number of sub-panels.
class Panel
{
private:
	Game &game;

	// Allocated input listener IDs that must be freed when the panel is done with them.
	std::vector<InputListenerID> inputActionListenerIDs;
	std::vector<InputListenerID> mouseButtonChangedListenerIDs;
	std::vector<InputListenerID> mouseButtonHeldListenerIDs;
	std::vector<InputListenerID> mouseScrollChangedListenerIDs;
	std::vector<InputListenerID> mouseMotionListenerIDs;
	std::vector<InputListenerID> textInputListenerIDs;

	std::vector<ButtonProxy> buttonProxies;

	// Registered draw calls that get evaluated every frame into a presentable texture + rect.
	std::vector<UiDrawCall> drawCalls;
	
	// Evaluated draw call results provided to the renderer every frame.
	std::vector<RenderElement2D> renderElementsCache;

	bool paused; // When not the top-most panel.
protected:
	Game &getGame() const;

	bool isPaused() const;

	void addInputActionListener(const std::string_view actionName, const InputActionCallback &callback);
	void addMouseButtonChangedListener(const MouseButtonChangedCallback &callback);
	void addMouseButtonHeldListener(const MouseButtonHeldCallback &callback);
	void addMouseScrollChangedListener(const MouseScrollChangedCallback &callback);
	void addMouseMotionListener(const MouseMotionCallback &callback);
	void addTextInputListener(const TextInputCallback &callback);

	// Adds a button proxy for a dynamic button (i.e. ListBox items).
	void addButtonProxy(MouseButtonType buttonType, const ButtonProxy::RectFunction &rectFunc,
		const ButtonProxy::Callback &callback, const Rect &parentRect = Rect(),
		const ButtonProxy::ActiveFunction &isActiveFunc = ButtonProxy::ActiveFunction());

	// Adds a button proxy for a static button.
	void addButtonProxy(MouseButtonType buttonType, const Rect &rect, const ButtonProxy::Callback &callback,
		const Rect &parentRect = Rect(), const ButtonProxy::ActiveFunction &isActiveFunc = ButtonProxy::ActiveFunction());

	void clearButtonProxies();

	void addDrawCall(const UiDrawCallInitInfo &initInfo);
	void addCursorDrawCall(UiTextureID textureID, UiPivotType pivotType, const UiDrawCallActiveFunc &activeFunc);
	void addCursorDrawCall(UiTextureID textureID, UiPivotType pivotType);

	void clearDrawCalls();
public:
	Panel(Game &game);
	virtual ~Panel();

	// Returns button proxies for ease of iteration and finding out which button is clicked in a frame
	// so its callback can be called.
	Span<const ButtonProxy> getButtonProxies() const;

	// Adds the registered UI draw calls from this panel. Each draw call is conditionally rendered
	// depending on whether it is active.
	void populateCommandList(UiCommandList &commandList);

	// Called when a sub-panel above this panel is pushed (added) or popped (removed).
	virtual void onPauseChanged(bool paused);

	// Called whenever the application window resizes. The panel should not handle
	// the resize event itself, since it's more of an "application event" than a
	// panel event, so it's handled in the game loop instead.
	virtual void resize(int windowWidth, int windowHeight);

	// Animates the panel by delta time. Override this method if a panel animates
	// in some form each frame without user input, or depends on things like a key
	// or a mouse button being held down.
	virtual void tick(double dt);
};

#endif
