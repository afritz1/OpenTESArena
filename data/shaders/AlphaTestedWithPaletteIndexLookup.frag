#version 450
#include "Light.glsl"

layout(set = 0, binding = 5) uniform usampler2D lightTableSampler;
layout(set = 3, binding = 0) uniform usampler2D textureSampler;
layout(set = 3, binding = 1) uniform usampler2D replacementSampler;

layout(location = 0) in vec2 fragInTexCoord;
layout(location = 1) in vec3 fragInWorldPoint;

layout(location = 0) out uint fragOutColor;

void main()
{
    uint texel = texture(textureSampler, fragInTexCoord).r;
    if (texel == 0)
    {
        discard;
    }
    
    uint replacementTexel = texelFetch(replacementSampler, ivec2(texel, 0), 0).r;
    uint lightLevel = getLightLevel(fragInWorldPoint, 0.0);    
    fragOutColor = texelFetch(lightTableSampler, ivec2(replacementTexel, lightLevel), 0).r;
}
