#include <iostream>

#include "SDL2\SDL.h"

#include "Game\Game.h"

int main(int argc, char *argv[])
{
	// It shouldn't be taking any command line arguments; it loads the same every time.
	static_cast<void>(argc);
	static_cast<void>(argv);
	
	Game g;
	g.loop();

	return EXIT_SUCCESS;
}
