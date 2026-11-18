#include <algorithm>

#include "SDL.h"

#include "TextureUtils.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/TextureAsset.h"
#include "../Assets/TextureManager.h"
#include "../Math/Rect.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/RendererUtils.h"
#include "../UI/ArenaFontName.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextRenderUtils.h"

void TextureUtils::populateGeneratedTexture(UiTexturePatternType type, Span2D<uint32_t> outPixels, TextureManager &textureManager, Renderer &renderer)
{
	const int outputWidth = outPixels.getWidth();
	const int outputHeight = outPixels.getHeight();

	constexpr uint32_t clearColor = Colors::TransparentRGBA;
	outPixels.fill(clearColor);

	auto writeToOutput = [&outPixels](Span2D<const uint8_t> srcTexels, const Palette &palette, int dstX, int dstY)
	{
		for (int y = 0; y < srcTexels.getHeight(); y++)
		{
			for (int x = 0; x < srcTexels.getWidth(); x++)
			{
				const uint8_t srcColor = srcTexels.get(x, y);
				const uint32_t dstColor = palette[srcColor].toRGBA();
				outPixels.set(dstX + x, dstY + y, dstColor);
			}
		}
	};

	if (type == UiTexturePatternType::Parchment)
	{
		DebugAssert(outputWidth >= TextureUtils::PARCHMENT_MIN_WIDTH);
		DebugAssert(outputHeight >= TextureUtils::PARCHMENT_MIN_HEIGHT);

		const std::string &tilesPaletteFilename = ArenaTextureName::CharacterCreation;
		const std::optional<PaletteID> tilesPaletteID = textureManager.tryGetPaletteID(tilesPaletteFilename.c_str());
		if (!tilesPaletteID.has_value())
		{
			DebugLogErrorFormat("Couldn't get tile palette ID for \"%s\".", tilesPaletteFilename.c_str());
			return;
		}

		const Palette &palette = textureManager.getPaletteHandle(*tilesPaletteID);
		const std::string &tilesFilename = ArenaTextureName::Parchment;
		const std::optional<TextureBuilderIdGroup> tilesTextureBuilderIDs = textureManager.tryGetTextureBuilderIDs(tilesFilename.c_str());
		if (!tilesTextureBuilderIDs.has_value())
		{
			DebugLogErrorFormat("Couldn't get tiles texture builder IDs for \"%s\".", tilesFilename.c_str());
			return;
		}

		const TextureBuilder &topLeftTextureBuilder = textureManager.getTextureBuilderHandle(tilesTextureBuilderIDs->getID(0));
		const TextureBuilder &topTextureBuilder = textureManager.getTextureBuilderHandle(tilesTextureBuilderIDs->getID(1));
		const TextureBuilder &topRightTextureBuilder = textureManager.getTextureBuilderHandle(tilesTextureBuilderIDs->getID(2));
		const TextureBuilder &leftTextureBuilder = textureManager.getTextureBuilderHandle(tilesTextureBuilderIDs->getID(3));
		const TextureBuilder &bodyTextureBuilder = textureManager.getTextureBuilderHandle(tilesTextureBuilderIDs->getID(4));
		const TextureBuilder &rightTextureBuilder = textureManager.getTextureBuilderHandle(tilesTextureBuilderIDs->getID(5));
		const TextureBuilder &bottomLeftTextureBuilder = textureManager.getTextureBuilderHandle(tilesTextureBuilderIDs->getID(6));
		const TextureBuilder &bottomTextureBuilder = textureManager.getTextureBuilderHandle(tilesTextureBuilderIDs->getID(7));
		const TextureBuilder &bottomRightTextureBuilder = textureManager.getTextureBuilderHandle(tilesTextureBuilderIDs->getID(8));
		Span2D<const uint8_t> topLeftTexels = topLeftTextureBuilder.getTexels8();
		Span2D<const uint8_t> topTexels = topTextureBuilder.getTexels8();
		Span2D<const uint8_t> topRightTexels = topRightTextureBuilder.getTexels8();
		Span2D<const uint8_t> leftTexels = leftTextureBuilder.getTexels8();
		Span2D<const uint8_t> bodyTexels = bodyTextureBuilder.getTexels8();
		Span2D<const uint8_t> rightTexels = rightTextureBuilder.getTexels8();
		Span2D<const uint8_t> bottomLeftTexels = bottomLeftTextureBuilder.getTexels8();
		Span2D<const uint8_t> bottomTexels = bottomTextureBuilder.getTexels8();
		Span2D<const uint8_t> bottomRightTexels = bottomRightTextureBuilder.getTexels8();

		// Draw body tiles.
		for (int y = topLeftTexels.getHeight(); y < (outputHeight - topRightTexels.getHeight()); y += bodyTexels.getHeight())
		{
			for (int x = topLeftTexels.getWidth(); x < (outputWidth - topRightTexels.getWidth()); x += bodyTexels.getWidth())
			{
				writeToOutput(bodyTexels, palette, x, y);
			}
		}

		// Draw edge tiles.
		for (int y = topLeftTexels.getHeight(); y < (outputHeight - bottomLeftTexels.getHeight()); y += leftTexels.getHeight())
		{
			writeToOutput(leftTexels, palette, 0, y);
			writeToOutput(rightTexels, palette, outputWidth - rightTexels.getWidth(), y);
		}

		for (int x = topLeftTexels.getWidth(); x < (outputWidth - topRightTexels.getWidth()); x += topTexels.getWidth())
		{
			writeToOutput(topTexels, palette, x, 0);
			writeToOutput(bottomTexels, palette, x, outputHeight - bottomTexels.getHeight());
		}

		// Draw corner tiles.
		writeToOutput(topLeftTexels, palette, 0, 0);
		writeToOutput(topRightTexels, palette, outputWidth - topRightTexels.getWidth(), 0);
		writeToOutput(bottomLeftTexels, palette, 0, outputHeight - bottomLeftTexels.getHeight());
		writeToOutput(bottomRightTexels, palette, outputWidth - bottomRightTexels.getWidth(), outputHeight - bottomRightTexels.getHeight());
	}
	else if (type == UiTexturePatternType::Dark)
	{
		DebugAssert(outputWidth >= 4);
		DebugAssert(outputHeight >= 4);

		constexpr uint32_t fillColor = Color(28, 24, 36, 255).toRGBA();
		constexpr uint32_t topColor = Color(36, 36, 48, 255).toRGBA();
		constexpr uint32_t bottomColor = Color(12, 12, 24, 255).toRGBA();
		constexpr uint32_t rightColor = Color(56, 69, 77, 255).toRGBA();
		constexpr uint32_t leftColor = bottomColor;
		constexpr uint32_t topRightColor = Color(69, 85, 89, 255).toRGBA();
		constexpr uint32_t bottomRightColor = Color(36, 36, 48, 255).toRGBA();

		outPixels.fill(fillColor);

		// Color edges.
		for (int x = 0; x < outputWidth; x++)
		{
			outPixels.set(x, 0, topColor);
			outPixels.set(x, 1, topColor);
			outPixels.set(x, outputHeight - 2, bottomColor);
			outPixels.set(x, outputHeight - 1, bottomColor);
		}

		for (int y = 0; y < outputHeight; y++)
		{
			outPixels.set(0, y, leftColor);
			outPixels.set(1, y, leftColor);
			outPixels.set(outputWidth - 2, y, rightColor);
			outPixels.set(outputWidth - 1, y, rightColor);
		}

		// Color corners.
		outPixels.set(1, 0, topColor);
		outPixels.set(outputWidth - 2, 0, topColor);
		outPixels.set(outputWidth - 1, 0, topRightColor);
		outPixels.set(outputWidth - 2, 1, topRightColor);
		outPixels.set(outputWidth - 2, outputHeight - 2, bottomRightColor);
		outPixels.set(outputWidth - 2, outputHeight - 1, bottomColor);
		outPixels.set(outputWidth - 1, outputHeight - 1, bottomRightColor);
	}
	else if (type == UiTexturePatternType::ShopkeeperTitle)
	{
		DebugAssert(outputWidth >= (6 + 6));
		DebugAssert(outputHeight == 20);

		const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(ArenaPaletteName::Default);
		DebugAssert(paletteID.has_value());
		const Palette &palette = textureManager.getPaletteHandle(*paletteID);

		const std::optional<TextureBuilderID> leftTextureBuilderID = textureManager.tryGetTextureBuilderID(ArenaTextureName::BarterTitleLeft.c_str());
		const std::optional<TextureBuilderID> middleTextureBuilderID = textureManager.tryGetTextureBuilderID(ArenaTextureName::BarterTitleMiddle.c_str());
		const std::optional<TextureBuilderID> rightTextureBuilderID = textureManager.tryGetTextureBuilderID(ArenaTextureName::BarterTitleRight.c_str());
		DebugAssert(leftTextureBuilderID.has_value());
		DebugAssert(middleTextureBuilderID.has_value());
		DebugAssert(rightTextureBuilderID.has_value());
		const TextureBuilder &leftTextureBuilder = textureManager.getTextureBuilderHandle(*leftTextureBuilderID);
		const TextureBuilder &middleTextureBuilder = textureManager.getTextureBuilderHandle(*middleTextureBuilderID);
		const TextureBuilder &rightTextureBuilder = textureManager.getTextureBuilderHandle(*rightTextureBuilderID);

		const Rect leftRect(0, 0, leftTextureBuilder.width, leftTextureBuilder.height);
		const Rect middleRect(leftTextureBuilder.width, 0, middleTextureBuilder.width, middleTextureBuilder.height);
		const Rect rightRect(outputWidth - rightTextureBuilder.width, 0, rightTextureBuilder.width, rightTextureBuilder.height);

		Span2D<const uint8_t> leftSrcPixels = leftTextureBuilder.getTexels8();
		Span2D<const uint8_t> middleSrcPixels = middleTextureBuilder.getTexels8();
		Span2D<const uint8_t> rightSrcPixels = rightTextureBuilder.getTexels8();

		// Left edge.
		for (int y = 0; y < leftRect.height; y++)
		{
			for (int x = 0; x < leftRect.width; x++)
			{
				const uint8_t srcColor = leftSrcPixels.get(x, y);
				const uint32_t srcColorRGBA = palette[srcColor].toRGBA();
				outPixels.set(x, y, srcColorRGBA);
			}
		}

		// Middle.
		int middleXOffset = middleRect.x;
		for (int remainingWidthToFill = outputWidth - leftRect.width - rightRect.width; remainingWidthToFill > 0; remainingWidthToFill -= middleRect.width)
		{			
			const int currentMiddleWidth = std::min(middleRect.width, remainingWidthToFill);
			for (int y = 0; y < middleRect.height; y++)
			{
				for (int x = 0; x < currentMiddleWidth; x++)
				{
					const uint8_t srcColor = middleSrcPixels.get(x, y);
					const uint32_t srcColorRGBA = palette[srcColor].toRGBA();
					outPixels.set(middleXOffset + x, y, srcColorRGBA);
				}
			}

			middleXOffset += middleRect.width;
		}

		// Right edge.
		for (int y = 0; y < rightRect.height; y++)
		{
			for (int x = 0; x < rightRect.width; x++)
			{
				const uint8_t srcColor = rightSrcPixels.get(x, y);
				const uint32_t srcColorRGBA = palette[srcColor].toRGBA();
				outPixels.set(rightRect.x + x, y, srcColorRGBA);
			}
		}
	}
	else if (type == UiTexturePatternType::PlayerInventoryHighlight)
	{
		DebugAssert(outputWidth >= 2);
		DebugAssert(outputHeight >= 2);

		constexpr uint32_t highlightColor = Color(199, 32, 0, 255).toRGBA();

		for (int x = 0; x < outputWidth; x++)
		{
			outPixels.set(x, 0, highlightColor);
			outPixels.set(x, outputHeight - 1, highlightColor);
		}

		for (int y = 0; y < outputHeight; y++)
		{
			outPixels.set(0, y, highlightColor);
			outPixels.set(outputWidth - 1, y, highlightColor);
		}
	}
	else if (type == UiTexturePatternType::Custom1)
	{
		DebugAssert(outputWidth >= 3);
		DebugAssert(outputHeight >= 3);

		constexpr uint32_t fillColor = Color(85, 85, 97, 255).toRGBA();
		constexpr uint32_t lightBorder = Color(125, 125, 145, 255).toRGBA();
		constexpr uint32_t darkBorder = Color(40, 40, 48, 255).toRGBA();

		outPixels.fill(fillColor);

		// Color edges.
		for (int x = 0; x < outputWidth; x++)
		{
			outPixels.set(x, 0, lightBorder);
			outPixels.set(x, outputHeight - 1, darkBorder);
		}

		for (int y = 0; y < outputHeight; y++)
		{
			outPixels.set(0, y, darkBorder);
			outPixels.set(outputWidth - 1, y, lightBorder);
		}

		// Color corners.
		outPixels.set(0, 0, fillColor);
		outPixels.set(outputWidth - 1, outputHeight - 1, fillColor);
	}
	else
	{
		DebugLogErrorFormat("Unrecognized pattern type %d.", type);
	}
}

