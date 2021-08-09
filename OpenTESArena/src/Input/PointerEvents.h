#ifndef POINTER_EVENTS_H
#define POINTER_EVENTS_H

#include <functional>

#include "PointerTypes.h"
#include "../Math/Vector2.h"

class Game;

// When a mouse button is pressed or released.
using MouseButtonChangedCallback = std::function<void(Game &game, MouseButtonType type, const Int2 &position, bool pressed)>;

// While a mouse button is held down.
using MouseButtonHeldCallback = std::function<void(Game &game, MouseButtonType type, const Int2 &position, double dt)>;

// When a mouse scroll wheel moves up or down.
using MouseScrollChangedCallback = std::function<void(Game &game, MouseWheelScrollType type, const Int2 &position)>;

// When the mouse cursor changes position on-screen.
// @todo: need to clarify if this is physical mouse position delta or on-screen mouse position delta.
using MouseMotionCallback = std::function<void(Game &game, int dx, int dy)>;

#endif
