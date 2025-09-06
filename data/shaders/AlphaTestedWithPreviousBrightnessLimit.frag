#version 450

// @todo make a descriptor set binding that's just FramebufferDimensions so the water/lava shaders and this can share that binding
layout(set = 0, binding = 2) uniform ScreenSpaceAnimation
{
    float percent;
    float framebufferWidthReal;
    float framebufferHeightReal;
} screenSpaceAnim;

layout(set = 0, binding = 3) uniform usampler2D framebufferSampler;
layout(set = 0, binding = 4) uniform sampler2D paletteSampler;
layout(set = 3, binding = 0) uniform usampler2D textureSampler;

layout(location = 0) in vec2 fragInTexCoord;

layout(location = 0) out uint fragOutColor;

void main()
{
    const uint brightnessLimit = 0x3F; // Highest 8-bit value each RGB component can be.
    const float brightnessLimitReal = float(brightnessLimit) / 255.0;

    vec2 framebufferUV = gl_FragCoord.xy / vec2(screenSpaceAnim.framebufferWidthReal, screenSpaceAnim.framebufferHeightReal);
    uint framebufferTexel = texture(framebufferSampler, framebufferUV).r;
    vec4 framebufferColor = texelFetch(paletteSampler, ivec2(framebufferTexel, 0), 0);
    uint texel = framebufferTexel;

    bool isDarkEnough = (framebufferColor.r <= brightnessLimitReal) && (framebufferColor.g <= brightnessLimitReal) && (framebufferColor.b <= brightnessLimitReal);
    if (isDarkEnough)
    {
        uint textureTexel = texture(textureSampler, fragInTexCoord).r;
        if (textureTexel != 0)
        {
            texel = textureTexel;
        }
    }

    fragOutColor = texel;
}
