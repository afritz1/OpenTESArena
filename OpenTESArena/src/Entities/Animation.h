#ifndef ANIMATION_H
#define ANIMATION_H

#include <cstddef>
#include <vector>

// Stores the current state of a sprite animation. The IDs each reference a texture
// in the software renderer.

class Animation
{
private:
	std::vector<int> ids;
	double timePerFrame, currentTime;
	size_t index;
	bool loop;
public:
	Animation(const std::vector<int> &ids, double timePerFrame, bool loop);
	~Animation();

	// Gets the current texture ID. If the animation doesn't loop and is finished, it
	// returns the last ID.
	int getCurrentID() const;

	// Returns whether the animation has gone through all of its IDs. If 'loop' is true,
	// this method always returns false.
	bool isFinished() const;

	// Tick the animation by delta time.
	void tick(double dt);
};

#endif
