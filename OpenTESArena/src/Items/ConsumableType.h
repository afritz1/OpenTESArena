#ifndef CONSUMABLE_TYPE_H
#define CONSUMABLE_TYPE_H

// Consumables are abstract types and thus require a derived implementation.

// One of the reasons for separation is that foods should not require identification,
// but potions should.

enum class ConsumableType
{
	Food,
	Potion
};

#endif