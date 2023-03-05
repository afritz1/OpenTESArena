#ifndef RANDOM_UTILS_H
#define RANDOM_UTILS_H

#include "components/utilities/BufferView.h"

class Random;

namespace RandomUtils
{
	// Shuffles the elements randomly with an unspecified RNG source.
	void shuffle(BufferView<int> buffer);

	// Shuffles the elements randomly with the given RNG source.
	void shuffle(BufferView<int> buffer, Random &random);
}

#endif
