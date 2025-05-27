#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "Jolt/Jolt.h"
#include "Jolt/Core/Factory.h"
#include "Jolt/RegisterTypes.h"
#include "SDL.h"

#include "Game/Game.h"
#include "Utilities/Platform.h"

#include "components/debug/Debug.h"

int main(int argc, char *argv[])
{
	static_cast<void>(argc);
	static_cast<void>(argv);

	const std::string logPath = Platform::getLogPath();
	if (!Debug::init(logPath.c_str()))
	{
		std::cerr << "Couldn't init debug logging.\n";
		return EXIT_FAILURE;
	}

	// Jolt Physics init.
	JPH::RegisterDefaultAllocator();
	JPH::Factory::sInstance = new JPH::Factory();
	JPH::RegisterTypes();

	try
	{
		// Allocated on the heap to avoid stack overflow warning.
		std::unique_ptr<Game> game = std::make_unique<Game>();
		if (!game->init())
		{
			DebugCrash("Couldn't init Game instance. Closing.");
		}

		game->loop();
	}
	catch (const std::exception &e)
	{
		DebugCrash("Exception: " + std::string(e.what()));
	}
	
	JPH::UnregisterTypes();
	if (JPH::Factory::sInstance != nullptr)
	{
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
	}

	Debug::shutdown();

	return EXIT_SUCCESS;
}
