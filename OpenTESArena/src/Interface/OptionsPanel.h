#ifndef OPTIONS_PANEL_H
#define OPTIONS_PANEL_H

#include <functional>

#include "Button.h"
#include "Panel.h"

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

		const std::string &getName() const;
		const std::string &getTooltip() const;
		Type getType() const;
		virtual std::string getDisplayedValue() const = 0;
	};

	class BoolOption : public Option
	{
	public:
		typedef std::function<void(bool)> Callback;
	private:
		bool value;
		Callback callback;
	public:
		BoolOption(const std::string &name, std::string &&tooltip, bool value,
			Callback &&callback);
		BoolOption(const std::string &name, bool value, Callback &&callback);

		virtual std::string getDisplayedValue() const override;

		void toggle();
	};

	class IntOption : public Option
	{
	public:
		typedef std::function<void(int)> Callback;
	private:
		int value, delta, min, max;
		Callback callback;
	public:
		IntOption(const std::string &name, std::string &&tooltip, int value, int delta,
			int min, int max, Callback &&callback);
		IntOption(const std::string &name, int value, int delta, int min, int max,
			Callback &&callback);

		int getNext() const; // Adds delta to current value, clamped between [min, max].
		int getPrev() const; // Subtracts delta from current value, clamped between [min, max].
		virtual std::string getDisplayedValue() const override;

		void set(int value);
	};

	class DoubleOption : public Option
	{
	public:
		typedef std::function<void(double)> Callback;
	private:
		double value, delta, min, max;
		int precision;
		Callback callback;
	public:
		DoubleOption(const std::string &name, std::string &&tooltip, double value, double delta,
			double min, double max, int precision, Callback &&callback);
		DoubleOption(const std::string &name, double value, double delta, double min,
			double max, int precision, Callback &&callback);

		double getNext() const; // Adds delta to current value, clamped between [min, max].
		double getPrev() const; // Subtracts delta from current value, clamped between [min, max].
		virtual std::string getDisplayedValue() const override;

		void set(double value);
	};

	class StringOption : public Option
	{
	public:
		typedef std::function<void(const std::string&)> Callback;
	private:
		std::string value;
		Callback callback;
	public:
		StringOption(const std::string &name, std::string &&tooltip, std::string &&value,
			Callback &&callback);
		StringOption(const std::string &name, std::string &&value, Callback &&callback);

		virtual std::string getDisplayedValue() const override;

		void set(std::string &&value);
	};

	static const int TOGGLE_BUTTON_SIZE;
	static const std::string FPS_TEXT;
	static const std::string RESOLUTION_SCALE_TEXT;
	static const std::string PLAYER_INTERFACE_TEXT;
	static const std::string VERTICAL_FOV_TEXT;
	static const std::string CURSOR_SCALE_TEXT;
	static const std::string LETTERBOX_ASPECT_TEXT;
	static const std::string HORIZONTAL_SENSITIVITY_TEXT;
	static const std::string VERTICAL_SENSITIVITY_TEXT;
	static const std::string COLLISION_TEXT;
	static const std::string SKIP_INTRO_TEXT;
	static const std::string FULLSCREEN_TEXT;
	static const std::string SOUND_RESAMPLING_TEXT;

	std::unique_ptr<TextBox> titleTextBox, backToPauseTextBox, fpsTextBox,
		resolutionScaleTextBox, playerInterfaceTextBox, verticalFOVTextBox,
		cursorScaleTextBox, letterboxAspectTextBox, hSensitivityTextBox, vSensitivityTextBox,
		collisionTextBox, skipIntroTextBox, fullscreenTextBox, soundResamplingTextBox;
	Button<Game&> backToPauseButton;
	Button<OptionsPanel&, Options&> fpsUpButton, fpsDownButton,
		verticalFOVUpButton, verticalFOVDownButton, cursorScaleUpButton, cursorScaleDownButton,
		hSensitivityUpButton, hSensitivityDownButton, vSensitivityUpButton, vSensitivityDownButton,
		collisionButton, skipIntroButton;
	Button<OptionsPanel&, Options&, Renderer&> resolutionScaleUpButton,
		resolutionScaleDownButton, letterboxAspectUpButton, letterboxAspectDownButton,
		fullscreenButton;
	Button<OptionsPanel&, Options&, AudioManager&> soundResamplingButton;
	Button<OptionsPanel&, Options&, Player&, Renderer&> playerInterfaceButton;

	static std::string getPlayerInterfaceString(bool modernInterface);
	static std::string getSoundResamplingString(int resamplingOption);

	void updateFPSText(int fps);
	void updateResolutionScaleText(double resolutionScale);
	void updatePlayerInterfaceText(PlayerInterface playerInterface);
	void updateVerticalFOVText(double verticalFOV);
	void updateCursorScaleText(double cursorScale);
	void updateLetterboxAspectText(double letterboxAspect);
	void updateHorizontalSensitivityText(double hSensitivity);
	void updateVerticalSensitivityText(double vSensitivity);
	void updateCollisionText(bool collision);
	void updateSkipIntroText(bool skip);
	void updateFullscreenText(bool fullscreen);
	void updateSoundResamplingText(int resamplingOption);

	void drawTooltip(const std::string &text, Renderer &renderer);
public:
	OptionsPanel(Game &game);
	virtual ~OptionsPanel() = default;

	virtual std::pair<SDL_Texture*, CursorAlignment> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
