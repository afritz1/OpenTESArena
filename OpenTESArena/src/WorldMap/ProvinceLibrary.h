#pragma once

#include "components/utilities/Buffer.h"
#include "components/utilities/Singleton.h"

class BinaryAssetLibrary;

struct ProvinceDefinition;

class ProvinceLibrary : public Singleton<ProvinceLibrary>
{
private:
	Buffer<ProvinceDefinition> provinces;
public:
	void init(const BinaryAssetLibrary &binaryAssetLibrary);

	// Gets the number of provinces in the world map.
	int getProvinceCount() const;

	const ProvinceDefinition &getProvinceDef(int index) const;
};
