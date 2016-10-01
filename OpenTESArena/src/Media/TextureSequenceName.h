#ifndef TEXTURE_SEQUENCE_NAME_H
#define TEXTURE_SEQUENCE_NAME_H

// A unique identifier for each .FLC/.CEL video, such as cinematics. This is not 
// intended for sprite animations.

// A couple sequence names are left out (for now) because they are only in the 
// CD version of the game, and development is focused on the floppy version.
// The videos in the CD version are:
// - AFLC2 (AFLC2.FLC)
// - IntroBook (INTRO.FLC)
// - Mage (MAGE.FLC, duplicate of MAGE.CEL)
// - NewJagarDeath (NUJAGDTH.FLC)
// - NewKing (NUKING.FLC)
// - Rogue (ROGUE.FLC, duplicate of ROGUE.CEL)
// - Staff (STAFF.FLC)
// - Walk (WALK.FLC)
// - Warrior (WARRIOR.FLC, duplicate of WARRIOR.CEL)

enum class TextureSequenceName
{
	ChaosVision,
	End01,
	End02,
	Jagar,
	JagarDeath,
	JagarShield,
	King,
	Mage,
	Morph,
	OpeningScroll,
	Rogue,
	Silmane,
	Warhaft,
	Warrior
};

#endif
