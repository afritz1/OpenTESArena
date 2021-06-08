#ifndef ANIMATION_STATE_H
#define ANIMATION_STATE_H

#include <functional>

class AnimationState
{
private:
	double targetSeconds, currentSeconds;
	bool looping;
	std::function<void()> onFinished;
public:
	AnimationState();

	void init(double targetSeconds, bool looping, std::function<void()> &&onFinished);
	void init(double targetSeconds, bool looping);

	double getPercent() const;

	void reset();
	void update(double dt);
};

#endif
