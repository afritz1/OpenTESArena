#version 450

#define MAX_LIGHTS 256
#define MAX_LIGHTS_PER_BIN 32
#define LIGHT_LEVEL_COUNT 13
#define LIGHT_LEVEL_COUNT_REAL float(LIGHT_LEVEL_COUNT)
#define LAST_LIGHT_LEVEL (LIGHT_LEVEL_COUNT - 1)

struct Light
{
    float pointX;
    float pointY;
    float pointZ;
    float startRadius;
    float startRadiusSqr;
    float endRadius;
    float endRadiusSqr;
    float radiusDiffRecip;
};

struct LightBin
{
    uint indices[MAX_LIGHTS_PER_BIN];
};

layout(set = 0, binding = 1) uniform AmbientLight
{
    float percent;
} ambient;

layout(set = 0, binding = 5) uniform usampler2D lightTableSampler;

layout(set = 1, binding = 0) uniform Lights
{
    Light lights[MAX_LIGHTS];
} lights;

layout(set = 1, binding = 1) buffer LightBins
{
    LightBin bins[];
} lightBins;

layout(set = 1, binding = 2) buffer LightBinLightCounts
{
    uint counts[];
} lightBinLightCounts;

layout(set = 1, binding = 3) uniform LightBinDimensions
{
    uint width;
    uint height;
} lightBinDims;

layout(set = 3, binding = 0) uniform usampler2D textureSampler;

layout(location = 0) in vec2 fragInTexCoord;
layout(location = 1) in vec3 fragInWorldPoint;

layout(location = 0) out uint fragOutColor;

float getLightIntensity(vec3 point, Light light)
{
    float lightPointDiffX = light.pointX - point.x;
    float lightPointDiffY = light.pointY - point.y;
    float lightPointDiffZ = light.pointZ - point.z;
    float lightDistanceSqr = (lightPointDiffX * lightPointDiffX) + (lightPointDiffY * lightPointDiffY) + (lightPointDiffZ * lightPointDiffZ);

    if (lightDistanceSqr <= light.startRadiusSqr)
    {
        return 1.0;
    }
    else if (lightDistanceSqr >= light.endRadiusSqr)
    {
        return 0.0;
    }
    else
    {
        float lightDistance = sqrt(lightDistanceSqr);
        float lightDistancePercent = (lightDistance - light.startRadius) * light.radiusDiffRecip;
        return clamp(1.0 - lightDistancePercent, 0.0, 1.0);
    }
}

float getPerPixelLightIntensitySum()
{
    uint lightBinX = uint(gl_FragCoord.x) / lightBinDims.width;
    uint lightBinY = uint(gl_FragCoord.y) / lightBinDims.height;
    uint lightBinIndex = lightBinX + (lightBinY * lightBinDims.width);

    LightBin lightBin = lightBins.bins[lightBinIndex];
    uint lightBinLightCount = lightBinLightCounts.counts[lightBinIndex];

    float lightIntensitySum = ambient.percent;
    for (uint i = 0; i < lightBinLightCount; i++)
    {
        uint lightIndex = lightBin.indices[i];
        Light light = lights.lights[lightIndex];
        lightIntensitySum += getLightIntensity(fragInWorldPoint, light);
        if (lightIntensitySum >= 1.0)
        {
            lightIntensitySum = 1.0;
            break;
        }
    }

    return lightIntensitySum;
}

uint getLightLevel()
{
    float lightIntensitySum = 0.0;

    const bool isPerPixel = true;
    if (isPerPixel)
    {
        lightIntensitySum = getPerPixelLightIntensitySum();
    }
    else
    {
        lightIntensitySum = 1.0; // @todo meshLightingPercent, push constant?
    }

    float lightLevelReal = lightIntensitySum * LIGHT_LEVEL_COUNT_REAL;
    uint lightLevel = LAST_LIGHT_LEVEL - clamp(uint(lightLevelReal), 0, LAST_LIGHT_LEVEL);

    if (isPerPixel)
    {
        // @todo dithering
    }

    return lightLevel;
}

void main()
{
    uint texel = texture(textureSampler, fragInTexCoord).r;
    uint lightLevel = getLightLevel();
    fragOutColor = texelFetch(lightTableSampler, ivec2(texel, lightLevel), 0).r;
}
