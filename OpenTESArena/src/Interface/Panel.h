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
protected:
	// Allocated input listener IDs that must be freed when the panel is done with them.
	std::vector<InputManager::ListenerID> inputActionListenerIDs;
	std::vector<InputManager::ListenerID> mouseButtonChangedListenerIDs;
	std::vector<InputManager::ListenerID> mouseButtonHeldListenerIDs;
	std::vector<InputManager::ListenerID> mouseScrollChangedListenerIDs;
	std::vector<InputManager::ListenerID> mouseMotionListenerIDs;

	std::vector<ButtonProxy> buttonProxies;

	Game &getGame() const;

	// Default cursor used by most panels.
	CursorData getDefaultCursor() const;

	void addInputActionListener(const std::string_view &actionName, const InputActionCallback &callback);
	void addMouseButtonChangedListener(const MouseButtonChangedCallback &callback);
	void addMouseButtonHeldListener(const MouseButtonHeldCallback &callback);
	void addMouseScrollChangedListener(const MouseScrollChangedCallback &callback);
	void addMouseMotionListener(const MouseMotionCallback &callback);

	// Adds a button proxy for a dynamic button (i.e. ListBox items).
	void addButtonProxy(MouseButtonType buttonType, const ButtonProxy::RectFunction &rectFunc,
		const ButtonProxy::Callback &callback, const ButtonProxy::ActiveFunction &isActiveFunc = ButtonProxy::ActiveFunction());

	// Adds a button proxy for a static button.
	void addButtonProxy(MouseButtonType buttonType, const Rect &rect, const ButtonProxy::Callback &callback,
		const ButtonProxy::ActiveFunction &isActiveFunc = ButtonProxy::ActiveFunction());

	void clearButtonProxies();
public:
	Panel(Game &game);
	virtual ~Panel();

	// Gets the panel's active mouse cursor and alignment, if any. Override this if the panel has at
	// least one cursor defined.
	virtual std::optional<CursorData> getCurrentCursor() const;

	// Handles panel-specific events. Application events like closing and resizing
	// are handled by the game loop.
	virtual void handleEvent(const SDL_Event &e);

	// Returns button proxies for ease of iteration and finding out which button is clicked in a frame
	// so its callback can be called.
	virtual BufferView<const ButtonProxy> getButtonProxies() const;

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
	virtual void render(Renderer &renderer) = 0;

	// Draws the panel's secondary contents (pop-up text, tooltips, etc.). Does not
	// clear the frame buffer. This method is only called for the top-most panel, and
	// does nothing by default.
	virtual void renderSecondary(Renderer &renderer);
};

#endif
