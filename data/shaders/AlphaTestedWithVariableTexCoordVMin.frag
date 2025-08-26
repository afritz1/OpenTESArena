#version 450

layout(set = 0, binding = 1) uniform sampler2D paletteSampler;
layout(set = 2, binding = 0) uniform usampler2D textureSampler;

layout(push_constant) uniform PushConstants
{
    layout(offset = 16) float vMin;
} pc;

layout(location = 0) in vec2 fragInTexCoord;

layout(location = 0) out vec4 fragOutColor;

void main()
{
    vec2 uv = fragInTexCoord;
    uv.y = clamp(pc.vMin + ((1.0 - pc.vMin) * uv.y), pc.vMin, 1.0);

    uint texel = texture(textureSampler, uv).r;
    if (texel == 0)
    {
        discard;
    }

    vec4 paletteColor = texelFetch(paletteSampler, ivec2(texel, 0), 0);
    fragOutColor = paletteColor;
}
