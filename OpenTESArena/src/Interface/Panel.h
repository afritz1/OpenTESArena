#ifndef PANEL_H
#define PANEL_H

#include <memory>
#include <string>

#include "../Math/Vector2.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureUtils.h"

// Each panel interprets user input and draws to the screen. There is only one panel 
// active at a time, and it is owned by the Game.

// How might "continued" text boxes work? Arena has some pop-up text boxes that have
// multiple screens based on the amount of text, and even some buttons like "yes/no" on
// the last screen. I think I'll just replace them with scrolled text boxes. The buttons
// can be separate interface objects (no need for a "ScrollableButtonedTextBox").

class Color;
class FontLibrary;
class Game;
class Renderer;
class Texture;

enum class CursorAlignment;
enum class FontName;

struct SDL_Texture;

union SDL_Event;

class Panel
{
public:
	class CursorData // @todo: rename to CursorDisplayState?
	{
	private:
		TextureBuilderID textureBuilderID; // @todo: maybe should be a UI texture handle at some point.
		PaletteID paletteID;
		CursorAlignment alignment;
	public:
		CursorData(TextureBuilderID textureBuilderID, PaletteID paletteID, CursorAlignment alignment);

		TextureBuilderID getTextureBuilderID() const;
		PaletteID getPaletteID() const;
		CursorAlignment getAlignment() const;
	};
private:
	Game &game;
protected:
	// Generates a tooltip texture with the default white foreground and gray
	// background with alpha blending.
	static Texture createTooltip(const std::string &text,
		FontName fontName, FontLibrary &fontLibrary, Renderer &renderer);

	Game &getGame() const;

	// Default cursor used by most panels.
	CursorData getDefaultCursor() const;
public:
	Panel(Game &game);
	virtual ~Panel() = default;

	static std::unique_ptr<Panel> defaultPanel(Game &game);

	// Gets the panel's active mouse cursor and alignment, if any. Override this if the panel has at
	// least one cursor defined.
	virtual std::optional<CursorData> getCurrentCursor() const;

	// Handles panel-specific events. Application events like closing and resizing
	// are handled by the game loop.
	virtual void handleEvent(const SDL_Event &e);

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
