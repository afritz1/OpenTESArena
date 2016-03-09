#ifndef YEAR_H
#define YEAR_H

#include <string>

class Year
{
private:
	static const int INITIAL_ERA;
	static const int INITIAL_YEAR;

	int eraNumber;
	int yearNumber;

	void incrementEra();
public:
	// Year constructor with a specific era and year.
	Year(int era, int year);

	// Default year constructor for 3E 389.
	Year();
	~Year();

	// This is shown in the manual to be a thousand years, though the Oblivion
	// crisis says otherwise. Let's use the Arena format anyway.
	static const int YEARS_PER_ERA;

	const int &getEraNumber() const;
	const int &getYearNumber() const;
	std::string toString() const;

	// Only the year can be incremented directly.
	void incrementYear();
};

#endif