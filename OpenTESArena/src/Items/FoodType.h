#ifndef FOOD_TYPE_H
#define FOOD_TYPE_H

// A unique identifier for each type of food. These are all made up, since the 
// original Arena did not have any food. It's easy to add more kinds of food because 
// the original game didn't have any art for any items either.

// There should be food types for flesh, like goblin meat (yuck!). Each creature might
// have an optional food that it drops on death. I think cooking food or getting hides 
// from corpses is a bit ouside the scope, because that would involve programming a 
// whole crafting system too. It's a fun slippery slope!

// Foods should just be found in existing containers. Foraging in the wilderness
// sounds like too much more code. It'd have to be only certain food types per bush...
// yeah.

enum class FoodType
{
	// Typical foods.
	Apple,
	ApplePie,
	Blueberry,
	BlueberryPie,
	Bread,
	Cabbage,
	Carrot,
	Cheese,
	Fish,
	Watermelon,

	// Creature meat. There has to be a specific creature per meat because that's 
	// where the data is.
	GoblinMeat,
	RatMeat,
	WolfMeat
};

#endif