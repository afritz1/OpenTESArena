#include <cassert>
#include <iostream>
#include <cmath>

#include <SDL2/SDL.h>

#include "PostProcessing.h"

// Some post processing techniques require separate source and destination buffers;
// use an assertion to verify that.

void PostProcessing::grayscale(const SDL_Surface *src, SDL_Surface *dst)
{
	assert(src->w == dst->w);
	assert(src->h == dst->h);

	const int area = src->w * src->h;
	const auto *srcPixels = static_cast<unsigned int*>(src->pixels);
	auto *dstPixels = static_cast<unsigned int*>(dst->pixels);

#pragma omp parallel for
	for (int i = 0; i < area; ++i)
	{
		const auto color = srcPixels[i];
		const auto r = static_cast<unsigned char>(color >> 16);
		const auto g = static_cast<unsigned char>(color >> 8);
		const auto b = static_cast<unsigned char>(color);

		auto averageComponent = static_cast<unsigned char>((r + g + b) / 3);
		auto averageColor = static_cast<unsigned int>(
			(averageComponent << 16) | (averageComponent << 8) | averageComponent);
		dstPixels[i] = averageColor;
	}
}

void PostProcessing::gammaCorrection(const SDL_Surface *src, SDL_Surface *dst,
	double gamma)
{
	assert(src->w == dst->w);
	assert(src->h == dst->h);

	const int area = src->w * src->h;
	const auto *srcPixels = static_cast<unsigned int*>(src->pixels);
	auto *dstPixels = static_cast<unsigned int*>(dst->pixels);
#pragma omp parallel for
	for (int i = 0; i < area; ++i)
	{
		const auto color = srcPixels[i];
		const auto r = static_cast<unsigned char>(color >> 16);
		const auto g = static_cast<unsigned char>(color >> 8);
		const auto b = static_cast<unsigned char>(color);

		auto gammaR = static_cast<unsigned int>(std::pow(r, gamma));
		auto gammaG = static_cast<unsigned int>(std::pow(g, gamma));
		auto gammaB = static_cast<unsigned int>(std::pow(b, gamma));

		const int low = 0;
		const int high = 255;
		gammaR = (gammaR > high) ? high : ((gammaR < low) ? low : gammaR);
		gammaG = (gammaG > high) ? high : ((gammaG < low) ? low : gammaG);
		gammaB = (gammaB > high) ? high : ((gammaB < low) ? low : gammaB);

		auto gammaColor = static_cast<unsigned int>(
			(static_cast<unsigned char>(gammaR) << 16) |
			(static_cast<unsigned char>(gammaG) << 8) |
			(static_cast<unsigned char>(gammaB)));
		dstPixels[i] = gammaColor;
	}
}

void PostProcessing::blur3x3(const SDL_Surface *src, SDL_Surface *dst)
{
	assert(src->w == dst->w);
	assert(src->h == dst->h);
	assert(src != dst);

	// Rewrite this for integer math. Right now it's designed for Float3's.

	// This kernel can be sped up quite a bit by using that special decomposition
	// method for doing a horizontal blur then a vertical blur.

	/*const int width = src->w;
	const int height = src->h;

	const auto *srcPixels = static_cast<unsigned int*>(src->pixels);
	auto *dstPixels = static_cast<unsigned int*>(dst->pixels);*/

	std::cerr << "PostProcessing::blur3x3 not done!" << "\n";
}
