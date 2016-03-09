#include <iostream>

#include "SDL2\SDL.h"

int main(int argc, char *argv[])
{
	// It shouldn't be taking any command line arguments; it loads the same every time.
	(void)argc;
	(void)argv;

	// Game code will go here at some point, once enough components are built.

	std::getchar();
	return EXIT_SUCCESS;
}