Surface TextureUtils::createTooltip(const std::string &text, const FontLibrary &fontLibrary)
{
	const char *fontName = ArenaFontName::D;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName, &fontDefIndex))
	{
		DebugCrash("Couldn't get font definition for \"" + std::string(fontName) + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	constexpr int lineSpacing = 1;
	TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(text, fontDef, std::nullopt, lineSpacing);
	constexpr int padding = 4;

	Surface surface = Surface::createWithFormat(textureGenInfo.width + padding, textureGenInfo.height + padding,
		RendererUtils::DEFAULT_BPP, RendererUtils::DEFAULT_PIXELFORMAT);

	constexpr Color backColor(32, 32, 32, 192);
	surface.fill(backColor.r, backColor.g, backColor.b, backColor.a);

	// Offset the text from the top left corner a bit so it isn't against the side of the tooltip
	// (for aesthetic purposes).
	const int dstX = padding / 2;
	const int dstY = padding / 2;

	constexpr Color textColor(255, 255, 255, 255);
	Span2D<uint32_t> surfacePixels = surface.getPixels();

	Buffer<std::string_view> textLines = TextRenderUtils::getTextLines(text);
	const Palette *palette = nullptr; // No tab colors supported in tooltips.
	constexpr TextAlignment alignment = TextAlignment::TopLeft;
	const TextRenderShadowInfo *shadow = nullptr;
	constexpr bool isCursorVisible = false;
	TextRenderUtils::drawTextLines(textLines, fontDef, dstX, dstY, textColor, palette, alignment, lineSpacing, shadow, isCursorVisible, surfacePixels);
	
	return surface;
}

Buffer<TextureAsset> TextureUtils::makeTextureAssets(const std::string &filename,
	TextureManager &textureManager)
{
	const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(filename.c_str());
	if (!metadataID.has_value())
	{
		DebugLogError("Couldn't get texture file metadata for \"" + filename + "\".");
		return Buffer<TextureAsset>();
	}

	const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);
	const int textureCount = textureFileMetadata.getTextureCount();
	Buffer<TextureAsset> textureAssets(textureCount);
	for (int i = 0; i < textureCount; i++)
	{
		TextureAsset textureAsset(textureFileMetadata.getFilename(), i);
		textureAssets.set(i, std::move(textureAsset));
	}

	return textureAssets;
}

