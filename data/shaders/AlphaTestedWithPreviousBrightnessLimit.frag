#version 450

layout(set = 0, binding = 1) uniform FramebufferDimensions
{
    uint width;
    uint height;
    float widthReal;
    float heightReal;
} framebuffer;

layout(set = 0, binding = 4) uniform usampler2D framebufferSampler;
layout(set = 0, binding = 5) uniform sampler2D paletteSampler;
layout(set = 3, binding = 0) uniform usampler2D textureSampler;

layout(location = 0) in vec2 fragInTexCoord;

layout(location = 0) out uint fragOutColor;

void main()
{
    const uint brightnessLimit = 0x3F; // Highest 8-bit value each RGB component can be.
    const float brightnessLimitReal = float(brightnessLimit) / 255.0;

    vec2 framebufferUV = gl_FragCoord.xy / vec2(framebuffer.widthReal, framebuffer.heightReal);
    uint framebufferTexel = texture(framebufferSampler, framebufferUV).r;
    vec4 framebufferColor = texelFetch(paletteSampler, ivec2(framebufferTexel, 0), 0);
    uint texel = framebufferTexel;

    bool isDarkEnough = (framebufferColor.r <= brightnessLimitReal) && (framebufferColor.g <= brightnessLimitReal) && (framebufferColor.b <= brightnessLimitReal);
    if (isDarkEnough)
    {
        uint textureTexel = texture(textureSampler, fragInTexCoord).r;
        if (textureTexel == 0)
        {
            discard;
        }

        texel = textureTexel;
    }

    fragOutColor = texel;
}
