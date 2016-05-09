#ifndef CL_PROGRAM_H
#define CL_PROGRAM_H

#include <memory>
#include <vector>

#include "CL\cl2.hpp"

// The OpenCL object should be kept alive while the game data object is alive.
// Otherwise, it would reload the kernel whenever the game world panel was 
// re-entered, which is unnecessary. Therefore, this object should be kept in
// the game data object.

struct SDL_Surface;

class CLProgram
{
private:
	static const std::string PATH;
	static const std::string FILENAME;
	static const std::string ENTRY_FUNCTION;

	cl::Device device; // The device selected from the devices list.
	cl::Context context;
	cl::CommandQueue commandQueue;
	cl::Program program;
	cl::Kernel kernel;
	cl::Buffer colorBuffer;
	int width, height;

	std::string getBuildReport() const;
	std::string getErrorString(cl_int error) const;
public:
	CLProgram(int width, int height);
	~CLProgram();

	static std::vector<cl::Platform> getPlatforms();
	static std::vector<cl::Device> getDevices(const cl::Platform &platform,
		cl_device_type type);

	void render(SDL_Surface *dst);
};

#endif
