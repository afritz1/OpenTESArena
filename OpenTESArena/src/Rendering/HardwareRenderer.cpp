#ifdef HAVE_OPENGL
#include <iostream>
#include <sstream>
//Ideally I would use SDL's opengl bindings, but the default OpenGL library is lacking,
//so I used glBinding to access the 'extension' functions.
#include "glbinding/Binding.h"
#include "glbinding/gl/gl.h"
#include "renderdoc_app.h"

#include "HardwareRenderer.h"
#include "../Utilities/Debug.h"
#include "../Math/MathUtils.h"

using namespace gl;

namespace
{
//Voxel vetices
std::vector<GLfloat> voxelVertices = {
	-0.5f, -0.5f, -0.5f,//0
	 0.5f, -0.5f, -0.5f,//1
	 0.5f,  0.5f, -0.5f,//2
	-0.5f,  0.5f, -0.5f,//3
	-0.5f, -0.5f,  0.5f,//4
	 0.5f, -0.5f,  0.5f,//5
	 0.5f,  0.5f,  0.5f,//6
	-0.5f,  0.5f,  0.5f,//7
};
//Indices, allow us to only define 8 vertices and re-use them
std::vector<GLuint> voxelIndices = {
	0,2,1,2,0,3,//Back
	4,5,6,6,7,4,//Front
	7,3,0,0,4,7,//Left
	6,1,2,1,6,5,//Right
	0,1,5,5,4,0,//Bottom
	3,6,2,6,3,7 //Top
};

//Local Shaders, perhaps can be moved, but are fine here while testing
const char* cubeVert = R"glsl(
#version 400 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in mat4 model;
layout (location = 5) in float texIdx;

uniform mat4 transform;

out vec3 TexCoords;
out float TexIdx;

void main()
{
	//Reversing the position fixes walls and floors, but not ceilings
	TexCoords = -aPos;
	TexIdx = texIdx;
    gl_Position = transform * model * vec4(aPos, 1.0);
}
)glsl";

const char* cubeFrag = R"glsl(
#version 400 core
in vec3 TexCoords;
in float TexIdx;
out vec4 FragColour;
uniform samplerCubeArray tex;
void main(){
	vec4 result = texture(tex, vec4(TexCoords,TexIdx));//vec4(TexCoords,1.0);
	FragColour = result.bgra;
}
)glsl";

//Private Helper to check the shaders for errors
void checkErrors(GLuint index, const std::string& type) {
	GLint success;
	GLchar log[1024];

	std::stringstream logger;
	logger << "Error: " << type;

	if (type == "Program")
	{
		glGetProgramiv(index, GLenum::GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(index, 1024, nullptr, log);
			logger << "Link Error - " << log;
		}
	}
	else
	{
		logger << " Shader ";
		glGetShaderiv(index, GL_COMPILE_STATUS, &success);
		if (!success) 
		{
			glGetShaderInfoLog(index, 1024, NULL, log);
			logger << "Compile Error - " << log;
		}
	}
	if (!success)
		DebugException(logger.str());
}

//Private helper to compile shaders
GLuint compileShader(const char* vertexSource, const char* fragmentSource, const char* geometrySource = nullptr)
{
	GLuint sVertex, sFragment, gShader, ID;
	// Vertex Shader
	sVertex = glCreateShader(GLenum::GL_VERTEX_SHADER);
	glShaderSource(sVertex, 1, &vertexSource, nullptr);
	glCompileShader(sVertex);
	checkErrors(sVertex, "Vertex");
	// Fragment Shader
	sFragment = glCreateShader(GLenum::GL_FRAGMENT_SHADER);
	glShaderSource(sFragment, 1, &fragmentSource, nullptr);
	glCompileShader(sFragment);
	checkErrors(sFragment, "Fragment");
	if (geometrySource != nullptr)
	{
		gShader = glCreateShader(GLenum::GL_GEOMETRY_SHADER);
		glShaderSource(gShader, 1, &geometrySource, NULL);
		glCompileShader(gShader);
		checkErrors(gShader, "Geometry");
	}
	// Shader Program
	ID = glCreateProgram();
	glAttachShader(ID, sVertex);
	glAttachShader(ID, sFragment);
	if (geometrySource != nullptr)
		glAttachShader(ID, gShader);
	glLinkProgram(ID);
	checkErrors(ID, "Program");
	// Delete shaders as they are now part of the program
	glDeleteShader(sVertex);
	glDeleteShader(sFragment);
	if (geometrySource != nullptr)
		glDeleteShader(gShader);

	return ID;
}
}

