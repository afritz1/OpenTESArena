#ifndef OPTIONS_PANEL_H
#define OPTIONS_PANEL_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Button.h"
#include "Panel.h"
#include "../Math/Vector2.h"

class AudioManager;
class Options;
class Player;
class Renderer;
class TextBox;

enum class PlayerInterface;

class OptionsPanel : public Panel
{
private:
	// This is the base class for all interactive options. Each option has a write-only interface
	// since the options panel shouldn't really store all the values itself; it's intended to be
	// a ferry between the UI and wherever in the program the options are actually used.
	class Option
	{
	public:
		enum class Type { Bool, Int, Double, String };
	private:
		const std::string &name; // Reference to global constant.
		std::string tooltip;
		Type type;
	public:
		Option(const std::string &name, std::string &&tooltip, Type type);
		Option(const std::string &name, Type type);
		virtual ~Option() = default;

		const std::string &getName() const;
		const std::string &getTooltip() const;
		Type getType() const;
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

	// Options panel tabs.
	enum class Tab { Graphics, Audio, Input, Misc, Dev };

	// Tabs.
	static const std::string GRAPHICS_TAB_NAME;
	static const std::string AUDIO_TAB_NAME;
	static const std::string INPUT_TAB_NAME;
	static const std::string MISC_TAB_NAME;
	static const std::string DEV_TAB_NAME;

	// Graphics.
	static const std::string CURSOR_SCALE_NAME;
	static const std::string FPS_LIMIT_NAME;
	static const std::string WINDOW_MODE_NAME;
	static const std::string LETTERBOX_MODE_NAME;
	static const std::string MODERN_INTERFACE_NAME;
	static const std::string RENDER_THREADS_MODE_NAME;
	static const std::string RESOLUTION_SCALE_NAME;
	static const std::string VERTICAL_FOV_NAME;

	// Audio.
	static const std::string SOUND_CHANNELS_NAME;
	static const std::string SOUND_RESAMPLING_NAME;
	static const std::string IS_3D_AUDIO_NAME;

	// Input.
	static const std::string HORIZONTAL_SENSITIVITY_NAME;
	static const std::string VERTICAL_SENSITIVITY_NAME;
	static const std::string CAMERA_PITCH_LIMIT_NAME;
	static const std::string PIXEL_PERFECT_SELECTION_NAME;

	// Misc.
	static const std::string SHOW_COMPASS_NAME;
	static const std::string SHOW_INTRO_NAME;
	static const std::string TIME_SCALE_NAME;
	static const std::string CHUNK_DISTANCE_NAME;
	static const std::string STAR_DENSITY_NAME;
	static const std::string PLAYER_HAS_LIGHT_NAME;

	// Dev.
	static const std::string COLLISION_NAME;
	static const std::string PROFILER_LEVEL_NAME;

	std::unique_ptr<TextBox> titleTextBox, backToPauseMenuTextBox, graphicsTextBox, audioTextBox,
		inputTextBox, miscTextBox, devTextBox;
	Button<Game&> backToPauseMenuButton;
	Button<OptionsPanel&, OptionsPanel::Tab> tabButton;
	std::vector<std::unique_ptr<OptionsPanel::Option>> graphicsOptions, audioOptions,
		inputOptions, miscOptions, devOptions;
	std::vector<std::unique_ptr<TextBox>> currentTabTextBoxes;
	OptionsPanel::Tab tab;

	// Gets the visible options group based on the current tab.
	std::vector<std::unique_ptr<OptionsPanel::Option>> &getVisibleOptions();

	// Regenerates option text for one option.
	void updateOptionTextBox(int index);

	// Regenerates all option text boxes in the current tab.
	void updateVisibleOptionTextBoxes();

	// Draws return buttons and tabs.
	void drawReturnButtonsAndTabs(Renderer &renderer);

	// Draws the text of the buttons.
	void drawText(Renderer &renderer);

	// Draw each option's text.
	void drawTextOfOptions(Renderer &renderer);

	// Draws description for an option. Not using mouse tooltips because they
	// get in the way of seeing what an option's value is.
	void drawDescription(const std::string &text, Renderer &renderer);
public:
	OptionsPanel(Game &game);
	virtual ~OptionsPanel() = default;

	virtual std::optional<Panel::CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
