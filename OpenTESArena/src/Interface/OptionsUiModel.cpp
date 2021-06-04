#include "OptionsUiModel.h"
#include "../Game/Game.h"

#include "components/utilities/String.h"

OptionsUiModel::Option::Option(const std::string &name, std::string &&tooltip, OptionType type)
	: name(name), tooltip(std::move(tooltip))
{
	this->type = type;
}

OptionsUiModel::Option::Option(const std::string &name, OptionType type)
	: Option(name, std::string(), type) { }

const std::string &OptionsUiModel::Option::getName() const
{
	return this->name;
}

const std::string &OptionsUiModel::Option::getTooltip() const
{
	return this->tooltip;
}

OptionsUiModel::OptionType OptionsUiModel::Option::getType() const
{
	return this->type;
}

OptionsUiModel::BoolOption::BoolOption(const std::string &name, std::string &&tooltip,
	bool value, Callback &&callback)
	: Option(name, std::move(tooltip), OptionType::Bool), callback(std::move(callback))
{
	this->value = value;
}

OptionsUiModel::BoolOption::BoolOption(const std::string &name, bool value, Callback &&callback)
	: BoolOption(name, std::string(), value, std::move(callback)) { }

std::string OptionsUiModel::BoolOption::getDisplayedValue() const
{
	return this->value ? "true" : "false";
}

void OptionsUiModel::BoolOption::toggle()
{
	this->value = !this->value;
	this->callback(this->value);
}

OptionsUiModel::IntOption::IntOption(const std::string &name, std::string &&tooltip, int value, int delta, int min,
	int max, std::vector<std::string> &&displayOverrides, Callback &&callback)
	: Option(name, std::move(tooltip), OptionType::Int), displayOverrides(std::move(displayOverrides)),
	callback(std::move(callback))
{
	this->value = value;
	this->delta = delta;
	this->min = min;
	this->max = max;
}

OptionsUiModel::IntOption::IntOption(const std::string &name, int value, int delta, int min, int max,
	std::vector<std::string> &&displayOverrides, Callback &&callback)
	: IntOption(name, std::string(), value, delta, min, max, std::move(displayOverrides), std::move(callback)) { }

OptionsUiModel::IntOption::IntOption(const std::string &name, std::string &&tooltip, int value, int delta,
	int min, int max, Callback &&callback)
	: IntOption(name, std::move(tooltip), value, delta, min, max, std::vector<std::string>(), std::move(callback)) { }

OptionsUiModel::IntOption::IntOption(const std::string &name, int value, int delta, int min, int max,
	Callback &&callback)
	: IntOption(name, std::string(), value, delta, min, max, std::move(callback)) { }

int OptionsUiModel::IntOption::getNext() const
{
	return std::min(this->value + this->delta, this->max);
}

int OptionsUiModel::IntOption::getPrev() const
{
	return std::max(this->value - this->delta, this->min);
}

std::string OptionsUiModel::IntOption::getDisplayedValue() const
{
	return (this->displayOverrides.size() > 0) ?
		this->displayOverrides.at(this->value) : std::to_string(this->value);
}

void OptionsUiModel::IntOption::set(int value)
{
	this->value = value;
	this->callback(this->value);
}

OptionsUiModel::DoubleOption::DoubleOption(const std::string &name, std::string &&tooltip,
	double value, double delta, double min, double max, int precision, Callback &&callback)
	: Option(name, std::move(tooltip), OptionType::Double), callback(std::move(callback))
{
	this->value = value;
	this->delta = delta;
	this->min = min;
	this->max = max;
	this->precision = precision;
}

OptionsUiModel::DoubleOption::DoubleOption(const std::string &name, double value, double delta,
	double min, double max, int precision, Callback &&callback)
	: DoubleOption(name, std::string(), value, delta, min, max, precision, std::move(callback)) { }

double OptionsUiModel::DoubleOption::getNext() const
{
	return std::min(this->value + this->delta, this->max);
}

double OptionsUiModel::DoubleOption::getPrev() const
{
	return std::max(this->value - this->delta, this->min);
}

std::string OptionsUiModel::DoubleOption::getDisplayedValue() const
{
	return String::fixedPrecision(this->value, this->precision);
}

void OptionsUiModel::DoubleOption::set(double value)
{
	this->value = value;
	this->callback(this->value);
}