HardwareRenderer::Camera::Camera(const Double3& eye, const Double3& direction,
	double fovY, double aspect, double projectionModifier)
	: eye(eye), direction(direction)
{
	// Variations of eye position for certain voxel calculations.
	this->eyeVoxelReal = Double3(
		std::floor(eye.x),
		std::floor(eye.y),
		std::floor(eye.z));
	this->eyeVoxel = Int3(
		static_cast<int>(this->eyeVoxelReal.x),
		static_cast<int>(this->eyeVoxelReal.y),
		static_cast<int>(this->eyeVoxelReal.z));

	// Camera axes. We trick the 2.5D ray caster into thinking the player is always looking
	// straight forward, but we use the Y component of the player's direction to offset 
	// projected coordinates via Y-shearing.
	const Double3 forwardXZ = Double3(direction.x, 0.0, direction.z).normalized();
	const Double3 rightXZ = forwardXZ.cross(Double3::UnitY).normalized();

	// Transformation matrix (model matrix isn't required because it's just the identity).
	this->transform = [&eye, &forwardXZ, &rightXZ, fovY, aspect, projectionModifier]()
	{
		// Global up vector, scaled by the projection modifier (i.e., to account for tall pixels).
		const Double3 up = Double3::UnitY * projectionModifier;

		const Matrix4f view = Matrix4f::view(Float3(eye), Float3(forwardXZ), Float3(rightXZ), Float3(up));
		const Matrix4f projection = Matrix4f::perspective(fovY, aspect, 0.0001, 1000.0);
		return  projection * view;
	}();
	this->forwardX = forwardXZ.x;
	this->forwardZ = forwardXZ.z;
	this->rightX = rightXZ.x;
	this->rightZ = rightXZ.z;

	this->fovY = fovY;

	// Zoom of the camera, based on vertical field of view.
	this->zoom = MathUtils::verticalFovToZoom(fovY);
	this->aspect = aspect;

	// Forward and right modifiers, for interpolating 3D vectors across the screen and
	// so vertical FOV and aspect ratio are taken into consideration.
	this->forwardZoomedX = this->forwardX * this->zoom;
	this->forwardZoomedZ = this->forwardZ * this->zoom;
	this->rightAspectedX = this->rightX * this->aspect;
	this->rightAspectedZ = this->rightZ * this->aspect;

	// Left and right 2D vectors of the view frustum (at left and right edges of the screen).
	const Double2 frustumLeft = Double2(
		this->forwardZoomedX - this->rightAspectedX,
		this->forwardZoomedZ - this->rightAspectedZ).normalized();
	const Double2 frustumRight = Double2(
		this->forwardZoomedX + this->rightAspectedX,
		this->forwardZoomedZ + this->rightAspectedZ).normalized();
	this->frustumLeftX = frustumLeft.x;
	this->frustumLeftZ = frustumLeft.y;
	this->frustumRightX = frustumRight.x;
	this->frustumRightZ = frustumRight.y;

	// Vertical angle of the camera relative to the horizon.
	this->yAngleRadians = [&direction]()
	{
		// Get the length of the direction vector's projection onto the XZ plane.
		const double xzProjection = std::sqrt(
			(direction.x * direction.x) + (direction.z * direction.z));

		if (direction.y > 0.0)
		{
			// Above the horizon.
			return std::acos(xzProjection);
		}
		else if (direction.y < 0.0)
		{
			// Below the horizon.
			return -std::acos(xzProjection);
		}
		else
		{
			// At the horizon.
			return 0.0;
		}
	}();

	// Y-shearing is the distance that projected Y coordinates are translated by based on the 
	// player's 3D direction and field of view. First get the player's angle relative to the 
	// horizon, then get the tangent of that angle. The Y component of the player's direction
	// must be clamped less than 1 because 1 would imply they are looking straight up or down, 
	// which is impossible in 2.5D rendering (the vertical line segment of the view frustum 
	// would be infinitely high or low). The camera code should take care of the clamping for us.

	// Get the number of screen heights to translate all projected Y coordinates by, relative to
	// the current zoom. As a reference, this should be some value roughly between -1.0 and 1.0
	// for "acceptable skewing" at a vertical FOV of 90.0. If the camera is not clamped, this
	// could theoretically be between -infinity and infinity, but it would result in far too much
	// skewing.
	this->yShear = std::tan(this->yAngleRadians) * this->zoom;
}

