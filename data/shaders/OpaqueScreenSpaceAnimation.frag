#version 450
#include "Light.glsl"

layout(set = 0, binding = 2) uniform ScreenSpaceAnimation
{
    float percent;
    float framebufferWidthReal;
    float framebufferHeightReal;
} screenSpaceAnim;

layout(set = 0, binding = 5) uniform usampler2D lightTableSampler;
layout(set = 3, binding = 0) uniform usampler2D textureSampler;

layout(location = 0) in vec2 fragInTexCoord;
layout(location = 1) in vec3 fragInWorldPoint;

layout(location = 0) out uint fragOutColor;

float getScreenSpaceAnimV()
{
    const int animFrameHeight = 100;
    int animFrameCount = textureSize(textureSampler, 0).y / animFrameHeight;
    float animFrameCountReal = float(animFrameCount);
    int animFrameIndex = clamp(int(animFrameCountReal * screenSpaceAnim.percent), 0, animFrameCount - 1);
    float animFrameIndexReal = float(animFrameIndex);

    float framebufferV = (gl_FragCoord.y / screenSpaceAnim.framebufferHeightReal) * 2.0;
    float normalizedV = (framebufferV >= 1.0) ? (framebufferV - 1.0) : framebufferV;
    return (normalizedV / animFrameCountReal) + (animFrameIndexReal / animFrameCountReal);
}

void main()
{
    vec2 screenSpaceUV = vec2(gl_FragCoord.x / screenSpaceAnim.framebufferWidthReal, getScreenSpaceAnimV());
    uint texel = texture(textureSampler, screenSpaceUV).r;
    uint lightLevel = getLightLevel(fragInWorldPoint, 0.0);
    fragOutColor = texelFetch(lightTableSampler, ivec2(texel, lightLevel), 0).r;
}
