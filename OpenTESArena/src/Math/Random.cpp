#include <ctime>

#include "Random.h"

Random::Random(int32_t seed)
{
	this->generator = std::default_random_engine(seed);
	this->integerDistribution = std::uniform_int_distribution<int32_t>(
		0, std::numeric_limits<int32_t>::max());
	this->realDistribution = std::uniform_real_distribution<double>(
		0.0, std::nextafter(1.0, std::numeric_limits<double>::max()));
}

Random::Random()
	: Random(static_cast<int32_t>(time(nullptr))) { }

Random::~Random()
{

}

int32_t Random::next()
{
	return this->integerDistribution(this->generator);
}

int32_t Random::next(int32_t exclusiveMax)
{
	return this->next() % exclusiveMax;
}

double Random::nextReal()
{
	return this->realDistribution(this->generator);
}
