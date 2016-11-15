#ifndef INT3_H
#define INT3_H

class Int3
{
private:
	int x, y, z;
public:
	Int3(int x, int y, int z);
	Int3(double x, double y, double z);
	Int3();
	~Int3();

	bool operator==(const Int3 &p) const;
	Int3 operator +(const Int3 &p) const;
	Int3 operator -(const Int3 &p) const;
	Int3 operator *(double scale) const;

	int getX() const;
	int getY() const;
	int getZ() const;

	void setX(int x);
	void setY(int y);
	void setZ(int z);
};

#include <unordered_map>

// Hash definition for Int3.
namespace std
{
	template <>
	struct hash<Int3>
	{
		size_t operator()(const Int3 &x) const
		{
			// Multiply with some prime numbers before xor'ing.
			return static_cast<size_t>(x.getX() ^ (x.getY() * 41) ^ (x.getZ() * 199));
		}
	};
}

#endif
