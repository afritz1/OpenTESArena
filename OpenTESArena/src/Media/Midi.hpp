#ifndef MEDIA_MIDI_HPP
#define MEDIA_MIDI_HPP

#include <memory>
#include <string>

/* Pure virtual interface for reading PCM samples from a MIDI-style song. */
class MidiSong {
public:
    virtual ~MidiSong() { }

    /* TODO: give back channel configuration and sample type (should not just
     * be channel count and bit depth; different channel configurations can
     * have the same count, and different sample types can have the same number
     * of bits).
     */
    virtual void getFormat(int *sampleRate) = 0;

    /* Read and return size is in sample frames. */
    virtual size_t read(char *buffer, size_t count) = 0;

    /* Offset is in sample frames. */
    virtual bool seek(size_t offset) = 0;
};
typedef std::unique_ptr<MidiSong> MidiSongPtr;

/* Pure virtual interface for opening MIDI-style songs. Should be implemented
 * as a factory singleton.
 */
class MidiDevice {
protected:
    static std::unique_ptr<MidiDevice> sInstance;

public:
    virtual ~MidiDevice() { }

    virtual MidiSongPtr open(const std::string &name) = 0;

    static bool isInited() { return !!sInstance; }
    static void shutdown() { sInstance = nullptr; }
    static MidiDevice &get() { return *sInstance.get(); }
};

#endif /* MEDIA_MIDI_HPP */
