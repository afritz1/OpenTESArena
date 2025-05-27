#include <cstdint>
#include <string>

#include "VOCFile.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"
#include "components/vfs/manager.hpp"

namespace
{
	// Data block types. Arena's .VOC files only use 0, 1, 6, and 7.
	enum class BlockType : uint8_t
	{
		Terminator = 0x00,
		SoundData = 0x01,
		SoundDataContinuation = 0x02,
		Silence = 0x03,
		Marker = 0x04,
		Text = 0x05,
		RepeatStart = 0x06,
		RepeatEnd = 0x07,
		ExtraInfo = 0x08,
		NewSoundData = 0x09
	};
}

bool VOCFile::init(const char *filename)
{
	Buffer<std::byte> src;
	if (!VFS::Manager::get().read(filename, &src))
	{
		DebugLogError("Could not read \"" + std::string(filename) + "\".");
		return false;
	}

	const uint8_t *srcPtr = reinterpret_cast<const uint8_t*>(src.begin());

	// Read part of the .VOC header. Bytes 0 to 18 contain "Creative Voice File",
	// and byte 19 prevents the whole file from being printed by accident.
	const uint8_t eofByte = *(srcPtr + 19);
	const uint16_t headerSize = Bytes::getLE16(srcPtr + 20);
	const uint16_t versionNumber = Bytes::getLE16(srcPtr + 22);
	const uint16_t checksum = Bytes::getLE16(srcPtr + 24);

	if (eofByte != 0x1A)
	{
		DebugLogError("Invalid EOF byte \"" + std::to_string(eofByte) + "\".");
		return false;
	}

	if (checksum != (~versionNumber + 0x1234))
	{
		DebugLogError("Invalid checksum \"" + std::to_string(checksum) + "\".");
		return false;
	}

	// Set the default sample rate to 0. Assume that a .VOC file has the same sample rate
	// for all of its sound data (error otherwise).
	this->sampleRate = 0;

	// Variables for repeating data (only for DRUMS.VOC).
	std::vector<uint8_t> repeatData;
	uint16_t repeatCount = 0;
	bool repeating = false;

	// Read data blocks.
	int offset = headerSize;
	while (offset < src.getCount())
	{
		constexpr int blockHeaderSize = 4;

		// One byte for the block type (0-9). Don't read any further if it's a terminator block.
		const BlockType blockType = static_cast<BlockType>(*(srcPtr + offset));
		if (blockType == BlockType::Terminator)
		{
			// End of file.
			break;
		}

		// Three bytes for the block size (unsigned 24-bit integer).
		const uint32_t blockSize = Bytes::getLE24(srcPtr + offset + 1);

		// Pointer to the beginning of the block's data (after the common header).
		const uint8_t *blockData = srcPtr + offset + blockHeaderSize;

		if (blockType == BlockType::SoundData)
		{
			// Read 8-bit unsigned PCM data.
			const uint8_t frequencyDivisor = *blockData;
			const uint8_t pcmCodec = *(blockData + 1);
			const uint8_t *audioBegin = blockData + 2;
			const uint8_t *audioEnd = blockData + blockSize;
			const int sampleRate = 1000000 / (256 - frequencyDivisor);

			constexpr int unsigned8BitPcmCodec = 0;
			DebugAssert(pcmCodec == unsigned8BitPcmCodec);

			if (this->sampleRate == 0)
			{
				this->sampleRate = sampleRate;
			}
			else
			{
				// Any subsequent sample rates must match.
				DebugAssert(this->sampleRate == sampleRate);
			}

			if (repeating)
			{
				repeatData.insert(repeatData.end(), audioBegin, audioEnd);
			}
			else
			{
				this->audioData.insert(this->audioData.end(), audioBegin, audioEnd);
			}
		}
		else if (blockType == BlockType::RepeatStart)
		{
			// Only used with DRUMS.VOC.
			// The sound blocks following this block should be repeated some number of times.
			repeatCount = Bytes::getLE16(blockData) + 1;
			repeating = true;

			// Don't handle the 0xFFFF special case (no .VOC repeats indefinitely in Arena).
			DebugAssert(repeatCount != 0xFFFF);
		}
		else if (blockType == BlockType::RepeatEnd)
		{
			// Only used with DRUMS.VOC.
			// An empty block like the terminator, tells when to stop repeating data blocks.
			// Take the repeat vector and append it onto the audio vector "repeatCount" times.
			for (int i = 0; i < repeatCount; i++)
			{
				this->audioData.insert(this->audioData.end(), repeatData.begin(), repeatData.end());
			}

			repeatData.clear();
			repeatCount = 0;
			repeating = false;
		}
		else
		{
			DebugCrashFormat("Block type \"%d\" not implemented.", blockType);
		}

		offset += blockHeaderSize + blockSize;
	}

	return true;
}

int VOCFile::getSampleRate() const
{
	return this->sampleRate;
}

BufferView<uint8_t> VOCFile::getAudioData()
{
	return this->audioData;
}

BufferView<const uint8_t> VOCFile::getAudioData() const
{
	return this->audioData;
}
