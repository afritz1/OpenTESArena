#ifndef SKY_INSTANCE_H
#define SKY_INSTANCE_H

#include <vector>

// Contains distant sky object instances and their state.

// The renderer should only care about 1) current direction, 2) current texture ID, 3) anchor,
// and 4) shading type. Maybe also rendering order. It has the option of doing visibility culling
// as well.

class SkyDefinition;
class SkyInfoDefinition;

class SkyInstance
{
private:
	
public:
	void init(const SkyDefinition &skyDefinition, const SkyInfoDefinition &skyInfoDefinition);

	/*int getObjectCount() const;
	const SkyObjectInstance &getObject(int index) const;

	void addObject(SkyObjectInstance &&inst);

	void update(double dt, double latitude, double daytimePercent, const SkyDefinition &skyDefinition);*/
};

#endif
