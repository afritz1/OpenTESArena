#include <cstdint>

#include "ExeUnpacker.h"

#include "../Utilities/Debug.h"

#include "components/vfs/manager.hpp"

ExeUnpacker::ExeUnpacker(const std::string &filename)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename.c_str());
	Debug::check(stream != nullptr, "Exe Unpacker", "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	const auto fileSize = stream->tellg();
	stream->seekg(0, std::ios::beg);

	std::vector<uint8_t> srcData(fileSize);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// To do:
	// PKLITE v1.12 decompressor implementation for A.EXE.
	// - Compressed file data is ready in "srcData".
	// - Put decompressed file data into "this->text".
	// - See http://unp.bencastricum.nl/unp4-src.zip for a version in assembly.
	// - Some relevant files in unp4-src are: epklt.asm, epksg.asm, and epktn.asm.

	// Keep this until the decompressor works.
	Debug::crash("Exe Unpacker", "Not implemented.");
}

ExeUnpacker::~ExeUnpacker()
{

}

const std::string &ExeUnpacker::getText() const
{
	return this->text;
}
