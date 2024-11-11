#ifndef TRINKET_TYPE_H
#define TRINKET_TYPE_H

// A unique identifier for non-metal accessories, or accessories that shouldn't
// receive bonuses from the Metallic class.
//
// This separation from accessories is something I thought of myself, because
// I wanted a way to organize all of the metal and non-metal jewelry into two
// groups.
enum class TrinketType
{
	Crystal,
	Mark
};

#endif
