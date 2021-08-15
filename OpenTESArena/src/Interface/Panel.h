#ifndef PANEL_H
#define PANEL_H

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "../Input/InputManager.h"
#include "../Math/Vector2.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureUtils.h"
#include "../UI/Button.h"
#include "../UI/UiDrawCall.h"

#include "components/utilities/BufferView.h"

// Each panel interprets user input and draws to the screen. There is only one panel active at
// a time, and it is owned by the Game, although there can be any number of sub-panels.

class Color;
class CursorData;
class FontLibrary;
class Game;
class Rect;
class Renderer;
class Texture;

enum class CursorAlignment;
enum class MouseButtonType;

struct SDL_Texture;

union SDL_Event;

class Panel
{
private:
	Game &game;

	// Allocated input listener IDs that must be freed when the panel is done with them.
	std::vector<InputManager::ListenerID> inputActionListenerIDs;
	std::vector<InputManager::ListenerID> mouseButtonChangedListenerIDs;
	std::vector<InputManager::ListenerID> mouseButtonHeldListenerIDs;
	std::vector<InputManager::ListenerID> mouseScrollChangedListenerIDs;
	std::vector<InputManager::ListenerID> mouseMotionListenerIDs;
	std::vector<InputManager::ListenerID> textInputListenerIDs;

	std::vector<ButtonProxy> buttonProxies;

	// Registered draw calls that will be iterated by the renderer.
	std::vector<UiDrawCall> drawCalls;
	// @todo: add a 'secondaryDrawCalls' list.
protected:
	Game &getGame() const;

	// Default cursor used by most panels.
	CursorData getDefaultCursor() const;

	void addInputActionListener(const std::string_view &actionName, const InputActionCallback &callback);
	void addMouseButtonChangedListener(const MouseButtonChangedCallback &callback);
	void addMouseButtonHeldListener(const MouseButtonHeldCallback &callback);
	void addMouseScrollChangedListener(const MouseScrollChangedCallback &callback);
	void addMouseMotionListener(const MouseMotionCallback &callback);
	void addTextInputListener(const TextInputCallback &callback);

	// Adds a button proxy for a dynamic button (i.e. ListBox items).
	void addButtonProxy(MouseButtonType buttonType, const ButtonProxy::RectFunction &rectFunc,
		const ButtonProxy::Callback &callback, const ButtonProxy::ActiveFunction &isActiveFunc = ButtonProxy::ActiveFunction());

	// Adds a button proxy for a static button.
	void addButtonProxy(MouseButtonType buttonType, const Rect &rect, const ButtonProxy::Callback &callback,
		const ButtonProxy::ActiveFunction &isActiveFunc = ButtonProxy::ActiveFunction());

	void clearButtonProxies();

	// Helper functions for registering UI draw calls.
	void addDrawCall(const UiDrawCall::TextureFunc &textureFunc, const UiDrawCall::RectFunc &rectFunc,
		const UiDrawCall::ActiveFunc &activeFunc, const std::optional<Rect> &clipRect = std::nullopt);
	void addDrawCall(const UiDrawCall::TextureFunc &textureFunc, const Rect &rect,
		const UiDrawCall::ActiveFunc &activeFunc, const std::optional<Rect> &clipRect = std::nullopt);
	void addDrawCall(const UiDrawCall::TextureFunc &textureFunc, const UiDrawCall::RectFunc &rectFunc,
		const std::optional<Rect> &clipRect = std::nullopt);
	void addDrawCall(const UiDrawCall::TextureFunc &textureFunc, const Rect &rect,
		const std::optional<Rect> &clipRect = std::nullopt);
	void addDrawCall(const UiDrawCall::TextureBuilderFunc &textureBuilderFunc, const UiDrawCall::RectFunc &rectFunc,
		const UiDrawCall::ActiveFunc &activeFunc, const std::optional<Rect> &clipRect = std::nullopt);
	void addDrawCall(const UiDrawCall::TextureBuilderFunc &textureBuilderFunc, const Rect &rect,
		const UiDrawCall::ActiveFunc &activeFunc, const std::optional<Rect> &clipRect = std::nullopt);
	void addDrawCall(const UiDrawCall::TextureBuilderFunc &textureBuilderFunc, const UiDrawCall::RectFunc &rectFunc,
		const std::optional<Rect> &clipRect = std::nullopt);
	void addDrawCall(const UiDrawCall::TextureBuilderFunc &textureBuilderFunc, const Rect &rect,
		const std::optional<Rect> &clipRect = std::nullopt);
	void addDrawCall(TextureBuilderID textureBuilderID, PaletteID paletteID, const UiDrawCall::RectFunc &rectFunc,
		const UiDrawCall::ActiveFunc &activeFunc, const std::optional<Rect> &clipRect = std::nullopt);
	void addDrawCall(TextureBuilderID textureBuilderID, PaletteID paletteID, const Rect &rect,
		const UiDrawCall::ActiveFunc &activeFunc, const std::optional<Rect> &clipRect = std::nullopt);
	void addDrawCall(TextureBuilderID textureBuilderID, PaletteID paletteID, const UiDrawCall::RectFunc &rectFunc,
		const std::optional<Rect> &clipRect = std::nullopt);
	void addDrawCall(TextureBuilderID textureBuilderID, PaletteID paletteID, const Rect &rect,
		const std::optional<Rect> &clipRect = std::nullopt);

	void clearDrawCalls();
public:
	Panel(Game &game);
	virtual ~Panel();

	// Gets the panel's active mouse cursor and alignment, if any. Override this if the panel has at
	// least one cursor defined.
	virtual std::optional<CursorData> getCurrentCursor() const;

	// Returns button proxies for ease of iteration and finding out which button is clicked in a frame
	// so its callback can be called.
	BufferView<const ButtonProxy> getButtonProxies() const;

	// Gets the registered UI draw calls for this panel. Each draw call is conditionally rendered
	// depending on whether it is active.
	BufferView<const UiDrawCall> getDrawCalls() const;

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

	// Draws the panel's main contents onto the display. Any contents that are hidden
	// when this panel is not the top-most one should go in renderSecondary().
	virtual void render(Renderer &renderer);

	// Draws the panel's secondary contents (pop-up text, tooltips, etc.). Does not
	// clear the frame buffer. This method is only called for the top-most panel, and
	// does nothing by default.
	virtual void renderSecondary(Renderer &renderer);
};

#endif
