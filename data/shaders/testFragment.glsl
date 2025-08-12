#version 450

layout(binding = 1) uniform sampler2D textureSampler;

layout(location = 0) in vec2 fragInTexCoord;

layout(location = 0) out vec4 fragOutColor;

void main()
{
    fragOutColor = vec4(texture(textureSampler, fragInTexCoord).rgb, 1.0);
}
