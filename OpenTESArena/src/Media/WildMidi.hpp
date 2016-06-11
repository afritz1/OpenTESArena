#ifndef MEDIA_WILDMIDI_HPP
#define MEDIA_WILDMIDI_HPP

#ifdef HAVE_WILDMIDI

#include "Midi.hpp"

/* Implementation for opening supported MIDI-like files through WildMidi. */
class WildMidiDevice : public MidiDevice {
    static std::unique_ptr<WildMidiDevice> sInstance;

    WildMidiDevice();
public:
    virtual ~WildMidiDevice();

    virtual MidiSongPtr open(const std::string &name) override;

    static WildMidiDevice &get();
    static void shutdown();
};

#endif /* HAVE_WILDMIDI */

#endif /* MEDIA_WILDMIDI_HPP */