int HardwareRenderer::Camera::getAdjustedEyeVoxelY(double ceilingHeight) const
{
	return static_cast<int>(this->eye.y / ceilingHeight);
}

HardwareRenderer::VoxelTexel::VoxelTexel()
{
	this->r = 0.0;
	this->g = 0.0;
	this->b = 0.0;
	this->emission = 0.0;
	this->transparent = false;
}

HardwareRenderer::HardwareRenderer()
{
	frameBuffer = voxelVAO = voxelVBO = cubemapArray = shaderID = colourBuffer = renderBuffer = width = height = 0;
}

void HardwareRenderer::init(int width, int height)
{
	this->width = width;
	this->height = height;
	this->voxelTextures = std::vector<VoxelTexture>(64);
	//set the size of the render viewport
	glViewport(0, 0, width, height);
	gl::glEnable(gl::GLenum::GL_BLEND);
	gl::glBlendFunc(gl::GLenum::GL_SRC_ALPHA, gl::GLenum::GL_ONE_MINUS_SRC_ALPHA);
	//Create and bind to the frame buffer object
	glGenFramebuffers(1, &frameBuffer);
	glBindFramebuffer(GLenum::GL_FRAMEBUFFER, frameBuffer);

	//Create the empty colour buffer texture
	glGenTextures(1, &colourBuffer);
	glBindTexture(GLenum::GL_TEXTURE_2D, colourBuffer);
	glTexImage2D(GLenum::GL_TEXTURE_2D, 0, GLenum::GL_RGB, width, height, 0, GLenum::GL_RGB, GLenum::GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GLenum::GL_TEXTURE_2D, GLenum::GL_TEXTURE_MIN_FILTER, GLenum::GL_LINEAR);
	glTexParameteri(GLenum::GL_TEXTURE_2D, GLenum::GL_TEXTURE_MAG_FILTER, GLenum::GL_LINEAR);
	glBindTexture(GLenum::GL_TEXTURE_2D, 0);

	//Attach to the fbo
	glFramebufferTexture2D(GLenum::GL_FRAMEBUFFER, GLenum::GL_COLOR_ATTACHMENT0, GLenum::GL_TEXTURE_2D, colourBuffer, 0);

	//Create render buffer object for depth and stencil buffers
	glGenRenderbuffers(1, &renderBuffer);
	glBindRenderbuffer(GLenum::GL_RENDERBUFFER, renderBuffer);
	glRenderbufferStorage(GLenum::GL_RENDERBUFFER, GLenum::GL_DEPTH24_STENCIL8, width, height);
	glBindRenderbuffer(GLenum::GL_RENDERBUFFER, 0);

	//Attach
	glFramebufferRenderbuffer(GLenum::GL_FRAMEBUFFER, GLenum::GL_DEPTH_STENCIL_ATTACHMENT, GLenum::GL_RENDERBUFFER, renderBuffer);

	//Make sure the the frame buffer is 'complete' so we can use it
	if (glCheckFramebufferStatus(GLenum::GL_FRAMEBUFFER) != GLenum::GL_FRAMEBUFFER_COMPLETE) {
		DebugException("Error: Framebuffer not Complete");
	}

	glBindFramebuffer(GLenum::GL_FRAMEBUFFER, 0); //Unbind

	glGenTextures(1, &cubemapArray);

	glBindTexture(GLenum::GL_TEXTURE_CUBE_MAP_ARRAY, cubemapArray);
	glTexStorage3D(GLenum::GL_TEXTURE_CUBE_MAP_ARRAY, 1, GLenum::GL_RGBA8, VoxelTexture::WIDTH, VoxelTexture::HEIGHT, 6 * 64);
	glTexParameteri(GLenum::GL_TEXTURE_CUBE_MAP_ARRAY, GLenum::GL_TEXTURE_MIN_FILTER, GLenum::GL_NEAREST);
	glTexParameteri(GLenum::GL_TEXTURE_CUBE_MAP_ARRAY, GLenum::GL_TEXTURE_MAG_FILTER, GLenum::GL_NEAREST);
	glTexParameteri(GLenum::GL_TEXTURE_CUBE_MAP_ARRAY, GLenum::GL_TEXTURE_WRAP_S, GLenum::GL_CLAMP_TO_EDGE);
	glTexParameteri(GLenum::GL_TEXTURE_CUBE_MAP_ARRAY, GLenum::GL_TEXTURE_WRAP_T, GLenum::GL_CLAMP_TO_EDGE);
}

