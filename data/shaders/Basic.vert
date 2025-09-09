#version 450

layout(set = 0, binding = 0) uniform Camera
{
    mat4 viewProjection;
    vec4 point;
    vec4 forward;
    vec4 forwardScaled;
    vec4 right;
    vec4 rightScaled;
    vec4 up;
    vec4 upScaledRecip;
} camera;

layout(set = 2, binding = 0) uniform Transform
{
    mat4 model;
} transform;

layout(location = 0) in vec3 vertInPosition;
layout(location = 1) in vec2 vertInTexCoord;

layout(location = 0) out vec2 fragInTexCoord;
layout(location = 1) out vec3 fragInWorldPoint;

void main()
{
    vec4 worldPoint = transform.model * vec4(vertInPosition, 1.0);

    gl_Position = camera.viewProjection * worldPoint;
    fragInTexCoord = vertInTexCoord;
    fragInWorldPoint = worldPoint.xyz;
}
