#ifndef RANDOM_UTILS_H
#define RANDOM_UTILS_H

class Random;

namespace RandomUtils
{
	// Shuffles the elements randomly with an unspecified RNG source.
	void shuffle(int *buffer, int count);

	// Shuffles the elements randomly with the given RNG source.
	void shuffle(int *buffer, int count, Random &random);
}

#endif
