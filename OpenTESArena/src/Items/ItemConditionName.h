#ifndef ITEM_CONDITION_NAME_H
#define ITEM_CONDITION_NAME_H

// A unique identifier for each relative stage of item degradation.
enum class ItemConditionName
{
	New,
	AlmostNew,
	SlightlyUsed,
	Used,
	Worn,
	Battered,
	Broken
};

#endif