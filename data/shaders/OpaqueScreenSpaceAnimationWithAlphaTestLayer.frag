#version 450
#include "Light.glsl"

layout(set = 0, binding = 1) uniform FramebufferDimensions
{
    uint width;
    uint height;
    float widthReal;
    float heightReal;
} framebuffer;

layout(set = 0, binding = 3) uniform ScreenSpaceAnimation
{
    float percent;
    float framebufferWidthReal;
    float framebufferHeightReal;
} screenSpaceAnim;

layout(set = 0, binding = 6) uniform usampler2D lightTableSampler;
layout(set = 3, binding = 0) uniform usampler2D mainTextureSampler;
layout(set = 3, binding = 1) uniform usampler2D layerTextureSampler;

layout(location = 0) in vec2 fragInTexCoord;
layout(location = 1) in vec3 fragInWorldPoint;

layout(location = 0) out uint fragOutColor;

float getScreenSpaceAnimV()
{
    const int animFrameHeight = 100;
    int animFrameCount = textureSize(mainTextureSampler, 0).y / animFrameHeight;
    float animFrameCountReal = float(animFrameCount);
    int animFrameIndex = clamp(int(animFrameCountReal * screenSpaceAnim.percent), 0, animFrameCount - 1);
    float animFrameIndexReal = float(animFrameIndex);

    float framebufferV = (gl_FragCoord.y / framebuffer.heightReal) * 2.0;
    float normalizedV = (framebufferV >= 1.0) ? (framebufferV - 1.0) : framebufferV;
    return (normalizedV / animFrameCountReal) + (animFrameIndexReal / animFrameCountReal);
}

void main()
{
    uint texel = texture(layerTextureSampler, fragInTexCoord).r;
    if (texel == 0)
    {
        vec2 screenSpaceUV = vec2(gl_FragCoord.x / framebuffer.widthReal, getScreenSpaceAnimV());
        texel = texture(mainTextureSampler, screenSpaceUV).r;
    }
    
    uint lightLevel = getLightLevel(fragInWorldPoint, 0.0, uvec2(framebuffer.width, framebuffer.height));
    fragOutColor = texelFetch(lightTableSampler, ivec2(texel, lightLevel), 0).r;
}
