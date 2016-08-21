#include <ctime>

#include "Random.h"

Random::Random(int seed)
{
	this->generator = std::default_random_engine(seed);
	this->integerDistribution = std::uniform_int_distribution<int>(
		0, std::numeric_limits<int>::max());
	this->realDistribution = std::uniform_real_distribution<double>(
		0.0, std::nextafter(1.0, std::numeric_limits<double>::max()));
}

Random::Random()
	: Random(static_cast<int>(time(nullptr))) { }

Random::~Random()
{

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
