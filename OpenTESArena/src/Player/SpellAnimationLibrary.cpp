#include "SpellAnimationLibrary.h"
#include "../Assets/ExeData.h"
#include "../Assets/TextureManager.h"

#include "components/debug/Debug.h"
#include "components/utilities/Span.h"

void SpellAnimationLibrary::init(const ExeData &exeData, TextureManager &textureManager)
{
	auto initDef = [&textureManager](const std::string &filename, SpellAnimationType type, SpellAnimationDefinition *outDef)
	{
		const TextureFileMetadataID metadataID = textureManager.getMetadataID(filename.c_str());
		if (metadataID < 0)
		{
			DebugLogErrorFormat("Couldn't get metadata ID for spell cast animation \"%s\".", filename.c_str());
			return;
		}

		const TextureFileMetadata &metadata = textureManager.getMetadataHandle(metadataID);
		const int frameCount = metadata.getTextureCount();

		outDef->type = type;
		outDef->width = metadata.getWidth(0);
		outDef->height = metadata.getHeight(0);

		outDef->textureAssets.init(frameCount);
		for (int frameIndex = 0; frameIndex < frameCount; frameIndex++)
		{
			outDef->textureAssets.set(frameIndex, TextureAsset(filename, frameIndex));
		}
	};

	Span<const std::string> spellCastAnimations = exeData.spells.spellCastAnimations;
	for (int i = 0; i < spellCastAnimations.getCount(); i++)
	{
		SpellAnimationDefinition def;
		initDef(spellCastAnimations[i], static_cast<SpellAnimationType>(i), &def);
		this->defs.emplace_back(std::move(def));
	}

	// Manually add smoke because the original game forgot it.
	SpellAnimationDefinition smokeDef;
	initDef("smoke.cfa", SpellAnimationType::Smoke, &smokeDef);
	this->defs.emplace_back(std::move(smokeDef));
}

const SpellAnimationDefinition &SpellAnimationLibrary::getDefinition(SpellAnimationDefinitionID id) const
{
	DebugAssertIndex(this->defs, id);
	return this->defs[id];
}

SpellAnimationDefinitionID SpellAnimationLibrary::getDefinitionIdByType(SpellAnimationType type) const
{
	for (int i = 0; i < static_cast<int>(this->defs.size()); i++)
	{
		if (this->defs[i].type == type)
		{
			return static_cast<SpellAnimationDefinitionID>(i);
		}
	}

	return -1;
}
