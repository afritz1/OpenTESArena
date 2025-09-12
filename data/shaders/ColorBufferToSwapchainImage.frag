#version 450

layout(set = 0, binding = 0) uniform usampler2D framebufferSampler;
layout(set = 0, binding = 1) uniform sampler2D paletteSampler;

layout(location = 0) in vec2 fragInTexCoord;

layout(location = 0) out vec4 fragOutColor;

void main()
{
    uint texel = texture(framebufferSampler, fragInTexCoord).r;
    fragOutColor = texelFetch(paletteSampler, ivec2(texel, 0), 0);
}
