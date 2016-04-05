#ifndef POST_PROCESSING_H
#define POST_PROCESSING_H

// These are slow on the CPU, but they're better than nothing because doing
// OpenCL kernels for each of them is low priority right now!

struct SDL_Surface;

class PostProcessing
{
private:
	PostProcessing() = delete;
	PostProcessing(const PostProcessing&) = delete;
	~PostProcessing() = delete;
public:
	static void grayscale(const SDL_Surface *src, SDL_Surface *dst);
	static void gammaCorrection(const SDL_Surface *src, SDL_Surface *dst, 
		double gamma);
	static void blur3x3(const SDL_Surface *src, SDL_Surface *dst);
};

#endif