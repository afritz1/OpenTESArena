#ifndef INT2_H
#define INT2_H

class Int2
{
private:
	int x, y;
public:
	Int2(int x, int y);
	Int2(double x, double y);
	Int2();
	~Int2();

	Int2 operator +(const Int2 &p) const;
	Int2 operator -(const Int2 &p) const;
	Int2 operator *(double scale) const;

	int getX() const;
	int getY() const;

	void setX(int x);
	void setY(int y);
};

#endif
