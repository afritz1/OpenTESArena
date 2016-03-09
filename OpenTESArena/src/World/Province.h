#ifndef PROVINCE_H
#define PROVINCE_H

#include <string>
#include <vector>

#include "ProvinceName.h"

// After checking through some climates, it feels like it would make sense to
// give each province a "typical weather", like rain and fog for Black Marsh,
// even though several of Black Marsh's locations are deserts. Maybe the
// developers intended it to be like wastelands, or barrens?

enum class LocationName;

class Province
{
private:
	// dynamic vector of random dungeons...?
	ProvinceName provinceName;
public:
	Province(ProvinceName provinceName);
	~Province();

	static std::vector<ProvinceName> getAllProvinceNames();

	const ProvinceName &getProvinceName() const;
	std::string toString() const;
	std::vector<LocationName> getCivilizations() const;
	std::vector<LocationName> getMainQuestDungeons() const;
	// vector<> getRandomDungeons()...?
};

#endif