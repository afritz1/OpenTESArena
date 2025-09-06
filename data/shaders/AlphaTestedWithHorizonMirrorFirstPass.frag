#version 450

layout(set = 0, binding = 1) uniform AmbientLight
{
    float percent;
} ambient;

layout(set = 3, binding = 0) uniform usampler2D textureSampler;

layout(location = 0) in vec2 fragInTexCoord;

layout(location = 0) out uint fragOutColor;

const uint PALETTE_INDEX_PUDDLE_EVEN_ROW = 30;

void main()
{
    uint texel = texture(textureSampler, fragInTexCoord).r;
    if (texel == 0 || texel == PALETTE_INDEX_PUDDLE_EVEN_ROW)
    {
        discard;
    }

    fragOutColor = texel;
}
