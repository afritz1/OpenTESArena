#version 450

layout(set = 0, binding = 1) uniform FramebufferDimensions
{
    uint width;
    uint height;
    float widthReal;
    float heightReal;
} framebuffer;

layout(set = 0, binding = 4) uniform usampler2D framebufferSampler;
layout(set = 0, binding = 6) uniform usampler2D lightTableSampler;
layout(set = 3, binding = 0) uniform usampler2D textureSampler;

layout(location = 0) in vec2 fragInTexCoord;
layout(location = 1) in vec3 fragInWorldPoint;

layout(location = 0) out uint fragOutColor;

const uint TEXELS_PER_LIGHT_LEVEL = 256;
const uint PALETTE_INDEX_LIGHT_LEVEL_LOWEST = 1;
const uint PALETTE_INDEX_LIGHT_LEVEL_HIGHEST = 13;
const uint PALETTE_INDEX_LIGHT_LEVEL_SRC1 = 14;
const uint PALETTE_INDEX_LIGHT_LEVEL_SRC2 = 15;
const uint PALETTE_INDEX_LIGHT_LEVEL_DST1 = 158;
const uint PALETTE_INDEX_LIGHT_LEVEL_DST2 = 159;

bool isLightLevelTexel(uint texel)
{
    return (texel >= PALETTE_INDEX_LIGHT_LEVEL_LOWEST) && (texel <= PALETTE_INDEX_LIGHT_LEVEL_HIGHEST);
}

uint getLightTableTexelIndex(uint texel)
{
    uint lightTableTexelIndex;

    if (isLightLevelTexel(texel))
    {
        uint texelAsLightLevel = texel - PALETTE_INDEX_LIGHT_LEVEL_LOWEST;

        vec2 framebufferUV = gl_FragCoord.xy / vec2(framebuffer.widthReal, framebuffer.heightReal);
        uint framebufferTexel = texture(framebufferSampler, framebufferUV).r;

        lightTableTexelIndex = framebufferTexel + (texelAsLightLevel * TEXELS_PER_LIGHT_LEVEL);
    }
    else
    {
        const int TEMP_lightLevel = 0; // @todo per pixel lighting (is a value even needed? This branch is for ghost eyes and... not sure what the else is. Fog cube?)

        uint lightTableOffset = TEMP_lightLevel * TEXELS_PER_LIGHT_LEVEL;
        if (texel == PALETTE_INDEX_LIGHT_LEVEL_SRC1)
        {
            lightTableTexelIndex = lightTableOffset + PALETTE_INDEX_LIGHT_LEVEL_DST1;
        }
        else if (texel == PALETTE_INDEX_LIGHT_LEVEL_SRC2)
        {
            lightTableTexelIndex = lightTableOffset + PALETTE_INDEX_LIGHT_LEVEL_DST2;
        }
        else
        {
            lightTableTexelIndex = lightTableOffset + texel;
        }
    }

    return lightTableTexelIndex;
}

void main()
{
    uint texel = texture(textureSampler, fragInTexCoord).r;
    if (texel == 0)
    {
        discard;
    }

    uint lightTableTexelIndex = getLightTableTexelIndex(texel);
    ivec2 lightTableTexelXY = ivec2(lightTableTexelIndex % TEXELS_PER_LIGHT_LEVEL, lightTableTexelIndex / TEXELS_PER_LIGHT_LEVEL);
    uint lightTableTexel = texelFetch(lightTableSampler, lightTableTexelXY, 0).r;
    fragOutColor = texelFetch(lightTableSampler, ivec2(lightTableTexel, 0), 0).r;
}
