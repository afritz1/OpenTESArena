#version 450

layout(set = 0, binding = 1) uniform ScreenSpaceAnimation
{
    float percent;
    float framebufferWidthReal;
    float framebufferHeightReal;
} screenSpaceAnim;

layout(set = 2, binding = 0) uniform usampler2D textureSampler;

layout(location = 0) in vec2 fragInTexCoord;

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
    fragOutColor = texture(textureSampler, screenSpaceUV).r;
}
