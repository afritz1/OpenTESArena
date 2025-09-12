#version 450

layout(push_constant) uniform PushConstant
{
    vec2 rectPoint;
    vec2 rectSize;
    vec2 windowSize;
} pc;

layout(location = 0) in vec2 vertInPosition;
layout(location = 1) in vec2 vertInTexCoord;

layout(location = 0) out vec2 fragInTexCoord;

void main()
{
    vec2 pixelPoint = pc.rectPoint + (vertInPosition * pc.rectSize);
    vec2 normalizedPoint = (2.0 * (pixelPoint / pc.windowSize)) - 1.0;
    gl_Position = vec4(normalizedPoint, 0.0, 1.0);
    fragInTexCoord = vertInTexCoord;
}