OptionsUiModel::StringOption::StringOption(const std::string &name, std::string &&tooltip,
	std::string &&value, Callback &&callback)
	: Option(name, std::move(tooltip), OptionType::String), value(std::move(value)),
	callback(std::move(callback)) { }

OptionsUiModel::StringOption::StringOption(const std::string &name, std::string &&value,
	Callback &&callback)
	: StringOption(name, std::string(), std::move(value), std::move(callback)) { }

std::string OptionsUiModel::StringOption::getDisplayedValue() const
{
	return this->value;
}

void OptionsUiModel::StringOption::set(std::string &&value)
{
	this->value = std::move(value);
	this->callback(this->value);
}

std::unique_ptr<OptionsUiModel::IntOption> OptionsUiModel::makeWindowModeOption(Game &game)
{
	const auto &options = game.getOptions();
	return std::make_unique<OptionsUiModel::IntOption>(
		OptionsUiModel::WINDOW_MODE_NAME,
		options.getGraphics_WindowMode(),
		1,
		Options::MIN_WINDOW_MODE,
		Options::MAX_WINDOW_MODE,
		std::vector<std::string> { "Window", "Borderless Full" },
		[&game](int value)
	{
		auto &options = game.getOptions();
		auto &renderer = game.getRenderer();
		options.setGraphics_WindowMode(value);

		const Renderer::WindowMode mode = [value]()
		{
			switch (value)
			{
			case 0:
				return Renderer::WindowMode::Window;
			case 1:
				return Renderer::WindowMode::BorderlessFull;
			default:
				DebugUnhandledReturnMsg(Renderer::WindowMode, std::to_string(value));
			}
		}();

		renderer.setWindowMode(mode);
	});
}

std::unique_ptr<OptionsUiModel::IntOption> OptionsUiModel::makeFpsLimitOption(Game &game)
{
	const auto &options = game.getOptions();
	return std::make_unique<OptionsUiModel::IntOption>(
		OptionsUiModel::FPS_LIMIT_NAME,
		options.getGraphics_TargetFPS(),
		5,
		Options::MIN_FPS,
		std::numeric_limits<int>::max(),
		[&game](int value)
	{
		auto &options = game.getOptions();
		options.setGraphics_TargetFPS(value);
	});
}

std::unique_ptr<OptionsUiModel::DoubleOption> OptionsUiModel::makeResolutionScaleOption(Game &game)
{
	const auto &options = game.getOptions();
	return std::make_unique<OptionsUiModel::DoubleOption>(
		OptionsUiModel::RESOLUTION_SCALE_NAME,
		"Percent of the window resolution to use for software rendering.\nThis has a significant impact on performance.",
		options.getGraphics_ResolutionScale(),
		0.050,
		Options::MIN_RESOLUTION_SCALE,
		Options::MAX_RESOLUTION_SCALE,
		2,
		[&game](double value)
	{
		auto &options = game.getOptions();
		options.setGraphics_ResolutionScale(value);

		// Resize the game world rendering.
		auto &renderer = game.getRenderer();
		const Int2 windowDimensions = renderer.getWindowDimensions();
		const bool fullGameWindow = options.getGraphics_ModernInterface();
		renderer.resize(windowDimensions.x, windowDimensions.y, value, fullGameWindow);
	});
}

std::unique_ptr<OptionsUiModel::DoubleOption> OptionsUiModel::makeVerticalFovOption(Game &game)
{
	const auto &options = game.getOptions();
	return std::make_unique<OptionsUiModel::DoubleOption>(
		OptionsUiModel::VERTICAL_FOV_NAME,
		"Recommended 60.0 for classic mode.",
		options.getGraphics_VerticalFOV(),
		5.0,
		Options::MIN_VERTICAL_FOV,
		Options::MAX_VERTICAL_FOV,
		1,
		[&game](double value)
	{
		auto &options = game.getOptions();
		options.setGraphics_VerticalFOV(value);
	});
}

