#ifndef PANEL_H
#define PANEL_H

// Should all panels have a pointer to the current game state, like in the OpenXcom 
// code? Does it break the model/view/controller pattern? Well, the panel is essentially 
// the view and the controller, and when it obtains input, it needs to change the state 
// of the model. 

// Giving a game state reference to the buttons seems logical. Is there any point where 
// something would write to the game state, and then be read from in the same frame? 
// Nothing in the panel should be reading from the game state that isn't already visible
// to itself.

// How might "continued" text boxes work? Arena has some pop-up text boxes that have
// multiple screens based on the amount of text, and even some buttons like "yes/no" on
// the last screen. I think I'll just replace them with scrolled text boxes. The buttons
// can be separate interface objects (no need for a "ScrollableButtonedTextBox").

class Panel
{
protected:

public:
	Panel();
	virtual ~Panel();
};

#endif