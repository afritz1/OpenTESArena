#include "Animation.h"

Animation::Animation(const std::vector<int> &ids, double timePerFrame, bool loop)
	: ids(ids)
{
	this->timePerFrame = timePerFrame;
	this->currentTime = 0.0;
	this->index = 0;
	this->loop = loop;
}

Animation::~Animation()
{

}

int Animation::getCurrentID() const
{
	return this->ids.at(!this->isFinished() ? this->index : (this->ids.size() - 1));
}

bool Animation::isFinished() const
{
	return !this->loop && (this->index == this->ids.size());
}

void Animation::tick(double dt)
{
	if (this->index < this->ids.size())
	{
		this->currentTime += dt;

		// Step to the next ID if its duration has passed.
		if (this->currentTime >= this->timePerFrame)
		{
			this->currentTime = 0.0;
			this->index++;

			// Return to the beginning if at the end and looping is enabled.
			if (this->loop && (this->index == this->ids.size()))
			{
				this->index = 0;
			}
		}
	}
}
