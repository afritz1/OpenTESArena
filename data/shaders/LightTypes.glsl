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
