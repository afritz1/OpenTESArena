#ifndef BUTTON_H
#define BUTTON_H

#include <functional>

#include "Surface.h"

class Button : public Surface
{
private:
	std::function<void()> function;
public:
	Button(int x, int y, int width, int height, const std::function<void()> &function);
	Button(const Int2 &center, int width, int height,
		const std::function<void()> &function);

	// "Hidden" button for hotkeys. It is not intended to be drawn anywhere.
	Button(const std::function<void()> &function);

	virtual ~Button();

	void click();
};

#endif
