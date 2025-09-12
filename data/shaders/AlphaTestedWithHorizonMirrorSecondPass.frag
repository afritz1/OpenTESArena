#version 450

layout(set = 0, binding = 1) uniform FramebufferDimensions
{
    uint width;
    uint height;
    float widthReal;
    float heightReal;
} framebuffer;

layout(set = 0, binding = 4) uniform usampler2D framebufferSampler;
layout(set = 0, binding = 7) uniform usampler2D skyBgTextureSampler;

layout(set = 0, binding = 8) uniform HorizonMirror
{
    float screenSpacePointX, screenSpacePointY;
} horizon;

layout(set = 3, binding = 0) uniform usampler2D textureSampler;

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

    bool isReflectedPixelInFramebuffer = (reflectedScreenSpacePointX >= 0.0) && (reflectedScreenSpacePointX < framebuffer.widthReal) &&
        (reflectedScreenSpacePointY >= 0.0) && (reflectedScreenSpacePointY < framebuffer.heightReal);
    if (isReflectedPixelInFramebuffer)
    {
        vec2 framebufferUV = vec2(reflectedScreenSpacePointX / framebuffer.widthReal, reflectedScreenSpacePointY / framebuffer.heightReal);
        reflectedTexel = texture(framebufferSampler, framebufferUV).r;
    }

    fragOutColor = reflectedTexel;
}
