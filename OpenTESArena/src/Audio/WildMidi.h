#pragma once

#include "Midi.h"

#ifdef HAVE_WILDMIDI

/* Implementation for opening supported MIDI-like files through WildMidi. */
class WildMidiDevice : public MidiDevice {
public:
	// Constructor (public for std::make_unique).
	WildMidiDevice(const std::string &midiConfig);
	~WildMidiDevice() override;

	static void init(const std::string &midiConfig);

	virtual MidiSongPtr open(const std::string &name) override;
};

#endif /* HAVE_WILDMIDI */
