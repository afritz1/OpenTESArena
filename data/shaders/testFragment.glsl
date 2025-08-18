#version 450

layout(set = 0, binding = 1) uniform usampler2D paletteSampler;
layout(set = 2, binding = 0) uniform usampler2D textureSampler;

layout(location = 0) in vec2 fragInTexCoord;

layout(location = 0) out vec4 fragOutColor;

void main()
{
    uint texel = texture(textureSampler, fragInTexCoord).r;
    if (texel == 0)
    {
        discard;
    }

    uvec4 paletteColor = texelFetch(paletteSampler, ivec2(texel, 0), 0);
    fragOutColor = paletteColor / vec4(255.0);
}
