#include <ctime>

#include "Random.h"

Random::Random(int seed)
{
	this->init(seed);
}

Random::Random()
{
	this->init();
}

void Random::init(int seed)
{
	this->generator = std::default_random_engine(seed);
	this->integerDistribution = std::uniform_int_distribution<int>(0, std::numeric_limits<int>::max());
	this->realDistribution = std::uniform_real_distribution<double>(0.0, 1.0);
}

void Random::init()
{
	this->init(static_cast<int>(std::time(nullptr)));
}

int Random::next()
{
	return this->integerDistribution(this->generator);
}

int Random::next(int exclusiveMax)
{
	return this->next() % exclusiveMax;
}

double Random::nextReal()
{
	return this->realDistribution(this->generator);
}

// ArenaRandom

ArenaRandom::ArenaRandom(uint32_t seed)
{
	this->value = seed;
}

ArenaRandom::ArenaRandom()
	: ArenaRandom(ArenaRandom::DEFAULT_SEED) { }

uint32_t ArenaRandom::getSeed() const
{
	return this->value;
}

int ArenaRandom::next()
{
	this->value *= 7143469;
	return (this->value >> 16) & 0xFFFF;
}

void ArenaRandom::srand(uint32_t seed)
{
	this->value = seed;
}
