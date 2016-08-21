#ifndef INT2_H
#define INT2_H

#include <cstdint>

class Int2
{
private:
	int32_t x, y;
public:
	Int2(int32_t x, int32_t y);
	Int2(double x, double y);
	Int2();
	~Int2();

	Int2 operator +(const Int2 &p) const;
	Int2 operator -(const Int2 &p) const;
	Int2 operator *(double scale) const;

	int32_t getX() const;
	int32_t getY() const;

	void setX(int32_t x);
	void setY(int32_t y);
};

#endif
