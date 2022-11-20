#ifndef RENDER_SHADER_UTILS_H
#define RENDER_SHADER_UTILS_H

enum class VertexShaderType
{
	Default
};

enum class PixelShaderType
{
	Opaque,
	AlphaTested,
	OpaqueWithAlphaTestLayer
};

enum class TextureSamplingType
{
	Default,
	ScreenSpaceRepeatY // Chasms.
};

#endif
