#ifndef CURSOR_ALIGNMENT_H
#define CURSOR_ALIGNMENT_H

// A unique identifier for each mouse cursor offset. These define where the cursor 
// is drawn relative to the mouse position. It is most relevant for the arrow cursors 
// in the game world interface. The default for most cursors is 'top left'.
enum class CursorAlignment
{
	TopLeft,
	Top,
	TopRight,
	Left,
	Middle,
	Right,
	BottomLeft,
	Bottom,
	BottomRight
};

#endif
