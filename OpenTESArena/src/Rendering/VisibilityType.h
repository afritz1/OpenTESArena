#pragma once

// For testing objects against a visibility volume like a camera frustum.
enum class VisibilityType
{
	Outside, // Completely outside.
	Inside, // Completely inside.
	Partial // Some inside, some outside.
};
