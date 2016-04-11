#ifndef ARTIFACT_NAME_H
#define ARTIFACT_NAME_H

// A unique identifier for each artifact in the game. This parent enumeration is
// for unifying the names of each derived artifact type.

// Sadly, there are no consumable artifacts. Yet.

enum class ArtifactName
{
	// Accessories.
	NecromancersAmulet,
	RingOfKhajiit,
	RingOfPhynaster,
	WarlocksRing,

	// Armor.
	AurielsShield,
	EbonyMail,
	LordsMail,
	SpellBreaker,

	// Miscellaneous.
	KingOrgnumsCoffer,
	OghmaInfinium,
	SkeletonsKey,

	// Weapons.
	AurielsBow,
	Chrysamere,
	EbonyBlade,
	StaffOfMagnus,
	Volendrung
};

#endif
