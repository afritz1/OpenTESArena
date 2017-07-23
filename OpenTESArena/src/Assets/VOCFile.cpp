#include "VOCFile.h"

#include "../Utilities/Bytes.h"
#include "../Utilities/Debug.h"

#include "components/vfs/manager.hpp"

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

VOCFile::VOCFile(const std::string &filename)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename.c_str());
	DebugAssert(stream != nullptr, "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	const auto fileSize = stream->tellg();
	stream->seekg(0, std::ios::beg);

	std::vector<uint8_t> srcData(fileSize);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// Read part of the .VOC header. Bytes 0 to 18 contain "Creative Voice File",
	// and byte 19 prevents the whole file from being printed by accident.
	const uint8_t eofByte = *(srcData.data() + 19);
	const uint16_t headerSize = Bytes::getLE16(srcData.data() + 20);
	const uint16_t versionNumber = Bytes::getLE16(srcData.data() + 22);
	const uint16_t checksum = Bytes::getLE16(srcData.data() + 24);

	DebugAssert(eofByte == 0x1A, "Invalid EOF byte \"" + std::to_string(eofByte) + "\".");
	DebugAssert(checksum == (~versionNumber + 0x1234), 
		"Invalid checksum \"" + std::to_string(checksum) + "\".");

	// Read data blocks.
	int offset = headerSize;
	while (offset < srcData.size())
	{
		const int blockHeaderSize = 4;

		// One byte for the block type (0-9). Don't read any further if it's a 
		// terminator block.
		const BlockType blockType = static_cast<BlockType>(*(srcData.data() + offset));

		if (blockType == BlockType::Terminator)
		{
			// End of file.
			break;
		}

		// Three bytes for the block size (unsigned 24-bit integer).
		const uint32_t blockSize = Bytes::getLE24(srcData.data() + offset + 1);

		// Pointer to the beginning of the block's data (after the common header).
		const uint8_t *blockData = srcData.data() + offset + blockHeaderSize;

		// Decide how to use the data block.
		if (blockType == BlockType::SoundData)
		{
			// Read 8-bit unsigned PCM data.
			const uint8_t frequencyDivisor = *blockData;
			const uint8_t pcmCodec = *(blockData + 1);
			const uint8_t *audioBegin = blockData + 2;
			const uint8_t *audioEnd = blockData + blockSize;
			const int sampleRate = 1000000 / (256 - frequencyDivisor);

			// To do.
			// - 8-bit unsigned PCM range is 0-255, with centerpoint of 128.
			const int centerPoint = 128;
		}
		else if (blockType == BlockType::RepeatStart)
		{
			// Only used with DRUMS.VOC.
			// The sound blocks following this block should be repeated some number of times.
			const uint16_t repeatCount = Bytes::getLE16(blockData) - 1;

			// To do. 
			// - Maybe keep a "repeating" boolean outside the loop? Append data blocks to
			//   a temp vector until "end" block is reached, where they're then appended to 
			//   the main vector 'count' times, and then 'repeating' is reset to false.
		}
		else if (blockType == BlockType::RepeatEnd)
		{
			// An empty block like the terminator, tells when to stop repeating data blocks.

			// To do.
		}
		else
		{
			DebugCrash("Block type \"" + 
				std::to_string(static_cast<int>(blockType)) + "\" not implemented.");
		}

		offset += blockHeaderSize + blockSize;
	}
}

VOCFile::~VOCFile()
{

}
