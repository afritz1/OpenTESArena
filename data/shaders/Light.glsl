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
    uint pixelWidth;
    uint pixelHeight;
    uint binCountX;
    uint binCountY;
} lightBinDims;

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

float getPerPixelLightIntensitySum(vec3 worldPoint)
{
    uint pixelX = uint(gl_FragCoord.x) - 1;
    uint pixelY = uint(gl_FragCoord.y) - 1;
    uint lightBinX = pixelX / lightBinDims.pixelWidth;
    uint lightBinY = pixelY / lightBinDims.pixelHeight;
    uint lightBinIndex = lightBinX + (lightBinY * lightBinDims.binCountX);

    LightBin lightBin = lightBins.bins[lightBinIndex];
    uint lightBinLightCount = lightBinLightCounts.counts[lightBinIndex];

    float lightIntensitySum = ambient.percent;
    for (uint i = 0; i < lightBinLightCount; i++)
    {
        uint lightIndex = lightBin.indices[i];
        Light light = lights.lights[lightIndex];
        lightIntensitySum += getLightIntensity(worldPoint, light);
        if (lightIntensitySum >= 1.0)
        {
            lightIntensitySum = 1.0;
            break;
        }
    }

    return lightIntensitySum;
}

uint getLightLevel(vec3 worldPoint)
{
    float lightIntensitySum = 0.0;

    const bool isPerPixel = true;
    if (isPerPixel)
    {
        lightIntensitySum = getPerPixelLightIntensitySum(worldPoint);
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
