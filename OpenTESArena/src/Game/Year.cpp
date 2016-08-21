#include <cassert>

#include "Year.h"

const int32_t Year::INITIAL_ERA = 3;
const int32_t Year::INITIAL_YEAR = 389;
const int32_t Year::YEARS_PER_ERA = 1000;

Year::Year(int32_t era, int32_t year)
{
	// 99.99% of the time, the era will be 3. It MIGHT go up if the player plays a ton.
	assert(era > 0);
	assert(year >= 0);

	this->eraNumber = era;
	this->yearNumber = year;
}

Year::Year()
	: Year(Year::INITIAL_ERA, Year::INITIAL_YEAR) { }

Year::~Year()
{

}

int32_t Year::getEraNumber() const
{
	return this->eraNumber;
}

int32_t Year::getYearNumber() const
{
	return this->yearNumber;
}

std::string Year::toString() const
{
	return std::to_string(this->getEraNumber()) + "E " +
		std::to_string(this->getYearNumber());
}

void Year::incrementEra()
{
	this->eraNumber++;
	assert(this->eraNumber > 0);
}

void Year::incrementYear()
{
	this->yearNumber++;
	assert(this->yearNumber >= 0);

	// Years are from 1-1000, not 0-999.
	if (this->getYearNumber() == (Year::YEARS_PER_ERA + 1))
	{
		this->yearNumber = 1;
		this->incrementEra();
	}
}