std::unique_ptr<OptionsUiModel::IntOption> OptionsUiModel::makeLetterboxModeOption(Game &game)
{
	const auto &options = game.getOptions();
	return std::make_unique<OptionsUiModel::IntOption>(
		OptionsUiModel::LETTERBOX_MODE_NAME,
		"Determines the aspect ratio of the game UI. The weapon animation\nin modern mode is unaffected by this.",
		options.getGraphics_LetterboxMode(),
		1,
		Options::MIN_LETTERBOX_MODE,
		Options::MAX_LETTERBOX_MODE,
		std::vector<std::string> { "16:10", "4:3", "Stretch" },
		[&game](int value)
	{
		auto &options = game.getOptions();
		auto &renderer = game.getRenderer();
		options.setGraphics_LetterboxMode(value);
		renderer.setLetterboxMode(value);
	});
}

std::unique_ptr<OptionsUiModel::DoubleOption> OptionsUiModel::makeCursorScaleOption(Game &game)
{
	const auto &options = game.getOptions();
	return std::make_unique<OptionsUiModel::DoubleOption>(
		OptionsUiModel::CURSOR_SCALE_NAME,
		options.getGraphics_CursorScale(),
		0.10,
		Options::MIN_CURSOR_SCALE,
		Options::MAX_CURSOR_SCALE,
		1,
		[&game](double value)
	{
		auto &options = game.getOptions();
		options.setGraphics_CursorScale(value);
	});
}

std::unique_ptr<OptionsUiModel::BoolOption> OptionsUiModel::makeModernInterfaceOption(Game &game)
{
	const auto &options = game.getOptions();
	return std::make_unique<OptionsUiModel::BoolOption>(
		OptionsUiModel::MODERN_INTERFACE_NAME,
		"Modern mode uses a minimal interface with free-look.",
		options.getGraphics_ModernInterface(),
		[&game](bool value)
	{
		auto &options = game.getOptions();
		options.setGraphics_ModernInterface(value);

		// If classic mode, make sure the player is looking straight forward.
		// This is a restriction on the camera to retain the original feel.
		const bool isModernMode = value;
		if (!isModernMode)
		{
			auto &player = game.getGameState().getPlayer();
			player.setDirectionToHorizon();
		}

		// Resize the game world rendering.
		auto &renderer = game.getRenderer();
		const Int2 windowDims = renderer.getWindowDimensions();
		const bool fullGameWindow = isModernMode;
		renderer.resize(windowDims.x, windowDims.y, options.getGraphics_ResolutionScale(), fullGameWindow);
	});
}

std::unique_ptr<OptionsUiModel::IntOption> OptionsUiModel::makeRenderThreadsModeOption(Game &game)
{
	const auto &options = game.getOptions();
	return std::make_unique<OptionsUiModel::IntOption>(
		OptionsUiModel::RENDER_THREADS_MODE_NAME,
		"Determines the number of CPU threads to use for rendering.\nThis has a significant impact on performance.\nVery Low: one, Low: 1/4, Medium: 1/2, High: 3/4,\nVery High: all but one, Max: all",
		options.getGraphics_RenderThreadsMode(),
		1,
		Options::MIN_RENDER_THREADS_MODE,
		Options::MAX_RENDER_THREADS_MODE,
		std::vector<std::string> { "Very Low", "Low", "Medium", "High", "Very High", "Max" },
		[&game](int value)
	{
		auto &options = game.getOptions();
		auto &renderer = game.getRenderer();
		options.setGraphics_RenderThreadsMode(value);
		renderer.setRenderThreadsMode(value);
	});
}

std::unique_ptr<OptionsUiModel::IntOption> OptionsUiModel::makeSoundChannelsOption(Game &game)
{
	const auto &options = game.getOptions();
	return std::make_unique<OptionsUiModel::IntOption>(
		OptionsUiModel::SOUND_CHANNELS_NAME,
		"Determines max number of concurrent sounds (including music).\nChanges are applied on next program start.",
		options.getAudio_SoundChannels(),
		1,
		Options::MIN_SOUND_CHANNELS,
		std::numeric_limits<int>::max(),
		[&game](int value)
	{
		auto &options = game.getOptions();
		options.setAudio_SoundChannels(value);
	});
}

std::unique_ptr<OptionsUiModel::IntOption> OptionsUiModel::makeSoundResamplingOption(Game &game)
{
	const auto &options = game.getOptions();
	return std::make_unique<OptionsUiModel::IntOption>(
		OptionsUiModel::SOUND_RESAMPLING_NAME,
		"Affects quality of sounds. Results may vary depending on OpenAL\nversion.",
		options.getAudio_SoundResampling(),
		1,
		0,
		Options::RESAMPLING_OPTION_COUNT - 1,
		std::vector<std::string> { "Default", "Fastest", "Medium", "Best" },
		[&game](int value)
	{
		auto &options = game.getOptions();
		options.setAudio_SoundResampling(value);

		// If the sound resampling extension is supported, update the audio manager sources.
		auto &audioManager = game.getAudioManager();
		if (audioManager.hasResamplerExtension())
		{
			audioManager.setResamplingOption(value);
		}
	});
}