bool TextureUtils::tryAllocUiTexture(const TextureAsset &textureAsset, const TextureAsset &paletteTextureAsset,
	TextureManager &textureManager, Renderer &renderer, UiTextureID *outID)
{
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteTextureAsset);
	if (!paletteID.has_value())
	{
		DebugLogErrorFormat("Couldn't get palette ID for \"%s\".", paletteTextureAsset.filename.c_str());
		return false;
	}

	const Palette &palette = textureManager.getPaletteHandle(*paletteID);
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAsset);
	if (!textureBuilderID.has_value())
	{
		DebugLogErrorFormat("Couldn't get texture builder ID for \"%s\".", textureAsset.filename.c_str());
		return false;
	}

	const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
	const UiTextureID textureID = renderer.createUiTexture(textureBuilder.width, textureBuilder.height);
	if (textureID < 0)
	{
		DebugLogErrorFormat("Couldn't create UI texture for \"%s\".", textureAsset.filename.c_str());
		return false;
	}

	if (!renderer.populateUiTexture(textureID, textureBuilder.bytes, &palette))
	{
		DebugLogErrorFormat("Couldn't populate UI texture for \"%s\".", textureAsset.filename.c_str());
	}

	*outID = textureID;
	return true;
}

bool TextureUtils::tryAllocUiTextureFromPixels(Span2D<const uint32_t> pixels, Renderer &renderer, UiTextureID *outID)
{
	const int width = pixels.getWidth();
	const int height = pixels.getHeight();

	const UiTextureID textureID = renderer.createUiTexture(width, height);
	if (textureID < 0)
	{
		DebugLogErrorFormat("Couldn't create %dx%d UI texture from pixels.", width, height);
		return false;
	}

	if (!renderer.populateUiTextureNoPalette(textureID, pixels))
	{
		DebugLogError("Couldn't populate UI texture from surface.");
	}

	*outID = textureID;
	return true;
}

bool TextureUtils::tryAllocUiTextureFromSurface(const Surface &surface, Renderer &renderer, UiTextureID *outID)
{
	return TextureUtils::tryAllocUiTextureFromPixels(surface.getPixels(), renderer, outID);
}
