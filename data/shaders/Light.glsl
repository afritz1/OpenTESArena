#include "LightTypes.glsl"

layout(set = 0, binding = 2) uniform AmbientLight
{
    float percent;
} ambient;

layout(set = 1, binding = 0) uniform Lights
{
    Light lights[MAX_LIGHTS];
} lights;

layout(set = 1, binding = 1) readonly buffer LightBins
{
    LightBin bins[];
} lightBins;

layout(set = 1, binding = 2) readonly buffer LightBinLightCounts
{
    uint counts[];
} lightBinLightCounts;

layout(set = 1, binding = 3) uniform usampler2D ditherTexture;

layout(set = 1, binding = 4) uniform LightBinDimensions
{
    uint pixelWidth;
    uint pixelHeight;
    uint binCountX;
    uint binCountY;
    uint visibleLightCount;
    uint ditherMode;
} lightBinDims;

layout(set = 3, binding = 2) uniform LightingMode
{
    uint isPerPixel;
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
    uint pixelX = uint(gl_FragCoord.x);
    uint pixelY = uint(gl_FragCoord.y);
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

    const bool isPerPixel = lightingMode.isPerPixel != 0;
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
        uvec2 ditherTextureDims = textureSize(ditherTexture, 0);

        bool shouldDither = false;
        if (ditherMode == DITHER_MODE_CLASSIC)
        {
            uint ditherTexelX = framebufferXY.x % ditherTextureDims.x;
            uint ditherTexelY = framebufferXY.y % ditherTextureDims.y;
            uint ditherTexel = texelFetch(ditherTexture, ivec2(ditherTexelX, ditherTexelY), 0).r;
            shouldDither = ditherTexel != 0;
        }
        else if (ditherMode == DITHER_MODE_MODERN)
        {
            if (lightIntensitySum < 1.0) // Keeps from dithering right next to the camera, not sure why the lowest dither level doesn't do this.
            {
                const uint modernMaskCount = DITHERING_MODERN_MASK_COUNT;
                float lightLevelFraction = lightLevelReal - floor(lightLevelReal);
                uint maskIndex = clamp(int(float(modernMaskCount) * lightLevelFraction), 0, modernMaskCount - 1);
                
                // Each dither mask is square, stored vertically.
                uint ditherTextureMaskHeight = ditherTextureDims.x;

                uint ditherTexelX = framebufferXY.x % ditherTextureDims.x;
                uint ditherTexelY = (framebufferXY.y % ditherTextureMaskHeight) + (maskIndex * ditherTextureMaskHeight);
                uint ditherTexel = texelFetch(ditherTexture, ivec2(ditherTexelX, ditherTexelY), 0).r;
                shouldDither = ditherTexel != 0;
            }
        }

        if (shouldDither)
        {
            lightLevel = min(lightLevel + 1, LAST_LIGHT_LEVEL);
        }
    }

    return lightLevel;
}
