#ifndef CITIZEN_UTILS_H
#define CITIZEN_UTILS_H

#include "EntityAnimationInstance.h"
#include "EntityUtils.h"
#include "../Media/TextureUtils.h"
#include "../World/Coord.h"

class BinaryAssetLibrary;
class Chunk;
class EntityDefinition;
class EntityDefinitionLibrary;
class EntityManager;
class Random;

enum class ClimateType;

namespace CitizenUtils
{
	struct CitizenGenInfo
	{
		EntityDefID maleEntityDefID;
		EntityDefID femaleEntityDefID;
		const EntityDefinition *maleEntityDef;
		const EntityDefinition *femaleEntityDef;
		EntityAnimationInstance maleAnimInst;
		EntityAnimationInstance femaleAnimInst;
		PaletteID paletteID;
		int raceID;

		void init(EntityDefID maleEntityDefID, EntityDefID femaleEntityDefID, const EntityDefinition *maleEntityDef,
			const EntityDefinition *femaleEntityDef, EntityAnimationInstance &&maleAnimInst,
			EntityAnimationInstance &&femaleAnimInst, PaletteID paletteID, int raceID);
	};

	int getCitizenCount(const EntityManager &entityManager);
	int getCitizenCountInChunk(const ChunkInt2 &chunk, const EntityManager &entityManager);

	CitizenGenInfo makeCitizenGenInfo(int raceID, ClimateType climateType,
		const EntityDefinitionLibrary &entityDefLibrary, TextureManager &textureManager);

	bool trySpawnCitizenInChunk(const Chunk &chunk, const CitizenGenInfo &citizenGenInfo, Random &random,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager, EntityManager &entityManager);

	// Writes the citizen textures to the renderer. This is done once for all citizens in a level.
	void writeCitizenTextures(const EntityDefinition &maleEntityDef, const EntityDefinition &femaleEntityDef,
		TextureManager &textureManager, Renderer &renderer);

	// Used when the player commits a crime and the guards are called.
	void clearCitizens(EntityManager &entityManager);
}

#endif
