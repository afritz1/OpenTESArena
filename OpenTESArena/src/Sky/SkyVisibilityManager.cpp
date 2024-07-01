#include "SkyInstance.h"
#include "SkyVisibilityManager.h"
#include "../Rendering/RenderCamera.h"

bool SkyVisibilityManager::isObjectInFrustum(int objectIndex) const
{
	return this->visibleObjectIndices.find(objectIndex) != this->visibleObjectIndices.end();
}

void SkyVisibilityManager::update(const RenderCamera &renderCamera, const SkyInstance &skyInst)
{
	this->visibleObjectIndices.clear();

	auto isSpaceObjectVisible = [&renderCamera, &skyInst](int objectIndex)
	{
		const SkyObjectInstance &skyObjectInst = skyInst.getSkyObjectInst(objectIndex);
		const Double3 &transformedDirection = skyObjectInst.transformedDirection;
		const double cameraDot = transformedDirection.dot(renderCamera.forward);
		return cameraDot >= -0.1;
	};

	// Just cull space objects for now. Might not need to cull anything else.
	for (int i = skyInst.landStart; i < skyInst.landEnd; i++)
	{
		this->visibleObjectIndices.emplace(i);
	}

	for (int i = skyInst.airStart; i < skyInst.airEnd; i++)
	{
		this->visibleObjectIndices.emplace(i);
	}

	for (int i = skyInst.moonStart; i < skyInst.moonEnd; i++)
	{
		if (isSpaceObjectVisible(i))
		{
			this->visibleObjectIndices.emplace(i);
		}
	}

	for (int i = skyInst.sunStart; i < skyInst.sunEnd; i++)
	{
		if (isSpaceObjectVisible(i))
		{
			this->visibleObjectIndices.emplace(i);
		}
	}

	for (int i = skyInst.starStart; i < skyInst.starEnd; i++)
	{
		if (isSpaceObjectVisible(i))
		{
			this->visibleObjectIndices.emplace(i);
		}
	}

	for (int i = skyInst.lightningStart; i < skyInst.lightningEnd; i++)
	{
		this->visibleObjectIndices.emplace(i);
	}
}

void SkyVisibilityManager::clear()
{
	this->visibleObjectIndices.clear();
}
