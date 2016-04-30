#ifndef DIRECTABLE_H
#define DIRECTABLE_H

#include "../Math/Float3.h"

// To be inherited by classes that have a 3D direction that they face.

class Directable
{
private:
	Float3d direction;
public:
	Directable(const Float3d &direction);
	~Directable();

	const Float3d &getDirection() const;

	void setDirection(const Float3d &direction);
};

#endif
