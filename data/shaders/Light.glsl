#define MAX_LIGHTS 256
#define MAX_LIGHTS_PER_BIN 32
#define LIGHT_LEVEL_COUNT 13
#define LIGHT_LEVEL_COUNT_REAL float(LIGHT_LEVEL_COUNT)
#define LAST_LIGHT_LEVEL (LIGHT_LEVEL_COUNT - 1)

#define DITHER_MODE_NONE 0
#define DITHER_MODE_CLASSIC 1
#define DITHER_MODE_MODERN 2

#define DITHERING_MODERN_MASK_COUNT 4

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

layout(set = 1, binding = 3) buffer DitherBuffer
{
    uint isPixelDithered[];
} dither;

layout(set = 1, binding = 4) uniform LightBinDimensions
{
    uint pixelWidth;
    uint pixelHeight;
    uint binCountX;
    uint binCountY;
    uint ditherMode;
} lightBinDims;

layout(set = 3, binding = 2) uniform LightingMode
{
    bool isPerPixel;
} lightingMode;

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

uvec2 getFramebufferXY()
{
    uint pixelX = uint(gl_FragCoord.x) - 1;
    uint pixelY = uint(gl_FragCoord.y) - 1;
    return uvec2(pixelX, pixelY);
}

float getPerPixelLightIntensitySum(vec3 worldPoint)
{
    uvec2 framebufferXY = getFramebufferXY();
    uint lightBinX = framebufferXY.x / lightBinDims.pixelWidth;
    uint lightBinY = framebufferXY.y / lightBinDims.pixelHeight;
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

uint getLightLevel(vec3 worldPoint, float meshLightPercent, uvec2 framebufferDims)
{
    float lightIntensitySum = 0.0;

    const bool isPerPixel = lightingMode.isPerPixel;
    if (isPerPixel)
    {
        lightIntensitySum = getPerPixelLightIntensitySum(worldPoint);
    }
    else
    {
        lightIntensitySum = meshLightPercent;
    }

    float lightLevelReal = lightIntensitySum * LIGHT_LEVEL_COUNT_REAL;
    uint lightLevel = LAST_LIGHT_LEVEL - clamp(int(lightLevelReal), 0, LAST_LIGHT_LEVEL);

    if (isPerPixel)
    {
        uint ditherMode = lightBinDims.ditherMode;
        
        uvec2 framebufferXY = getFramebufferXY();
        uint pixelIndex = framebufferXY.x + (framebufferXY.y * framebufferDims.x); // @todo this should just modulo into the dither texture eventually
        uint framebufferPixelCount = framebufferDims.x * framebufferDims.y; // @todo this should be dither texture pixel count eventually

        bool shouldDither = false;
        if (ditherMode == DITHER_MODE_CLASSIC)
        {
            uint ditherBufferIndex = pixelIndex;
            shouldDither = dither.isPixelDithered[ditherBufferIndex] != 0;
        }
        else if (ditherMode == DITHER_MODE_MODERN)
        {
            if (lightIntensitySum < 1.0) // Keeps from dithering right next to the camera, not sure why the lowest dither level doesn't do this.
            {
                const uint modernMaskCount = DITHERING_MODERN_MASK_COUNT;
                float lightLevelFraction = lightLevelReal - floor(lightLevelReal);
                uint maskIndex = clamp(int(float(modernMaskCount) * lightLevelFraction), 0, modernMaskCount - 1);
                uint ditherBufferIndex = pixelIndex + (maskIndex * framebufferPixelCount);
                shouldDither = dither.isPixelDithered[ditherBufferIndex] != 0;
            }
        }

        if (shouldDither)
        {
            lightLevel = min(lightLevel + 1, LAST_LIGHT_LEVEL);
        }
    }

    return lightLevel;
}
