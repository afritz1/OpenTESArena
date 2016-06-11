
#include "WildMidi.hpp"

#ifdef HAVE_WILDMIDI

#include "components/vfs/manager.hpp"

#include "wildmidi_lib.h"


namespace
{

class WildMidiSong : public MidiSong {
    midi *mSong;

public:
    WildMidiSong(midi *song);
    virtual ~WildMidiSong();
};

WildMidiSong::WildMidiSong(midi *song)
    : mSong(song)
{
}

WildMidiSong::~WildMidiSong()
{
    WildMidi_Close(mSong);
}

}


std::unique_ptr<WildMidiDevice> WildMidiDevice::sInstance;

WildMidiDevice &WildMidiDevice::get()
{
    if(!sInstance)
        sInstance.reset(new WildMidiDevice());
    return *sInstance.get();
}

void WildMidiDevice::shutdown()
{
    sInstance = nullptr;
}


WildMidiDevice::WildMidiDevice()
{
    /* TODO: Make this configurable. */
    static const char patchCfg[] = "/etc/timidity/freepats.cfg";

    if(WildMidi_Init(patchCfg, 48000, WM_MO_ENHANCED_RESAMPLING) < 0)
        throw std::runtime_error("Failed to init WildMidi");
    WildMidi_MasterVolume(100);
}

WildMidiDevice::~WildMidiDevice()
{
    WildMidi_Shutdown();
}

MidiSongPtr WildMidiDevice::open(const std::string &name)
{
    VFS::IStreamPtr fstream = VFS::Manager::get().open(name.c_str());
    if(fstream)
    {
        std::vector<char> midibuf;
        char readbuf[1024];

        /* Read the file into a buffer through the VFS, as it may be in an
         * archive that WildMidi can't read from.
         */
        while(fstream->read(readbuf, sizeof(readbuf)))
            midibuf.insert(midibuf.end(), readbuf, readbuf + fstream->gcount());

        if(!midibuf.empty())
        {
            midi *song = WildMidi_OpenBuffer(
                reinterpret_cast<unsigned char*>(midibuf.data()), midibuf.size()
            );
            if(song)
                return MidiSongPtr(new WildMidiSong(song));
        }
    }

    std::cerr<< "Failed to open resource "<<name <<std::endl;
    return MidiSongPtr(nullptr);
}

#endif /* HAVE_WILDMIDI */
