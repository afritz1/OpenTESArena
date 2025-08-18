#version 450

layout(set = 0, binding = 0) uniform Camera
{
    mat4 viewProjection;
} camera;

layout(set = 1, binding = 0) uniform Transform
{
    mat4 translation, rotation, scale;
} transform;

layout(location = 0) in vec3 vertInPosition;
layout(location = 1) in vec2 vertInTexCoord;

layout(location = 0) out vec2 fragInTexCoord;

void main()
{
    mat4 modelMatrix = transform.translation * (transform.rotation * transform.scale);
    gl_Position = camera.viewProjection * (modelMatrix * vec4(vertInPosition, 1.0));
    fragInTexCoord = vertInTexCoord;
}
