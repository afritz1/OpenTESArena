#ifndef APPLICATION_EVENTS_H
#define APPLICATION_EVENTS_H

#include <functional>

// When the X button on the application window is selected.
using ApplicationExitCallback = std::function<void()>;

// When the application window has received a resize event from the operating system.
using WindowResizedCallback = std::function<void(int width, int height)>;

// When the application switches between desktop and exclusive fullscreen in some APIs like Direct3D.
using RenderTargetsResetCallback = std::function<void()>;

#endif
