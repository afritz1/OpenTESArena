#include "WildMidi.h"

#ifdef HAVE_WILDMIDI

#include <cassert>

#include "../Utilities/Debug.h"

#include "components/vfs/manager.hpp"
#include "wildmidi_lib.h"

namespace
{
	int sInitState = -1;

	class WildMidiSong : public MidiSong {
		midi *mSong;

	public:
		WildMidiSong(midi *song);
		virtual ~WildMidiSong();

		virtual void getFormat(int *sampleRate) override;

		virtual size_t read(char *buffer, size_t count) override;

		virtual bool seek(size_t offset) override;
	};

	WildMidiSong::WildMidiSong(midi *song)
		: mSong(song)
	{
	}

	WildMidiSong::~WildMidiSong()
	{
		WildMidi_Close(mSong);
	}

	void WildMidiSong::getFormat(int *sampleRate)
	{
		/* Currently always outputs 16-bit stereo 48khz */
		*sampleRate = 48000;
	}

	size_t WildMidiSong::read(char *buffer, size_t count)
	{
		/* WildMidi wants bytes, so convert from and to sample frames. */
#if LIBWILDMIDI_VERSION >= ((0u << 16) | (4u << 8) | 0)
		return WildMidi_GetOutput(mSong, reinterpret_cast<int8_t*>(buffer), 
			static_cast<uint32_t>(count * 4)) / 4;
#else
		return WildMidi_GetOutput(mSong, buffer, static_cast<unsigned long>(count * 4)) / 4;
#endif
	}

	bool WildMidiSong::seek(size_t offset)
	{
		unsigned long pos = static_cast<unsigned long>(offset);
		const int status = WildMidi_FastSeek(mSong, &pos);
		return status >= 0;
	}
}


WildMidiDevice::WildMidiDevice(const std::string &soundfont)
{
	sInitState = WildMidi_Init(soundfont.c_str(), 48000, WM_MO_ENHANCED_RESAMPLING);
	if (sInitState < 0)
		DebugMention("Failed to init WildMidi.");
	else
		WildMidi_MasterVolume(100);
}

WildMidiDevice::~WildMidiDevice()
{
	if (sInitState >= 0)
		WildMidi_Shutdown();
}

void WildMidiDevice::init(const std::string &soundfont)
{
	sInstance = std::make_unique<WildMidiDevice>(soundfont);
}

MidiSongPtr WildMidiDevice::open(const std::string &name)
{
	if (sInitState < 0)
		return MidiSongPtr(nullptr);

	VFS::IStreamPtr fstream = VFS::Manager::get().open(name.c_str());
	if (!fstream)
	{
		std::cerr << "Failed to open resource " << name << std::endl;
		return MidiSongPtr(nullptr);
	}

	/* Read the file into a buffer through the VFS, as it may be in an archive
	 * that WildMidi can't read from.
	 */
	std::vector<char> midibuf;
	while (fstream->good())
	{
		char readbuf[1024];
		fstream->read(readbuf, sizeof(readbuf));
		midibuf.insert(midibuf.end(), readbuf, readbuf + fstream->gcount());
	}

	midi *song = WildMidi_OpenBuffer(
		reinterpret_cast<unsigned char*>(midibuf.data()), 
		static_cast<unsigned long>(midibuf.size()));

	return (song != nullptr) ? std::make_unique<WildMidiSong>(song) : nullptr;
}

#endif /* HAVE_WILDMIDI */
