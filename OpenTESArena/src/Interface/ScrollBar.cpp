#include "ScrollBar.h"
#include "../Utilities/Debug.h"

ScrollBar::ScrollBar(const Rect &rect, int totalElements, int visibleElements)
	: rect(rect)
{
	this->totalElements = totalElements;
	this->visibleElements = visibleElements;
	this->stepIndex = 0;
}

ScrollBar::~ScrollBar()
{

}

int ScrollBar::getTotalSteps() const
{
	DebugNotImplemented();
	return -1;
}

int ScrollBar::getYOffset(int index) const
{
	DebugNotImplemented();
	return -1;
}

int ScrollBar::getNextLowerIndex(int y) const
{
	// Determines which step offset the bar is at for some Y coordinate.
	// Probably will involve integer truncation.
	DebugNotImplemented();
	return -1;
}

int ScrollBar::getBarY() const
{
	// Function of current step, total step, and rect.
	DebugNotImplemented();
	return -1;
}

int ScrollBar::getBarHeight() const
{
	// Function of currentVisible / totalVisible.
	DebugNotImplemented();
	return -1;
}

const Rect &ScrollBar::getRect() const
{
	return this->rect;
}

void ScrollBar::stepUp()
{
	DebugNotImplemented();
}

void ScrollBar::stepDown()
{
	DebugNotImplemented();
}

void ScrollBar::centerAt(int y)
{
	// Probably will use getNextLowerIndex().
	DebugNotImplemented();
}
