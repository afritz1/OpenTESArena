#include "CommonUiController.h"
#include "../Game/Game.h"
#include "../Input/InputActionEvents.h"

void CommonUiController::onDebugInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		auto &game = values.game;
		auto &options = game.getOptions();

		// Increment or wrap profiler level.
		const int oldProfilerLevel = options.getMisc_ProfilerLevel();
		const int newProfilerLevel = (oldProfilerLevel < Options::MAX_PROFILER_LEVEL) ? (oldProfilerLevel + 1) : Options::MIN_PROFILER_LEVEL;
		options.setMisc_ProfilerLevel(newProfilerLevel);
	}
}