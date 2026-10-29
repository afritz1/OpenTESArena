#pragma once

#include <functional>

#include "ProvinceDefinition.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/Singleton.h"

class BinaryAssetLibrary;
class Random;

using ProvinceDefinitionID = int;
using ProvinceDefinitionPredicate = std::function<bool(const ProvinceDefinition&)>;

class ProvinceLibrary : public Singleton<ProvinceLibrary>
{
private:
	Buffer<ProvinceDefinition> provinces;
public:
	void init(const BinaryAssetLibrary &binaryAssetLibrary);

	int getProvinceCount() const;

	const ProvinceDefinition &getDefinition(ProvinceDefinitionID id) const;
	ProvinceDefinitionID getRandomDefinitionIdIf(Random &random, const ProvinceDefinitionPredicate &predicate) const;
	ProvinceDefinitionID findDefinitionIdIf(const ProvinceDefinitionPredicate &predicate) const;
};
