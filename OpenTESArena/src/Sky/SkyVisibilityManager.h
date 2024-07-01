#ifndef SKY_VISIBILITY_MANAGER_H
#define SKY_VISIBILITY_MANAGER_H

#include <unordered_set>

class SkyInstance;

struct RenderCamera;

class SkyVisibilityManager
{
private:
	std::unordered_set<int> visibleObjectIndices;
public:
	bool isObjectInFrustum(int objectIndex) const;

	void update(const RenderCamera &renderCamera, const SkyInstance &skyInst);
	void clear();
};

#endif
