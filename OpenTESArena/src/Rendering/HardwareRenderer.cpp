//Ideally I would use SDL's opengl bindings, but I'm lazy and didn't recomplie SDL with opengl, 
//so I used glBinding, which was already configured, instead.
#include <glbinding/Binding.h>
#include <glbinding/gl/gl.h>
#include <sstream>
#include <iostream>
//#include <SDL.h>
#include "../Utilities/Debug.h"
#include "../Math/MathUtils.h"
#include "HardwareRenderer.h"

using namespace gl;

//Voxel vetices
GLfloat vertices[] = {
	// positions          // normals           // texture coords
	//Back
	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
	 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
	 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
	 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
	-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
	//Front
	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
	 0.5f,	0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
	 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
	-0.5f,	0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
	//Left
	-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
	-0.5f,	0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
	-0.5f,	0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
	-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
	//Right
	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
	 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	 //Bottom
	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
	 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
	-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
	//Top
	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
	 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
	-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
};

/*GLfloat voxelPoints[]{
	0.0f, 0.0f, -1.0f,//Back
	0.0, 0.0, 1.0f,//Front
	-1.0f, 0.0f, 0.0f,//Left
	1.0f, 0.0f, 0.0f,//Right
	0.0f, -1.0f, 0.0f,//Bottom
	0.0f, 1.0f, 0.0f//Top
};*/

GLfloat voxelPoints[]{
	 1, 0, 0,
	-1, 0, 0,
	0, 1, 0,
	0,-1, 0,
	0, 0, 1,
	0, 0,-1
};

GLint voxelFaces[] = {5,4,1,0,3,2};

//Local Shaders, perhaps can be moved, but are fine here while testing
const char* cubeVert = R"glsl(
#version 410
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 transform;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;  
    TexCoords = aTexCoords;
    gl_Position = transform * vec4(FragPos, 1.0);
}
)glsl";

const char* cubeFrag = R"glsl(
#version 330 core
in vec2 TexCoords;
out vec4 FragColour;
uniform sampler2D tex;
void main(){
	FragColour = texture(tex, TexCoords);//vec4(0.1,0.5,0.25,1.0);
}
)glsl";

//Private Helper to check the shaders for errors
void checkErrors(GLuint index, const std::string& type) {
	GLint success;
	GLchar log[1024];

	std::stringstream logger;
	logger << "Error: " << type;

	if (type == "Program") {
		glGetProgramiv(index, GLenum::GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(index, 1024, nullptr, log);
			logger << "Link Error - " << log;
		}
	}
	else {
		logger << " Shader ";
		glGetShaderiv(index, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(index, 1024, NULL, log);
			logger << "Compile Error - " << log;
		}
	}
	if (!success)
		DebugException(logger.str());
}

//Private helper to compile t
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

HardwareRenderer::Voxel::Voxel() {
	//Generate the vertex array for a voxel
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	//Generate vertex buffer object for a voxel
	glGenBuffers(1, &vbo);
	glBindBuffer(GLenum::GL_ARRAY_BUFFER, vbo);
	//Give vbo the vertex data
	glBufferData(GLenum::GL_ARRAY_BUFFER, sizeof(vertices), vertices, GLenum::GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GLenum::GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);//First three are local space position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GLenum::GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));//Second three are local normal vector
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GLenum::GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));//Texture coords
	glEnableVertexAttribArray(2);

	//Compile shader program
	shaderID = compileShader(cubeVert, cubeFrag);
}

HardwareRenderer::Voxel::~Voxel() {
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
}

HardwareRenderer::HardwareRenderer()
{
	frameBuffer = frameVAO = frameVBO = colourBuffer = renderBuffer = width = height = 0;
}

GLuint texelTexture(int width, int height, const uint32_t* srcTexels) {
	GLuint ID = 0;

	glGenTextures(1, &ID);
	glBindTexture(GLenum::GL_TEXTURE_2D, ID);

	glTexImage2D(GLenum::GL_TEXTURE_2D, 0, GLenum::GL_RGBA, width, height, 0, GLenum::GL_RGBA, GLenum::GL_UNSIGNED_BYTE, srcTexels);
	// Set Texture wrap and filter modes
	glTexParameteri(GLenum::GL_TEXTURE_2D, GLenum::GL_TEXTURE_WRAP_S, GLenum::GL_REPEAT);
	glTexParameteri(GLenum::GL_TEXTURE_2D, GLenum::GL_TEXTURE_WRAP_T, GLenum::GL_REPEAT);
	glTexParameteri(GLenum::GL_TEXTURE_2D, GLenum::GL_TEXTURE_MIN_FILTER, GLenum::GL_NEAREST);
	glTexParameteri(GLenum::GL_TEXTURE_2D, GLenum::GL_TEXTURE_MAG_FILTER, GLenum::GL_NEAREST);
	// Unbind texture
	glBindTexture(GLenum::GL_TEXTURE_2D, 0);

	return ID;
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

			// If it's a white texel, it's used with night lights (i.e., yellow at night).
			const bool isWhite = (srcTexel.x == 1.0) && (srcTexel.y == 1.0) && (srcTexel.z == 1.0);

			if (isWhite)
			{
				texture.lightTexels.push_back(Int2(x, y));
			}
		}
	}
	texture.ID = texelTexture(VoxelTexture::WIDTH, VoxelTexture::HEIGHT,srcTexels);
}
void HardwareRenderer::init(int width, int height)
{
	this->width = width;
	this->height = height;
	this->voxelTextures = std::vector<VoxelTexture>(64);
	//set the size of the render viewport
	glViewport(0, 0, width, height);
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
}

