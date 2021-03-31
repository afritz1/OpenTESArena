#ifndef TIMED_TEXT_BOX_H
#define TIMED_TEXT_BOX_H

#include <memory>

#include "TextBox.h"

struct TimedTextBox
{
	double remainingDuration;
	std::unique_ptr<TextBox> textBox;

	TimedTextBox(double remainingDuration, std::unique_ptr<TextBox> textBox);
	TimedTextBox();

	// Returns whether there's remaining duration.
	bool hasRemainingDuration() const;

	// Sets remaining duration to zero and empties the text box.
	void reset();
};

#endif