void HardwareRenderer::generateCubeMap(int index, int width, int height, const uint32_t* srcTexels)
{
	//Bind to cubemap array texture
	glBindTexture(GLenum::GL_TEXTURE_CUBE_MAP_ARRAY, cubemapArray);

	//give each layer-face the same value 
	for (unsigned int i = 0; i < 6; i++) 
	{
		glTexSubImage3D(GLenum::GL_TEXTURE_CUBE_MAP_ARRAY, 0, 0, 0, i + (6 * index), width, height, 1, GLenum::GL_BGRA, GLenum::GL_UNSIGNED_INT_8_8_8_8_REV, srcTexels);
	}
}

void HardwareRenderer::setVoxelTexture(int id, const uint32_t* srcTexels)
{
	// Clear the selected texture.
	VoxelTexture& texture = this->voxelTextures.at(id);
	std::fill(texture.texels.begin(), texture.texels.end(), VoxelTexel());
	texture.lightTexels.clear();

	for (int y = 0; y < VoxelTexture::HEIGHT; y++)
	{
		for (int x = 0; x < VoxelTexture::WIDTH; x++)
		{
			// @todo: change this calculation for rotated textures. Make sure to have a 
			// source index and destination index.
			// - "dstX" and "dstY" should be calculated, and also used with lightTexels.
			const int index = x + (y * VoxelTexture::WIDTH);

			// Convert ARGB color from integer to double-precision format. This does waste
			// an extreme amount of memory (32 bytes per pixel!), but it's not a big deal
			// for Arena's textures (eight textures is a megabyte).
			const Double4 srcTexel = Double4::fromARGB(srcTexels[index]);
			VoxelTexel& dstTexel = texture.texels[index];
			dstTexel.r = srcTexel.x;
			dstTexel.g = srcTexel.y;
			dstTexel.b = srcTexel.z;
			dstTexel.transparent = srcTexel.w == 0.0;
			if (dstTexel.transparent) texture.hasAlpha = true;

			// If it's a white texel, it's used with night lights (i.e., yellow at night).
			const bool isWhite = (srcTexel.x == 1.0) && (srcTexel.y == 1.0) && (srcTexel.z == 1.0);

			if (isWhite)
			{
				texture.lightTexels.push_back(Int2(x, y));
			}
		}
	}
	generateCubeMap(id, VoxelTexture::WIDTH, VoxelTexture::HEIGHT, srcTexels);
}

