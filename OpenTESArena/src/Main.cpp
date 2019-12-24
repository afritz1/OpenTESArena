#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "SDL.h"

#include "Game/Game.h"

#include "components/debug/Debug.h"
#include "Game/Collider3D.h"

int main(int argc, char *argv[])
{
	static_cast<void>(argc);
	static_cast<void>(argv);

	try
	{
		Collider3D::RunUnitTests();

		// Allocated on the heap to avoid stack overflow warning.
		auto g = std::make_unique<Game>();
		g->loop();
	}
	catch (const std::exception &e)
	{
		DebugCrash("Exception! " + std::string(e.what()));
	}

	return EXIT_SUCCESS;
}
