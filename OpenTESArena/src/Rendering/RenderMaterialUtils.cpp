#include <algorithm>

#include "RenderMaterialUtils.h"

#include "components/debug/Debug.h"

RenderMaterialKey::RenderMaterialKey()
{
	this->vertexShaderType = static_cast<VertexShaderType>(-1);
	this->fragmentShaderType = static_cast<FragmentShaderType>(-1);

	std::fill(std::begin(this->textureIDs), std::end(this->textureIDs), -1);
	this->textureCount = 0;

	this->lightingType = static_cast<RenderLightingType>(-1);

	this->enableBackFaceCulling = false;
	this->enableDepthRead = false;
	this->enableDepthWrite = false;
}

bool RenderMaterialKey::operator==(const RenderMaterialKey &other) const
{
	if (this == &other)
	{
		return true;
	}

	if (this->vertexShaderType != other.vertexShaderType)
	{
		return false;
	}

	if (this->fragmentShaderType != other.fragmentShaderType)
	{
		return false;
	}

	if (this->textureCount != other.textureCount)
	{
		return false;
	}

	for (int i = 0; i < this->textureCount; i++)
	{
		if (this->textureIDs[i] != other.textureIDs[i])
		{
			return false;
		}
	}

	if (this->lightingType != other.lightingType)
	{
		return false;
	}

	if (this->enableBackFaceCulling != other.enableBackFaceCulling)
	{
		return false;
	}

	if (this->enableDepthRead != other.enableDepthRead)
	{
		return false;
	}

	if (this->enableDepthWrite != other.enableDepthWrite)
	{
		return false;
	}

	return true;
}

void RenderMaterialKey::init(VertexShaderType vertexShaderType, FragmentShaderType fragmentShaderType, Span<const ObjectTextureID> textureIDs,
	RenderLightingType lightingType, bool enableBackFaceCulling, bool enableDepthRead, bool enableDepthWrite)
{
	this->vertexShaderType = vertexShaderType;
	this->fragmentShaderType = fragmentShaderType;

	DebugAssert(textureIDs.getCount() <= RenderMaterialKey::MAX_TEXTURE_COUNT);
	std::copy(textureIDs.begin(), textureIDs.end(), std::begin(this->textureIDs));
	this->textureCount = textureIDs.getCount();

	this->lightingType = lightingType;

	this->enableBackFaceCulling = enableBackFaceCulling;
	this->enableDepthRead = enableDepthRead;
	this->enableDepthWrite = enableDepthWrite;
}

RenderMaterial::RenderMaterial()
{
	this->id = -1;
}

void RenderMaterial::init(RenderMaterialKey key, RenderMaterialID id)
{
	this->key = key;
	this->id = id;
}
