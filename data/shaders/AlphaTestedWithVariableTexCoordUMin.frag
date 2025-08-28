#version 450

layout(set = 2, binding = 0) uniform usampler2D textureSampler;

layout(push_constant) uniform PushConstants
{
    float uMin;
} pc;

layout(location = 0) in vec2 fragInTexCoord;

layout(location = 0) out uint fragOutColor;

void main()
{
    vec2 uv = fragInTexCoord;
    uv.x = clamp(pc.uMin + ((1.0 - pc.uMin) * uv.x), pc.uMin, 1.0);

    uint texel = texture(textureSampler, uv).r;
    if (texel == 0)
    {
        discard;
    }

    fragOutColor = texel;
}
