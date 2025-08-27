#version 450

layout(set = 0, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) in vec2 fragInTexCoord;

layout(location = 0) out vec4 fragOutColor;

void main()
{
    vec4 texel = texture(textureSampler, fragInTexCoord);
    fragOutColor = texel;
}
