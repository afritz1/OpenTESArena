#ifndef MEDIA_MIDI_HPP
#define MEDIA_MIDI_HPP

#include <memory>


/* Pure virtual interface for reading PCM samples from a MIDI-style song. */
class MidiSong {
public:
    virtual ~MidiSong() { }
};
typedef std::unique_ptr<MidiSong> MidiSongPtr;

/* Pure virtual interface for opening MIDI-style songs. Should be implemented
 * as a factory singleton.
 */
class MidiDevice {
public:
    virtual ~MidiDevice() { }

    virtual MidiSongPtr open(const std::string &name) = 0;
};

#endif /* MEDIA_MIDI_HPP */
