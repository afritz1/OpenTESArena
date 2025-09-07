#version 450

layout(set = 0, binding = 0) uniform Camera
{
    mat4 viewProjection;
} camera;

layout(set = 2, binding = 0) uniform Transform
{
    mat4 model;
} transform;

layout(location = 0) in vec3 vertInPosition;
layout(location = 1) in vec2 vertInTexCoord;

layout(location = 0) out vec2 fragInTexCoord;

void main()
{
    gl_Position = camera.viewProjection * (transform.model * vec4(vertInPosition, 1.0));
    fragInTexCoord = vertInTexCoord;
}
