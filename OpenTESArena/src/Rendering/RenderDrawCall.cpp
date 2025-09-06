#include "RenderDrawCall.h"

RenderDrawCall::RenderDrawCall()
{
	this->clear();
}

void RenderDrawCall::clear()
{
	this->transformBufferID = -1;
	this->transformIndex = -1;
	this->positionBufferID = -1;
	this->normalBufferID = -1;
	this->texCoordBufferID = -1;
	this->indexBufferID = -1;
	this->materialID = -1;
	this->multipassType = RenderMultipassType::None;
}
