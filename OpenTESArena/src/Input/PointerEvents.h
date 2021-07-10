#ifndef POINTER_EVENTS_H
#define POINTER_EVENTS_H

#include <functional>

#include "PointerTypes.h"
#include "../Math/Vector2.h"

// When a mouse button is pressed or released.
using MouseButtonChangedCallback = std::function<void(MouseButtonType type, const Int2 &position, bool pressed)>;

// While a mouse button is held down.
using MouseButtonHeldCallback = std::function<void(MouseButtonType type, const Int2 &position)>;

// When a mouse scroll wheel moves up or down.
using MouseScrollChangedCallback = std::function<void(MouseWheelScrollType type, const Int2 &position)>;

// When the mouse cursor changes position on-screen.
using MousePositionDeltaCallback = std::function<void(int dx, int dy)>;

#endif
