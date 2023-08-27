#ifndef VISIBILITY_TYPE_H
#define VISIBILITY_TYPE_H

// For testing objects against a visibility volume like a camera frustum.
enum class VisibilityType
{
	Outside, // Completely outside.
	Inside, // Completely inside.
	Partial // Some inside, some outside.
};

#endif
