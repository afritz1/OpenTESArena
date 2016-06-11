#include <iostream>

#include "SDL.h"

#include "Game/Game.h"

int main(int argc, char *argv[])
{
	// It shouldn't be taking any command line arguments; it loads from text files.
	static_cast<void>(argc);
	static_cast<void>(argv);

	Game g;
	g.loop();

	return EXIT_SUCCESS;
}
