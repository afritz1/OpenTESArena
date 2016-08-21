#ifndef INT3_H
#define INT3_H

#include <cstdint>

class Int3
{
private:
	int32_t x, y, z;
public:
	Int3(int32_t x, int32_t y, int32_t z);
	Int3(double x, double y, double z);
	Int3();
	~Int3();

	Int3 operator +(const Int3 &p) const;
	Int3 operator -(const Int3 &p) const;
	Int3 operator *(double scale) const;

	int32_t getX() const;
	int32_t getY() const;
	int32_t getZ() const;

	void setX(int32_t x);
	void setY(int32_t y);
	void setZ(int32_t z);
};

#endif
