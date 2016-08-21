#ifndef YEAR_H
#define YEAR_H

#include <cstdint>
#include <string>

class Year
{
private:
	static const int32_t INITIAL_ERA;
	static const int32_t INITIAL_YEAR;

	int32_t eraNumber;
	int32_t yearNumber;

	void incrementEra();
public:
	// Year constructor with a specific era and year.
	Year(int32_t era, int32_t year);

	// Default year constructor for 3E 389.
	Year();
	~Year();

	// This is shown in the manual to be a thousand years, though the Oblivion
	// crisis says otherwise. Let's use the Arena format anyway.
	static const int32_t YEARS_PER_ERA;

	int32_t getEraNumber() const;
	int32_t getYearNumber() const;
	std::string toString() const;

	// Only the year can be incremented directly.
	void incrementYear();
};

#endif
