#version 450
#include "Light.glsl"

layout(set = 0, binding = 5) uniform usampler2D lightTableSampler;

layout(set = 3, binding = 0) uniform usampler2D textureSampler;

layout(push_constant) uniform PushConstants
{
    float vMin;
} pc;

layout(location = 0) in vec2 fragInTexCoord;
layout(location = 1) in vec3 fragInWorldPoint;

layout(location = 0) out uint fragOutColor;

void main()
{
    vec2 uv = fragInTexCoord;
    uv.y = clamp(pc.vMin + ((1.0 - pc.vMin) * uv.y), pc.vMin, 1.0);

    uint texel = texture(textureSampler, uv).r;
    if (texel == 0)
    {
        discard;
    }
    
    uint lightLevel = getLightLevel(fragInWorldPoint, 0.0);
    fragOutColor = texelFetch(lightTableSampler, ivec2(texel, lightLevel), 0).r;
}
