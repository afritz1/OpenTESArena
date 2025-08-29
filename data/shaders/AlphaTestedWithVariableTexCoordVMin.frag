#version 450

layout(set = 0, binding = 1) uniform AmbientLight
{
    float percent;
} ambient;

layout(set = 2, binding = 0) uniform usampler2D textureSampler;

layout(push_constant) uniform PushConstants
{
    layout(offset = 16) float vMin;
} pc;

layout(location = 0) in vec2 fragInTexCoord;

layout(location = 0) out uint fragOutColor;

void main()
{
    vec2 uv = fragInTexCoord;
    uv.y = clamp(pc.vMin + ((1.0 - pc.vMin) * uv.y), pc.vMin, 1.0);

    uint texel = texture(textureSampler, uv).r;
    if (texel == 0)
    {
        discard;
    }

    fragOutColor = texel;
}
