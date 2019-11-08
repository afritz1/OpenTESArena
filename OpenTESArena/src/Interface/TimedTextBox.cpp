#include "TimedTextBox.h"

TimedTextBox::TimedTextBox(double remainingDuration, std::unique_ptr<TextBox> textBox)
	: textBox(std::move(textBox))
{
	this->remainingDuration = remainingDuration;
}

TimedTextBox::TimedTextBox()
	: TimedTextBox(0.0, nullptr) { }

bool TimedTextBox::hasRemainingDuration() const
{
	return this->remainingDuration > 0.0;
}

void TimedTextBox::reset()
{
	this->remainingDuration = 0.0;
	this->textBox = nullptr;
}