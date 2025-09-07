#version 450
#include "Light.glsl"

layout(set = 0, binding = 5) uniform usampler2D lightTableSampler;

layout(set = 3, binding = 0) uniform usampler2D textureSampler;

layout(location = 0) in vec2 fragInTexCoord;
layout(location = 1) in vec3 fragInWorldPoint;

layout(location = 0) out uint fragOutColor;

void main()
{
    uint texel = texture(textureSampler, fragInTexCoord).r;
    uint lightLevel = getLightLevel(fragInWorldPoint);
    fragOutColor = texelFetch(lightTableSampler, ivec2(texel, lightLevel), 0).r;
}
