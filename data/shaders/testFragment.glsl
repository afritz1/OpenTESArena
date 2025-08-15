#version 450

layout(set = 0, binding = 1) uniform usampler2D paletteSampler;
layout(set = 1, binding = 1) uniform usampler2D textureSampler;

layout(location = 0) in vec2 fragInTexCoord;

layout(location = 0) out vec4 fragOutColor;

void main()
{
    //uint texel = texture(textureSampler, fragInTexCoord).r;
    //ivec2 texelCoord = ivec2(fragInTexCoord * textureSize(textureSampler, 0));
    //uint texel = texelFetch(textureSampler, texelCoord, 0).r;

    fragOutColor = vec4(fract(fragInTexCoord), 0.0, 1.0);

    //uvec4 paletteColor = texelFetch(paletteSampler, ivec2(texel, 0), 0);
    //fragOutColor = vec4(float(paletteColor.r) / 255.0);
}
