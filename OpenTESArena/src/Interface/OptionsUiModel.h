#ifndef OPTIONS_UI_MODEL_H
#define OPTIONS_UI_MODEL_H

#include <functional>
#include <string>

namespace OptionsUiModel
{
	// Tabs.
	const std::string GRAPHICS_TAB_NAME = "Graphics";
	const std::string AUDIO_TAB_NAME = "Audio";
	const std::string INPUT_TAB_NAME = "Input";
	const std::string MISC_TAB_NAME = "Misc";
	const std::string DEV_TAB_NAME = "Dev";

	// Graphics.
	const std::string CURSOR_SCALE_NAME = "Cursor Scale";
	const std::string FPS_LIMIT_NAME = "FPS Limit";
	const std::string WINDOW_MODE_NAME = "Window Mode";
	const std::string LETTERBOX_MODE_NAME = "Letterbox Mode";
	const std::string MODERN_INTERFACE_NAME = "Modern Interface";
	const std::string RENDER_THREADS_MODE_NAME = "Render Threads Mode";
	const std::string RESOLUTION_SCALE_NAME = "Resolution Scale";
	const std::string VERTICAL_FOV_NAME = "Vertical FOV";

	// Audio.
	const std::string SOUND_CHANNELS_NAME = "Sound Channels";
	const std::string SOUND_RESAMPLING_NAME = "Sound Resampling";
	const std::string IS_3D_AUDIO_NAME = "Is 3D Audio";

	// Input.
	const std::string HORIZONTAL_SENSITIVITY_NAME = "Horizontal Sensitivity";
	const std::string VERTICAL_SENSITIVITY_NAME = "Vertical Sensitivity";
	const std::string CAMERA_PITCH_LIMIT_NAME = "Camera Pitch Limit";
	const std::string PIXEL_PERFECT_SELECTION_NAME = "Pixel-Perfect Selection";

	// Misc.
	const std::string SHOW_COMPASS_NAME = "Show Compass";
	const std::string SHOW_INTRO_NAME = "Show Intro";
	const std::string TIME_SCALE_NAME = "Time Scale";
	const std::string CHUNK_DISTANCE_NAME = "Chunk Distance";
	const std::string STAR_DENSITY_NAME = "Star Density";
	const std::string PLAYER_HAS_LIGHT_NAME = "Player Has Light";

	// Dev.
	const std::string COLLISION_NAME = "Collision";
	const std::string PROFILER_LEVEL_NAME = "Profiler Level";

	enum class OptionType { Bool, Int, Double, String };

	// This is the base class for all interactive options. Each option has a write-only interface
	// since the options panel shouldn't really store all the values itself; it's intended to be
	// a ferry between the UI and wherever in the program the options are actually used.
	class Option
	{
	private:
		const std::string &name; // Reference to global constant.
		std::string tooltip;
		OptionType type;
	public:
		Option(const std::string &name, std::string &&tooltip, OptionType type);
		Option(const std::string &name, OptionType type);
		virtual ~Option() = default;

		const std::string &getName() const;
		const std::string &getTooltip() const;
		OptionType getType() const;
		virtual std::string getDisplayedValue() const = 0;
	};

	class BoolOption : public Option
	{
	public:
		using Callback = std::function<void(bool)>;
	private:
		bool value;
		Callback callback;
	public:
		BoolOption(const std::string &name, std::string &&tooltip, bool value,
			Callback &&callback);
		BoolOption(const std::string &name, bool value, Callback &&callback);
		virtual ~BoolOption() = default;

		virtual std::string getDisplayedValue() const override;

		void toggle();
	};

	class IntOption : public Option
	{
	public:
		using Callback = std::function<void(int)>;
	private:
		int value, delta, min, max;
		Callback callback;
		std::vector<std::string> displayOverrides; // For displaying names instead of integers.
	public:
		IntOption(const std::string &name, std::string &&tooltip, int value, int delta,
			int min, int max, Callback &&callback);
		IntOption(const std::string &name, int value, int delta, int min, int max,
			Callback &&callback);
		virtual ~IntOption() = default;

		int getNext() const; // Adds delta to current value, clamped between [min, max].
		int getPrev() const; // Subtracts delta from current value, clamped between [min, max].
		virtual std::string getDisplayedValue() const override;

		void set(int value);
		void setDisplayOverrides(std::vector<std::string> &&displayOverrides);
	};

	class DoubleOption : public Option
	{
	public:
		using Callback = std::function<void(double)>;
	private:
		double value, delta, min, max;
		int precision;
		Callback callback;
	public:
		DoubleOption(const std::string &name, std::string &&tooltip, double value, double delta,
			double min, double max, int precision, Callback &&callback);
		DoubleOption(const std::string &name, double value, double delta, double min,
			double max, int precision, Callback &&callback);
		virtual ~DoubleOption() = default;

		double getNext() const; // Adds delta to current value, clamped between [min, max].
		double getPrev() const; // Subtracts delta from current value, clamped between [min, max].
		virtual std::string getDisplayedValue() const override;

		void set(double value);
	};

	class StringOption : public Option
	{
	public:
		using Callback = std::function<void(const std::string&)>;
	private:
		std::string value;
		Callback callback;
	public:
		StringOption(const std::string &name, std::string &&tooltip, std::string &&value,
			Callback &&callback);
		StringOption(const std::string &name, std::string &&value, Callback &&callback);
		virtual ~StringOption() = default;

		virtual std::string getDisplayedValue() const override;

		void set(std::string &&value);
	};
}

#endif
