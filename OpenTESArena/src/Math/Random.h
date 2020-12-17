#ifndef RANDOM_H
#define RANDOM_H

#include <cstdint>
#include <limits>
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

	// Initialized with the given seed.
	void init(int seed);

	// Initialized with the current time.
	void init();

	// Includes 0 to ~2.14 billion.
	int next();

	// Includes 0 to (exclusiveMax - 1).
	int next(int exclusiveMax);

	// Includes [0.0, 1.0).
	double nextReal();
};

// This class mimics the behavior of Arena's random number generator.
class ArenaRandom
{
private:
	static constexpr uint32_t DEFAULT_SEED = 12345;

	uint32_t value;
public:
	ArenaRandom(uint32_t seed);
	ArenaRandom();

	static constexpr int MAX = std::numeric_limits<uint16_t>::max();

	uint32_t getSeed() const;
	int next();
	void srand(uint32_t seed);
};

#endif
