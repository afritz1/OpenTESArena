#include <algorithm>
#include <cstdio>

#include "ArenaLevelLibrary.h"
#include "MIFUtils.h"

#include "components/debug/Debug.h"
#include "components/dos/DOSUtils.h"

bool ArenaLevelLibrary::init()
{
	DebugLog("Initializing Arena level assets.");
	bool success = this->initCityBlockMifs();
	success &= this->initWildernessChunks();
	return success;
}

bool ArenaLevelLibrary::initCityBlockMifs()
{
	const int codeCount = MIFUtils::getCityBlockCodeCount();
	const int variationsCount = MIFUtils::getCityBlockVariationsCount();
	const int rotationCount = MIFUtils::getCityBlockRotationCount();

	bool success = true;

	// Iterate over all city block codes, variations, and rotations.
	for (int i = 0; i < codeCount; i++)
	{
		const std::string &code = MIFUtils::getCityBlockCode(i);
		const int variations = MIFUtils::getCityBlockVariations(i);

		// Variation IDs are 1-based.
		for (int variation = 1; variation <= variations; variation++)
		{
			for (int k = 0; k < rotationCount; k++)
			{
				const std::string &rotation = MIFUtils::getCityBlockRotation(k);
				const std::string mifName = MIFUtils::makeCityBlockMifName(code.c_str(), variation, rotation.c_str());

				const auto existingIter = std::find_if(this->cityBlockMifs.begin(), this->cityBlockMifs.end(),
					[&mifName](const MIFFile &mif)
				{
					return mif.getFilename() == mifName;
				});

				// No duplicate .MIFs allowed.
				DebugAssert(existingIter == this->cityBlockMifs.end());

				MIFFile mif;
				if (mif.init(mifName.c_str()))
				{
					this->cityBlockMifs.emplace_back(std::move(mif));
				}
				else
				{
					DebugLogError("Could not init .MIF \"" + mifName + "\".");
					success = false;
				}
			}
		}
	}

	return success;
}

bool ArenaLevelLibrary::initWildernessChunks()
{
	// The first four wilderness files are city blocks but they can be loaded anyway.
	this->wildernessChunks.resize(70);

	for (int i = 0; i < static_cast<int>(this->wildernessChunks.size()); i++)
	{
		const int rmdID = i + 1;
		DOSUtils::FilenameBuffer rmdFilename;
		std::snprintf(rmdFilename.data(), rmdFilename.size(), "WILD0%02d.RMD", rmdID);

		RMDFile &rmdFile = this->wildernessChunks[i];
		if (!rmdFile.init(rmdFilename.data()))
		{
			DebugLogWarning("Couldn't init .RMD file \"" + std::string(rmdFilename.data()) + "\".");
		}
	}

	return true;
}

BufferView<const MIFFile> ArenaLevelLibrary::getCityBlockMifs() const
{
	return this->cityBlockMifs;
}

BufferView<const RMDFile> ArenaLevelLibrary::getWildernessChunks() const
{
	return this->wildernessChunks;
}
