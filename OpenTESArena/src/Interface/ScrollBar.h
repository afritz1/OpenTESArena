#ifndef SCROLL_BAR_H
#define SCROLL_BAR_H

#include "../Math/Rect.h"

class ScrollBar
{
private:
	Rect rect;
	int totalElements, visibleElements, stepIndex;

	int getTotalSteps() const;
	int getYOffset(int index) const; // The Y position of some step index.
	int getNextLowerIndex(int y) const; // The index for some truncated Y coordinate.
public:
	ScrollBar(const Rect &rect, int totalElements, int visibleElements);
	~ScrollBar();

	// The Y pixel coordinate at the top of the scroll bar.
	int getBarY() const;

	// The height of the scroll bar in pixels.
	int getBarHeight() const;

	// The total area the scroll bar can move in, in absolute 320x200 space.
	const Rect &getRect() const;

	// Move the scroll bar up one step. Does nothing if at the top.
	void stepUp();

	// Move the scroll bar down one step. Does nothing if at the bottom.
	void stepDown();

	// Sets the position of the scroll bar such that its center is as close
	// to the given Y coordinate as possible. The Y coordinate is expected to 
	// be in absolute 320x200 space.
	void centerAt(int y);
};

#endif
