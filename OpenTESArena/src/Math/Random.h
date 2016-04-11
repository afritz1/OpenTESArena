#ifndef RANDOM_H
#define RANDOM_H

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

	int next();
	int next(int exclusiveMax);
	double nextReal();
};

#endif
