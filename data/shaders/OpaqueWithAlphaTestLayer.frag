#version 450

layout(set = 2, binding = 0) uniform usampler2D mainTextureSampler;
layout(set = 2, binding = 1) uniform usampler2D layerTextureSampler;

layout(location = 0) in vec2 fragInTexCoord;

layout(location = 0) out uint fragOutColor;

void main()
{
    uint texel = texture(layerTextureSampler, fragInTexCoord).r;
    if (texel == 0)
    {
        texel = texture(mainTextureSampler, fragInTexCoord).r;
    }

    fragOutColor = texel;
}