std::unique_ptr<OptionsUiModel::BoolOption> OptionsUiModel::makeIs3dAudioOption(Game &game)
{
	const auto &options = game.getOptions();
	return std::make_unique<OptionsUiModel::BoolOption>(
		OptionsUiModel::IS_3D_AUDIO_NAME,
		"Determines whether sounds in the game world have a 3D position.\nSet to false for classic behavior.",
		options.getAudio_Is3DAudio(),
		[&game](bool value)
	{
		auto &options = game.getOptions();
		options.setAudio_Is3DAudio(value);

		auto &audioManager = game.getAudioManager();
		audioManager.set3D(value);
	});
}

std::unique_ptr<OptionsUiModel::DoubleOption> OptionsUiModel::makeHorizontalSensitivityOption(Game &game)
{
	const auto &options = game.getOptions();
	return std::make_unique<OptionsUiModel::DoubleOption>(
		OptionsUiModel::HORIZONTAL_SENSITIVITY_NAME,
		options.getInput_HorizontalSensitivity(),
		0.10,
		Options::MIN_HORIZONTAL_SENSITIVITY,
		Options::MAX_HORIZONTAL_SENSITIVITY,
		1,
		[&game](double value)
	{
		auto &options = game.getOptions();
		options.setInput_HorizontalSensitivity(value);
	});
}

std::unique_ptr<OptionsUiModel::DoubleOption> OptionsUiModel::makeVerticalSensitivityOption(Game &game)
{
	const auto &options = game.getOptions();
	return std::make_unique<OptionsUiModel::DoubleOption>(
		OptionsUiModel::VERTICAL_SENSITIVITY_NAME,
		"Only affects camera look in modern mode.",
		options.getInput_VerticalSensitivity(),
		0.10,
		Options::MIN_VERTICAL_SENSITIVITY,
		Options::MAX_VERTICAL_SENSITIVITY,
		1,
		[&game](double value)
	{
		auto &options = game.getOptions();
		options.setInput_VerticalSensitivity(value);
	});
}

std::unique_ptr<OptionsUiModel::DoubleOption> OptionsUiModel::makeCameraPitchLimitOption(Game &game)
{
	const auto &options = game.getOptions();
	return std::make_unique<OptionsUiModel::DoubleOption>(
		OptionsUiModel::CAMERA_PITCH_LIMIT_NAME,
		"Determines how far above or below the horizon the camera can\nlook in modern mode.",
		options.getInput_CameraPitchLimit(),
		5.0,
		Options::MIN_CAMERA_PITCH_LIMIT,
		Options::MAX_CAMERA_PITCH_LIMIT,
		1,
		[&game](double value)
	{
		auto &options = game.getOptions();
		options.setInput_CameraPitchLimit(value);

		// Reset player view to forward.
		auto &player = game.getGameState().getPlayer();
		player.setDirectionToHorizon();
	});
}

std::unique_ptr<OptionsUiModel::BoolOption> OptionsUiModel::makePixelPerfectSelectionOption(Game &game)
{
	const auto &options = game.getOptions();
	return std::make_unique<OptionsUiModel::BoolOption>(
		OptionsUiModel::PIXEL_PERFECT_SELECTION_NAME,
		"Changes entity selection so only clicks on opaque places are\nregistered, if enabled.",
		options.getInput_PixelPerfectSelection(),
		[&game](bool value)
	{
		auto &options = game.getOptions();
		options.setInput_PixelPerfectSelection(value);
	});
}

std::unique_ptr<OptionsUiModel::BoolOption> OptionsUiModel::makeShowCompassOption(Game &game)
{
	const auto &options = game.getOptions();
	return std::make_unique<OptionsUiModel::BoolOption>(
		OptionsUiModel::SHOW_COMPASS_NAME,
		options.getMisc_ShowCompass(),
		[&game](bool value)
	{
		auto &options = game.getOptions();
		options.setMisc_ShowCompass(value);
	});
}

