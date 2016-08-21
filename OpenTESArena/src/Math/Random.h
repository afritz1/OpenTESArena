#ifndef RANDOM_H
#define RANDOM_H

#include <cstdint>
#include <random>

class Random
{
private:
	std::default_random_engine generator;
	std::uniform_int_distribution<int32_t> integerDistribution;
	std::uniform_real_distribution<double> realDistribution;
public:
	// Initialized with the given seed.
	Random(int32_t seed);

	// Initialized with the current time.
	Random();
	~Random();

	// Includes 0 to ~2.14 billion.
	int32_t next();

	// Includes 0 to (exclusiveMax - 1).
	int32_t next(int32_t exclusiveMax);

	// Includes 0.0 to 1.0.
	double nextReal();
};

#endif