void HardwareRenderer::createMap(const VoxelGrid& voxelGrid, int adjustedY, double ceilingHeight)
{
	std::vector<Matrix4f> modelMatrices;//the position matrices of every voxel
	std::vector<GLuint> textureIndices;//texture index for each voxel
	std::vector<Matrix4f> transparentModels;//store transparent voxels
	std::vector<GLuint> transparentTextures;//store transparent textures
	for (int x = 0; x <= voxelGrid.getWidth(); x++)
	{
		for (int y = voxelGrid.getHeight(); y >= 0; y--)
		{
			for (int z = 0; z <= voxelGrid.getDepth(); z++)
			{
				bool voxelIsValid =
					(x >= 0) &&
					(y >= 0) &&
					(z >= 0) &&
					(x < voxelGrid.getWidth()) &&
					(y < voxelGrid.getHeight()) &&
					(z < voxelGrid.getDepth());
				if (!voxelIsValid) continue;
				auto voxel = voxelGrid.getVoxel(x, y, z);
				auto voxelData = voxelGrid.getVoxelData(voxel);

				if (voxelData.dataType != VoxelDataType::None)
				{
					
					int id=0;

					if (voxelData.dataType == VoxelDataType::Ceiling)
					{
						id  = voxelData.ceiling.id;
					}
					else if (voxelData.dataType == VoxelDataType::Floor)
					{
						id = voxelData.floor.id;
					}
					else if (voxelData.dataType == VoxelDataType::Wall)
					{
						id = voxelData.wall.sideID;
					}
					else if (voxelData.dataType == VoxelDataType::Chasm)
					{
						id = voxelData.chasm.id;
					}
					else if (voxelData.dataType == VoxelDataType::Door)
					{
						id = voxelData.door.id;
					}
					else if (voxelData.dataType == VoxelDataType::Raised)
					{
						id = voxelData.raised.sideID;
					}

					//Trial and error to get the voxel y position to match the software renderer
					//There has to be a way to work this out mathematically, because the value is 'good enough' but there is 
					//a tiny bit of noticable movement I can't fix by eye
					Matrix4f model = Matrix4f::translation(x + 0.5f, (adjustedY + 0.32815f) - ((y - 1.0f) * ceilingHeight), z + 0.5f) * Matrix4f::scale(1.0f, ceilingHeight, 1.0f);

					if (voxelTextures.at(id).hasAlpha)
					{
						transparentTextures.push_back(id);
						transparentModels.push_back(model);
					}
					else
					{
						textureIndices.push_back(id);
						modelMatrices.push_back(model);
					}
				}
			}
		}
	}

	//Concatenate opaque and transparent voxels so blending occurs correctly
	modelMatrices.insert(modelMatrices.end(), std::make_move_iterator(transparentModels.begin()), std::make_move_iterator(transparentModels.end()));
	textureIndices.insert(textureIndices.end(), std::make_move_iterator(transparentTextures.begin()), std::make_move_iterator(transparentTextures.end()));

	amount = modelMatrices.size();

	//Create and bind to the vertex buffer object
	glGenBuffers(1, &voxelVBO);
	glBindBuffer(GLenum::GL_ARRAY_BUFFER, voxelVBO);
	//Batch the buffer data, create empty buffer -> fill with vertex information -> fill with model information -> fill with texture information
	glBufferData(GLenum::GL_ARRAY_BUFFER, (sizeof(GLfloat) * voxelVertices.size()) + (amount * sizeof(Matrix4f)) + (amount * sizeof(GLuint)), nullptr, GLenum::GL_STATIC_DRAW);
	glBufferSubData(GLenum::GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * voxelVertices.size(), &voxelVertices[0]);
	glBufferSubData(GLenum::GL_ARRAY_BUFFER, sizeof(GLfloat) * voxelVertices.size(), amount * sizeof(Matrix4f), &modelMatrices[0]);
	glBufferSubData(GLenum::GL_ARRAY_BUFFER, sizeof(GLfloat) * voxelVertices.size() + amount * sizeof(Matrix4f), amount * sizeof(GLuint), &textureIndices[0]);

	//Create the vertex array
	glGenVertexArrays(1, &voxelVAO);
	glBindVertexArray(voxelVAO);
	//Tell opengl where the vertices are
	glVertexAttribPointer(0, 3, GLenum::GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glEnableVertexAttribArray(0);

	//Offsets of the model matrix
	GLsizei vec4Size = 4 * sizeof(float);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GLenum::GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(sizeof(GLfloat) * voxelVertices.size()));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GLenum::GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(sizeof(GLfloat) * voxelVertices.size() + vec4Size));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GLenum::GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(sizeof(GLfloat) * voxelVertices.size() + (2 * vec4Size)));
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GLenum::GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(sizeof(GLfloat) * voxelVertices.size() + (3 * vec4Size)));

	//Divide the matrix so when we instance the voxel, we change which model matrix to use
	glVertexAttribDivisor(1, 1);
	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);

	//Same for textures
	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 1, GLenum::GL_UNSIGNED_INT, GL_FALSE, sizeof(GLint), (void*)((sizeof(GLfloat) * voxelVertices.size()) + (amount * sizeof(Matrix4f))));
	glVertexAttribDivisor(5, 1);

	//Create element buffer
	glGenBuffers(1, &voxelEBO);
	glBindBuffer(GLenum::GL_ELEMENT_ARRAY_BUFFER, voxelEBO);
	//Give it the indices
	glBufferData(GLenum::GL_ELEMENT_ARRAY_BUFFER, voxelIndices.size() * sizeof(GLuint), &voxelIndices[0], GLenum::GL_STATIC_DRAW);

	//Compile the voxels shader program
	shaderID = compileShader(cubeVert, cubeFrag);
}