std::unique_ptr<OptionsUiModel::BoolOption> OptionsUiModel::makeShowIntroOption(Game &game)
{
	const auto &options = game.getOptions();
	return std::make_unique<OptionsUiModel::BoolOption>(
		OptionsUiModel::SHOW_INTRO_NAME,
		"Shows startup logo and related screens.",
		options.getMisc_ShowIntro(),
		[&game](bool value)
	{
		auto &options = game.getOptions();
		options.setMisc_ShowIntro(value);
	});
}

std::unique_ptr<OptionsUiModel::DoubleOption> OptionsUiModel::makeTimeScaleOption(Game &game)
{
	const auto &options = game.getOptions();
	return std::make_unique<OptionsUiModel::DoubleOption>(
		OptionsUiModel::TIME_SCALE_NAME,
		"Affects speed of gameplay. Lower this to simulate the speed of\nlower cycles in DOSBox.",
		options.getMisc_TimeScale(),
		0.050,
		Options::MIN_TIME_SCALE,
		Options::MAX_TIME_SCALE,
		2,
		[&game](double value)
	{
		auto &options = game.getOptions();
		options.setMisc_TimeScale(value);
	});
}

std::unique_ptr<OptionsUiModel::IntOption> OptionsUiModel::makeChunkDistanceOption(Game &game)
{
	const auto &options = game.getOptions();
	return std::make_unique<OptionsUiModel::IntOption>(
		OptionsUiModel::CHUNK_DISTANCE_NAME,
		"Affects how many chunks away from the player chunks are\nsimulated and rendered.",
		options.getMisc_ChunkDistance(),
		1,
		Options::MIN_CHUNK_DISTANCE,
		std::numeric_limits<int>::max(),
		[&game](int value)
	{
		auto &options = game.getOptions();
		options.setMisc_ChunkDistance(value);
	});
}

std::unique_ptr<OptionsUiModel::IntOption> OptionsUiModel::makeStarDensityOption(Game &game)
{
	const auto &options = game.getOptions();
	return std::make_unique<OptionsUiModel::IntOption>(
		OptionsUiModel::STAR_DENSITY_NAME,
		"Determines number of stars in the sky. Changes take effect the next\ntime stars are generated.",
		options.getMisc_StarDensity(),
		1,
		Options::MIN_STAR_DENSITY_MODE,
		Options::MAX_STAR_DENSITY_MODE,
		std::vector<std::string> { "Classic", "Moderate", "High" },
		[&game](int value)
	{
		auto &options = game.getOptions();
		options.setMisc_StarDensity(value);
	});
}

std::unique_ptr<OptionsUiModel::BoolOption> OptionsUiModel::makePlayerHasLightOption(Game &game)
{
	const auto &options = game.getOptions();
	return std::make_unique<OptionsUiModel::BoolOption>(
		OptionsUiModel::PLAYER_HAS_LIGHT_NAME,
		"Whether the player has a light attached like in the original game.",
		options.getMisc_PlayerHasLight(),
		[&game](bool value)
	{
		auto &options = game.getOptions();
		options.setMisc_PlayerHasLight(value);
	});
}

std::unique_ptr<OptionsUiModel::BoolOption> OptionsUiModel::makeCollisionOption(Game &game)
{
	const auto &options = game.getOptions();
	return std::make_unique<OptionsUiModel::BoolOption>(
		OptionsUiModel::COLLISION_NAME,
		"Enables player collision (not fully implemented yet).",
		options.getMisc_Collision(),
		[&game](bool value)
	{
		auto &options = game.getOptions();
		options.setMisc_Collision(value);
	});
}

std::unique_ptr<OptionsUiModel::IntOption> OptionsUiModel::makeProfilerLevelOption(Game &game)
{
	const auto &options = game.getOptions();
	return std::make_unique<OptionsUiModel::IntOption>(
		OptionsUiModel::PROFILER_LEVEL_NAME,
		"Displays varying levels of profiler information in the game world.",
		options.getMisc_ProfilerLevel(),
		1,
		Options::MIN_PROFILER_LEVEL,
		Options::MAX_PROFILER_LEVEL,
		[&game](int value)
	{
		auto &options = game.getOptions();
		options.setMisc_ProfilerLevel(value);
	});
}
