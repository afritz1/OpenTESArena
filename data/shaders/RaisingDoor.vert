#version 450

layout(set = 0, binding = 0) uniform Camera
{
    mat4 viewProjection;
} camera;

layout(set = 2, binding = 0) uniform Transform
{
    mat4 translation, rotation, scale;
} transform;

layout(push_constant) uniform PushConstants
{
    vec4 preScaleTranslation;
} pc;

layout(location = 0) in vec3 vertInPosition;
layout(location = 1) in vec2 vertInTexCoord;

layout(location = 0) out vec2 fragInTexCoord;

void main()
{
    vec4 translatedVertex = vec4(vertInPosition, 1.0) + pc.preScaleTranslation;
    vec4 scaledVertex = transform.scale * translatedVertex;
    vec4 resultVertex = scaledVertex - pc.preScaleTranslation;

    gl_Position = camera.viewProjection * (transform.translation * (transform.rotation * resultVertex));
    fragInTexCoord = vertInTexCoord;
}
