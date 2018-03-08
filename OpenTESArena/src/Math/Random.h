#ifndef RANDOM_H
#define RANDOM_H

#include <cstdint>
#include <random>

class Random
{
private:
	std::default_random_engine generator;
	std::uniform_int_distribution<int> integerDistribution;
	std::uniform_real_distribution<double> realDistribution;
public:
	// Initialized with the given seed.
	Random(int seed);

	// Initialized with the current time.
	Random();
	~Random();

	// Includes 0 to ~2.14 billion.
	int next();

	// Includes 0 to (exclusiveMax - 1).
	int next(int exclusiveMax);

	// Includes 0.0 to 1.0.
	double nextReal();
};

// This class mimics the behavior of Arena's random number generator.
class ArenaRandom
{
private:
	static const uint32_t DEFAULT_SEED;

	uint32_t value;
public:
	ArenaRandom(uint32_t seed);
	ArenaRandom();
	~ArenaRandom();

	static const int MAX;

	uint32_t getSeed() const;
	int next();
	void srand(uint32_t seed);
};

#endif
