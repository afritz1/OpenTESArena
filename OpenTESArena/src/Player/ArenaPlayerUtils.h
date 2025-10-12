#ifndef ARENA_PLAYER_UTILS_H
#define ARENA_PLAYER_UTILS_H

class Random;

struct ExeData;
struct PrimaryAttributes;
struct DerivedAttributes;

namespace ArenaPlayerUtils
{
	constexpr int EyeHeight = 60;
	constexpr int TopOfHeadHeight = EyeHeight + 1;

	int scaleAttribute256To100(int attributeValue);

	int getBaseSpeed(int speedAttribute, int encumbranceMod);
	int getMoveSpeed(int baseSpeed);
	int getTurnSpeed(int baseSpeed);

	constexpr int AccelerationRate = 32;
	constexpr int AccelerationMax = 256;
	constexpr int DecelerationRate = 64;

	constexpr int ChasmHeightDeltaDryChasm = -80;
	constexpr int ChasmHeightSwimmingInterior = -25;
	constexpr int ChasmHeightSwimmingCity = -50;
	constexpr int ChasmHeightSwimmingWild = -10;
	constexpr int ChasmHeightRowBoatInterior = -10;
	constexpr int ChasmHeightRowBoatWild = -1;

	constexpr int ChasmMagnetUnitsPerFrame = 16;
	int getChasmFallSpeed(int frame);

	constexpr int ChasmClimbingUnitsPerFrame = 6;

	constexpr int JumpFrameCount = 10;
	constexpr int JumpFrameCountAcrobat = JumpFrameCount * 2;
	constexpr int JumpDisallowedCameraHeightUnits = 60; // Or in voxel column with any chasm.
	constexpr int StandingJumpForwardUnits = 30; // (dist*pc.attr[STRENGTH])/128
	int getJumpUnitsPerFrame(int frame);

	int rollHealthDice(int healthDie, Random &random);
	int calculateMaxHealthPoints(int charClassDefID, Random &random);
	int calculateMaxStamina(int strength, int endurance);
	int calculateMaxSpellPoints(int charClassDefID, int intelligence);
	int calculateDamageBonus(int strength);
	int calculateMaxWeight(int strength);
	int calculateMagicDefenseBonus(int willpower);
	int calculateBonusToHit(int agility);
	int calculateBonusToHealth(int endurance);
	int calculateStartingGold(Random &random);

	DerivedAttributes calculateStrengthDerivedBonuses(int strength);
	DerivedAttributes calculateWillpowerDerivedBonuses(int willpower);
	DerivedAttributes calculateAgilityDerivedBonuses(int agility);
	DerivedAttributes calculateEnduranceDerivedBonuses(int endurance);
	DerivedAttributes calculatePersonalityDerivedBonuses(int personality);
	DerivedAttributes calculateTotalDerivedBonuses(const PrimaryAttributes &attributes);

	int getThievingChance(int difficultyLevel, int thievingDivisor, int playerLevel, const PrimaryAttributes &attributes);
	bool attemptThieving(int difficultyLevel, int thievingDivisor, int playerLevel, const PrimaryAttributes &attributes, Random &random);
	int getLockDifficultyMessageIndex(int difficultyLevel, int thievingDivisor, int playerLevel, const PrimaryAttributes &attributes, const ExeData &exeData);

	int getSelfDamageFromBashWithFists(Random& random);
	bool doesBashSucceed(int damage, int lockLevel, const PrimaryAttributes& attributes, Random& random);
}
#endif