void HardwareRenderer::createMap(const VoxelGrid& voxelGrid, int adjustedY, double ceilingHeight) {
	for (int x = 0; x < voxelGrid.getWidth(); x++) {
		for (int y = voxelGrid.getHeight(); y > 0; y--) {
			for (int z = 0; z < voxelGrid.getDepth(); z++) {
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

				if (voxelData.dataType != VoxelDataType::None) {
					//Trial and error to get the voxel y position to match the software renderer
					auto v = std::make_unique<Voxel>();
					v->model = Matrix4f::translation(x + 0.5f, (adjustedY + 0.33f) - ((y - 1.0f) * ceilingHeight), z + 0.5f) * Matrix4f::scale(1.0f, ceilingHeight, 1.0f);
					v->x = x;
					v->y = y;
					v->z = z;
					if (voxelData.dataType == VoxelDataType::Wall) {
						v->texture = voxelTextures.at(voxelData.wall.sideID);
					}
					else if (voxelData.dataType == VoxelDataType::Ceiling) {
						v->texture = voxelTextures.at(voxelData.ceiling.id);
					}
					map.push_back(std::move(v));
				}
			}
		}
	}
}

void HardwareRenderer::render(const Double3 & eye, const Double3 & direction, double fovY, double ceilingHeight, const VoxelGrid & voxelGrid, uint32_t * colourBuffer) {
	//From SoftwareRenderer
	const double widthReal = static_cast<double>(this->width);
	const double heightReal = static_cast<double>(this->height);
	const double aspect = widthReal / heightReal;

	const double projectionModifier = 1.20;
	const Camera camera(eye, direction, fovY, aspect, projectionModifier);

	//Use our fbo
	glBindFramebuffer(GLenum::GL_FRAMEBUFFER, frameBuffer);
	//Clear the colour and depth buffers
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(ClearBufferMask::GL_COLOR_BUFFER_BIT | ClearBufferMask::GL_DEPTH_BUFFER_BIT);
	glEnable(GLenum::GL_DEPTH_TEST);
	//Only render faces we are looking at
	glEnable(GLenum::GL_CULL_FACE);
	glFrontFace(GLenum::GL_CW);
	//glPolygonMode(GLenum::GL_FRONT_AND_BACK, GLenum::GL_LINE);//Wireframe for testing
	//Draw Calls
	//glUseProgram(shaderID);
	//Set the transform value in the shader to the values in camera.transform
	//glUniformMatrix4fv(glGetUniformLocation(shaderID, "transform"), 1, GL_FALSE, &camera.transform.x[0]);
	const int adjustedVoxelY = camera.getAdjustedEyeVoxelY(ceilingHeight);
	//Probably a better way to iterate over voxels, but keeping it simple for the moment
	float fogDistance = 15;
	
	if (firstrun) {
		createMap(voxelGrid, adjustedVoxelY, ceilingHeight);
		firstrun = false;
	}


	GLuint currentTexture = 0;
	for (auto& voxel : map) {
		if (voxel->x > camera.eyeVoxel.x - fogDistance && voxel->x< camera.eyeVoxel.x + fogDistance && voxel->z> camera.eyeVoxel.z - fogDistance && voxel->z < camera.eyeVoxel.z + fogDistance) {
			glBindVertexArray(voxel->vao);
			glUseProgram(voxel->shaderID);
			glUniformMatrix4fv(glGetUniformLocation(voxel->shaderID, "transform"), 1, GL_FALSE, &camera.transform.x[0]);
			glActiveTexture(GLenum::GL_TEXTURE0 + (currentTexture++));
			glBindTexture(GLenum::GL_TEXTURE_2D, voxel->texture.ID);
			glUniformMatrix4fv(glGetUniformLocation(voxel->shaderID, "model"), 1, GL_FALSE, &voxel->model.x[0]);
			glDrawArrays(GLenum::GL_TRIANGLES, 0, 36);
		}
	}

	//Read colour buffer to the SDL renderer
	glReadPixels(0, 0, width, height, GLenum::GL_RGBA, GLenum::GL_UNSIGNED_BYTE, colourBuffer);
	glBindFramebuffer(GLenum::GL_FRAMEBUFFER, 0);
}

HardwareRenderer::~HardwareRenderer()
{
	glDeleteBuffers(1, &colourBuffer);
	glDeleteBuffers(1, &renderBuffer);
}
