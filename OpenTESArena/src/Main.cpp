#include <iostream>
#include <stdexcept>
#include <string>

#include "SDL.h"

#include "Game/Game.h"
#include "Utilities/Debug.h"

int main(int argc, char *argv[])
{
	static_cast<void>(argc);
	static_cast<void>(argv);

	try
	{
		Game g;
		g.loop();
	}
	catch (const std::exception &e)
	{
		DebugCrash("Exception! " + std::string(e.what()));
	}

	return EXIT_SUCCESS;
}