void HardwareRenderer::render(const Double3& eye, const Double3& direction, double fovY, double ceilingHeight, const VoxelGrid& voxelGrid, uint32_t* colourBuffer) {
	//From SoftwareRenderer
	const double widthReal = static_cast<double>(this->width);
	const double heightReal = static_cast<double>(this->height);
	const double aspect = widthReal / heightReal;

	const double projectionModifier = 1.20;
	const Camera camera(eye, direction, fovY, aspect, projectionModifier);

	//Use our framebuffer
	glBindFramebuffer(GLenum::GL_FRAMEBUFFER, frameBuffer);
	//Clear the colour and depth buffers
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(ClearBufferMask::GL_COLOR_BUFFER_BIT | ClearBufferMask::GL_DEPTH_BUFFER_BIT);
	glEnable(GLenum::GL_DEPTH_TEST);
	//Only render faces we are looking at
	glEnable(GLenum::GL_CULL_FACE);
	//glPolygonMode(GLenum::GL_FRONT_AND_BACK, GLenum::GL_LINE);//Wireframe for testing
	//Draw Calls
	const int adjustedVoxelY = camera.getAdjustedEyeVoxelY(ceilingHeight);
	float fogDistance = 20;
	//On first run, generate the voxel grid (will need to update when a new level is loaded)
	if (firstrun)
	{
		createMap(voxelGrid, adjustedVoxelY, ceilingHeight);
	}

	//Use the voxel shader program
	glUseProgram(shaderID);
	//bind to the voxel vertex array
	glBindVertexArray(voxelVAO);
	//Bind to the cubemap array texture
	glActiveTexture(GLenum::GL_TEXTURE0);
	glBindTexture(GLenum::GL_TEXTURE_CUBE_MAP_ARRAY, cubemapArray);
	//Set the transform value in the shader to the values in camera.transform
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "transform"), 1, GL_FALSE, &camera.transform.x[0]);
	//Draw 'amount' instances of the voxel
	glDrawElementsInstanced(GLenum::GL_TRIANGLES, voxelIndices.size(), GLenum::GL_UNSIGNED_INT, 0, amount);

	//Read colour buffer to the SDL renderer
	glReadPixels(0, 0, width, height, GLenum::GL_RGBA, GLenum::GL_UNSIGNED_BYTE, colourBuffer);
	glBindFramebuffer(GLenum::GL_FRAMEBUFFER, 0);
	if (firstrun)
		firstrun = false;
}

HardwareRenderer::~HardwareRenderer()
{
	glDeleteVertexArrays(1, &voxelVAO);
	glDeleteBuffers(1, &voxelEBO);
	glDeleteBuffers(1, &voxelVBO);
	glDeleteBuffers(1, &colourBuffer);
	glDeleteBuffers(1, &renderBuffer);
	glDeleteProgram(shaderID);
}
#endif