#version 450

// @todo make a descriptor set binding that's just FramebufferDimensions so the water/lava shaders and this can share that binding
layout(set = 0, binding = 2) uniform ScreenSpaceAnimation
{
    float percent;
    float framebufferWidthReal;
    float framebufferHeightReal;
} screenSpaceAnim;

layout(set = 0, binding = 3) uniform usampler2D framebufferSampler;
layout(set = 0, binding = 6) uniform usampler2D skyBgTextureSampler;
layout(set = 0, binding = 7) uniform HorizonMirror
{
    float screenSpacePointX, screenSpacePointY;
} horizon;

layout(set = 2, binding = 0) uniform usampler2D textureSampler;

layout(location = 0) in vec2 fragInTexCoord;

layout(location = 0) out uint fragOutColor;

const uint PALETTE_INDEX_PUDDLE_EVEN_ROW = 30;

void main()
{
    uint texel = texture(textureSampler, fragInTexCoord).r;
    if (texel != PALETTE_INDEX_PUDDLE_EVEN_ROW)
    {
        discard;
    }

    float reflectedScreenSpacePointX = gl_FragCoord.x;
    float reflectedScreenSpacePointY = horizon.screenSpacePointY + (horizon.screenSpacePointY - gl_FragCoord.y);

    uint fallbackTexel = texelFetch(skyBgTextureSampler, ivec2(0, 0), 0).r;
    uint reflectedTexel = fallbackTexel;

    bool isReflectedPixelInFramebuffer = (reflectedScreenSpacePointX >= 0.0) && (reflectedScreenSpacePointX < screenSpaceAnim.framebufferWidthReal) &&
        (reflectedScreenSpacePointY >= 0.0) && (reflectedScreenSpacePointY < screenSpaceAnim.framebufferHeightReal);
    if (isReflectedPixelInFramebuffer)
    {
        vec2 framebufferUV = vec2(reflectedScreenSpacePointX / screenSpaceAnim.framebufferWidthReal, reflectedScreenSpacePointY / screenSpaceAnim.framebufferHeightReal);
        reflectedTexel = texture(framebufferSampler, framebufferUV).r;
    }

    fragOutColor = reflectedTexel;
}
