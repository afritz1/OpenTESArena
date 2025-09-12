#version 450
#include "Light.glsl"

layout(set = 0, binding = 1) uniform FramebufferDimensions
{
    uint width;
    uint height;
    float widthReal;
    float heightReal;
} framebuffer;

layout(set = 0, binding = 6) uniform usampler2D lightTableSampler;
layout(set = 3, binding = 0) uniform usampler2D textureSampler;

layout(location = 0) in vec2 fragInTexCoord;
layout(location = 1) in vec3 fragInWorldPoint;

layout(location = 0) out uint fragOutColor;

const uint PALETTE_INDEX_PUDDLE_EVEN_ROW = 30;

void main()
{
    uint texel = texture(textureSampler, fragInTexCoord).r;
    if (texel == 0 || texel == PALETTE_INDEX_PUDDLE_EVEN_ROW)
    {
        discard;
    }
    
    uint lightLevel = getLightLevel(fragInWorldPoint, 0.0, uvec2(framebuffer.width, framebuffer.height));
    fragOutColor = texelFetch(lightTableSampler, ivec2(texel, lightLevel), 0).r;
}
