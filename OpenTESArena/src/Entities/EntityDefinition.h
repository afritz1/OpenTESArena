#ifndef ENTITY_DEFINITION_H
#define ENTITY_DEFINITION_H

#include <optional>
#include <string_view>

#include "EntityAnimationDefinition.h"
#include "EntityAnimationUtils.h"
#include "../Assets/ExeData.h"

class EntityDefinition
{
public:
	struct CreatureData
	{
		char name[64];
		int level;
		int minHP;
		int maxHP;
		int baseExp;
		int expMultiplier;
		int soundIndex;
		char soundName[32];
		int minDamage;
		int maxDamage;
		int magicEffects;
		int scale;
		int yOffset;
		bool hasNoCorpse;
		int bloodIndex;
		int diseaseChances;
		int attributes[8];

		CreatureData();

		void init(int creatureIndex, bool isFinalBoss, const ExeData &exeData);
	};

	struct InfData
	{
		// .INF flat index.
		// @todo: remove dependency on this? I.e. just keep all the equivalent data (entity
		// double size, puddle, etc.) in this class.
		int flatIndex;

		// Several copied over from .INF data (not all, just for initial implementation).
		int yOffset;
		bool collider;
		bool puddle;
		bool largeScale;
		bool dark;
		bool transparent;
		bool ceiling;
		bool mediumScale;
		bool streetLight;
		std::optional<int> lightIntensity;

		InfData();

		void init(int flatIndex, int yOffset, bool collider, bool puddle, bool largeScale,
			bool dark, bool transparent, bool ceiling, bool mediumScale, bool streetLight,
			const std::optional<int> &lightIntensity);
	};
private:
	CreatureData creatureData;
	InfData infData;
	EntityAnimationDefinition animDef; // @todo: might move to EntityAnimationLibrary eventually
	//EntityAnimID animID;
	bool isCreatureInited;
	bool isHumanEnemyInited;
	bool isOtherInited;
public:
	EntityDefinition();

	void initCreature(int creatureIndex, bool isFinalBoss, int flatIndex, const ExeData &exeData,
		EntityAnimationDefinition &&animDef);

	void initHumanEnemy(const char *name, int flatIndex, int yOffset, bool collider, bool largeScale,
		bool dark, bool transparent, bool ceiling, bool mediumScale,
		const std::optional<int> &lightIntensity, EntityAnimationDefinition &&animDef);

	// @todo: eventually blacksmith/wizard/etc. info here, or no? (entirely dependent on current level?)
	void initOther(int flatIndex, int yOffset, bool collider, bool puddle, bool largeScale, bool dark,
		bool transparent, bool ceiling, bool mediumScale, bool streetLight,
		const std::optional<int> &lightIntensity, EntityAnimationDefinition &&animDef);

	std::string_view getDisplayName() const;

	// @todo: quick hacks; probably refactor or use some enum class or integrate with each data struct?
	// @todo: more formal discriminated union.
	bool isCreature() const;
	bool isHumanEnemy() const;
	bool isOther() const;

	/*EntityAnimID &getAnimID();
	const EntityAnimID &getAnimID() const;*/
	const EntityAnimationDefinition &getAnimDef() const;

	CreatureData &getCreatureData();
	const CreatureData &getCreatureData() const;

	InfData &getInfData();
	const InfData &getInfData() const;
};

#endif
