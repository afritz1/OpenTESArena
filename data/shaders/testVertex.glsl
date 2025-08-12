#version 450

layout(binding = 0) uniform Camera
{
    mat4 model, view, projection;
} camera;

layout(location = 0) in vec3 vertInPosition;
layout(location = 1) in vec2 vertInTexCoord;

layout(location = 0) out vec2 fragInTexCoord;

void main()
{
    gl_Position = camera.projection * (camera.view * (camera.model * vec4(vertInPosition, 1.0)));
    fragInTexCoord = vertInTexCoord;
}
