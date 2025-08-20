#version 450

//layout(set = 0, binding = 1) uniform sampler2D paletteSampler;
//layout(set = 2, binding = 0) uniform usampler2D textureSampler;

layout(location = 0) in vec2 fragInTexCoord;

layout(location = 0) out vec4 fragOutColor;

void main()
{
    //uint texel = texture(textureSampler, fragInTexCoord).r;
    //vec4 paletteColor = texelFetch(paletteSampler, ivec2(texel, 0), 0);
    //fragOutColor = paletteColor;
    fragOutColor = vec4(fragInTexCoord, 0.0, 1.0);
}
