#include <algorithm>

#include "SDL.h"

#include "PaletteFile.h"
#include "PaletteName.h"
#include "PaletteUtils.h"
#include "TextureManager.h"
#include "../Assets/ArenaAssetUtils.h"
#include "../Assets/CFAFile.h"
#include "../Assets/CIFFile.h"
#include "../Assets/COLFile.h"
#include "../Assets/Compression.h"
#include "../Assets/DFAFile.h"
#include "../Assets/FLCFile.h"
#include "../Assets/IMGFile.h"
#include "../Assets/LGTFile.h"
#include "../Assets/RCIFile.h"
#include "../Assets/SETFile.h"
#include "../Math/Vector2.h"
#include "../Rendering/Renderer.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"
#include "components/utilities/StringView.h"

namespace
{
	// Texture filename extensions.
	constexpr const char *EXTENSION_BMP = "BMP";
}

bool TextureManager::matchesExtension(const char *filename, const char *extension)
{
	return StringView::caseInsensitiveEquals(StringView::getExtension(filename), extension);
}

std::string TextureManager::makeTextureMappingName(const char *filename,
	const std::optional<PaletteID> &paletteID)
{
	const std::string textureName(filename);
	return paletteID.has_value() ? (textureName + std::to_string(*paletteID)) : textureName;
}

bool TextureManager::tryLoadPalettes(const char *filename, Buffer<Palette> *outPalettes)
{
	if (TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_COL))
	{
		COLFile col;
		if (!col.init(filename))
		{
			DebugLogWarning("Couldn't init .COL file \"" + std::string(filename) + "\".");
			return false;
		}

		outPalettes->init(1);
		outPalettes->set(0, col.getPalette());
	}
	else if (TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_CEL) ||
		TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_FLC))
	{
		FLCFile flc;
		if (!flc.init(filename))
		{
			DebugLogWarning("Couldn't init .FLC/.CEL file \"" + std::string(filename) + "\".");
			return false;
		}

		outPalettes->init(flc.getFrameCount());
		for (int i = 0; i < flc.getFrameCount(); i++)
		{
			outPalettes->set(i, flc.getFramePalette(i));
		}
	}
	else if (TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_IMG) ||
		TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_MNU))
	{
		Palette palette;
		if (!IMGFile::tryExtractPalette(filename, palette))
		{
			DebugLogWarning("Couldn't extract .IMG palette from \"" + std::string(filename) + "\".");
			return false;
		}

		outPalettes->init(1);
		outPalettes->set(0, palette);
	}
	else
	{
		DebugLogWarning("Unrecognized palette file \"" + std::string(filename) + "\".");
		return false;
	}

	return true;
}

bool TextureManager::tryLoadImages(const char *filename, const std::optional<PaletteID> &paletteID,
	Buffer<Image> *outImages)
{
	auto makeImage = [paletteID](int width, int height, const uint8_t *srcPixels)
	{
		Image image;
		image.init(width, height, paletteID);

		const int pixelCount = width * height;
		std::copy(srcPixels, srcPixels + pixelCount, image.getPixels());

		return image;
	};

	if (TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_CFA))
	{
		CFAFile cfa;
		if (!cfa.init(filename))
		{
			DebugLogWarning("Couldn't init .CFA file \"" + std::string(filename) + "\".");
			return false;
		}

		outImages->init(cfa.getImageCount());
		for (int i = 0; i < cfa.getImageCount(); i++)
		{
			Image image = makeImage(cfa.getWidth(), cfa.getHeight(), cfa.getPixels(i));
			outImages->set(i, std::move(image));
		}
	}
	else if (TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_CIF))
	{
		CIFFile cif;
		if (!cif.init(filename))
		{
			DebugLogWarning("Couldn't init .CIF file \"" + std::string(filename) + "\".");
			return false;
		}

		outImages->init(cif.getImageCount());
		for (int i = 0; i < cif.getImageCount(); i++)
		{
			Image image = makeImage(cif.getWidth(i), cif.getHeight(i), cif.getPixels(i));
			outImages->set(i, std::move(image));
		}
	}
	else if (TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_DFA))
	{
		DFAFile dfa;
		if (!dfa.init(filename))
		{
			DebugLogWarning("Couldn't init .DFA file \"" + std::string(filename) + "\".");
			return false;
		}

		outImages->init(dfa.getImageCount());
		for (int i = 0; i < dfa.getImageCount(); i++)
		{
			Image image = makeImage(dfa.getWidth(), dfa.getHeight(), dfa.getPixels(i));
			outImages->set(i, std::move(image));
		}
	}
	else if (TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_FLC) ||
		TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_CEL))
	{
		FLCFile flc;
		if (!flc.init(filename))
		{
			DebugLogWarning("Couldn't init .FLC/.CEL file \"" + std::string(filename) + "\".");
			return false;
		}

		outImages->init(flc.getFrameCount());
		for (int i = 0; i < flc.getFrameCount(); i++)
		{
			Image image = makeImage(flc.getWidth(), flc.getHeight(), flc.getPixels(i));
			outImages->set(i, std::move(image));
		}
	}
	else if (TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_IMG) ||
		TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_MNU))
	{
		IMGFile img;
		if (!img.init(filename))
		{
			DebugLogWarning("Couldn't init .IMG/.MNU file \"" + std::string(filename) + "\".");
			return false;
		}

		Image image = makeImage(img.getWidth(), img.getHeight(), img.getPixels());
		outImages->init(1);
		outImages->set(0, std::move(image));
	}
	else if (TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_LGT))
	{
		LGTFile lgt;
		if (!lgt.init(filename))
		{
			DebugLogWarning("Couldn't init .LGT file \"" + std::string(filename) + "\".");
			return false;
		}

		outImages->init(LGTFile::PALETTE_COUNT);
		for (int i = 0; i < outImages->getCount(); i++)
		{
			const BufferView<const uint8_t> lightPalette = lgt.getLightPalette(i);
			Image image = makeImage(lightPalette.getCount(), 1, lightPalette.get());
			outImages->set(i, std::move(image));
		}
	}
	else if (TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_RCI))
	{
		RCIFile rci;
		if (!rci.init(filename))
		{
			DebugLogWarning("Couldn't init .RCI file \"" + std::string(filename) + "\".");
			return false;
		}

		outImages->init(rci.getImageCount());
		for (int i = 0; i < rci.getImageCount(); i++)
		{
			Image image = makeImage(RCIFile::WIDTH, RCIFile::HEIGHT, rci.getPixels(i));
			outImages->set(i, std::move(image));
		}
	}
	else if (TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_SET))
	{
		SETFile set;
		if (!set.init(filename))
		{
			DebugLogWarning("Couldn't init .SET file \"" + std::string(filename) + "\".");
			return false;
		}

		outImages->init(set.getImageCount());
		for (int i = 0; i < set.getImageCount(); i++)
		{
			Image image = makeImage(SETFile::CHUNK_WIDTH, SETFile::CHUNK_HEIGHT, set.getPixels(i));
			outImages->set(i, std::move(image));
		}
	}
	else
	{
		DebugLogWarning("Unrecognized image file \"" + std::string(filename) + "\".");
		return false;
	}

	return true;
}

bool TextureManager::tryLoadSurfaces(const char *filename, const Palette &palette,
	Buffer<Surface> *outSurfaces)
{
	// Reuse image loading code for convenience.
	// @todo: presumably could put some 32-bit-only loading here, like .BMP, but the palette
	// would need to be nullable then.
	Buffer<Image> images;
	if (!TextureManager::tryLoadImages(filename, std::nullopt, &images))
	{
		return false;
	}

	outSurfaces->init(images.getCount());
	for (int i = 0; i < images.getCount(); i++)
	{
		const Image &image = images.get(i);
		Surface surface = TextureUtils::makeSurfaceFrom8Bit(
			image.getWidth(), image.getHeight(), image.getPixels(), palette);
		outSurfaces->set(i, std::move(surface));
	}

	return true;
}

bool TextureManager::tryLoadTextures(const char *filename, const Palette &palette,
	Renderer &renderer, Buffer<Texture> *outTextures)
{
	// Reuse image loading code for convenience.
	// @todo: presumably could put some 32-bit-only loading here, like .BMP, but the palette
	// would need to be nullable then.
	Buffer<Image> images;
	if (!TextureManager::tryLoadImages(filename, std::nullopt, &images))
	{
		return false;
	}

	outTextures->init(images.getCount());
	for (int i = 0; i < images.getCount(); i++)
	{
		const Image &image = images.get(i);
		Texture texture = TextureUtils::makeTextureFrom8Bit(
			image.getWidth(), image.getHeight(), image.getPixels(), palette, renderer);
		outTextures->set(i, std::move(texture));
	}

	return true;
}

bool TextureManager::tryLoadTextureBuilders(const char *filename, Buffer<TextureBuilder> *outTextures)
{
	auto makePaletted = [](int width, int height, const uint8_t *texels)
	{
		TextureBuilder textureBuilder;
		textureBuilder.initPaletted(width, height, texels);
		return textureBuilder;
	};

	auto makeTrueColor = [](int width, int height, const uint32_t *texels)
	{
		TextureBuilder textureBuilder;
		textureBuilder.initTrueColor(width, height, texels);
		return textureBuilder;
	};

	if (TextureManager::matchesExtension(filename, EXTENSION_BMP))
	{
		SDL_Surface *surface = SDL_LoadBMP(filename);
		if (surface == nullptr)
		{
			DebugLogWarning("Couldn't load .BMP file \"" + std::string(filename) + "\".");
			return false;
		}

		SDL_Surface *optimizedSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ARGB8888, 0);
		SDL_FreeSurface(surface);
		if (optimizedSurface == nullptr)
		{
			DebugLogWarning("Couldn't optimize .BMP file \"" + std::string(filename) + "\".");
			return false;
		}

		TextureBuilder textureBuilder = makeTrueColor(optimizedSurface->w, optimizedSurface->h,
			static_cast<const uint32_t*>(optimizedSurface->pixels));
		SDL_FreeSurface(optimizedSurface);

		outTextures->init(1);
		outTextures->set(0, std::move(textureBuilder));
	}
	else if (TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_CFA))
	{
		CFAFile cfa;
		if (!cfa.init(filename))
		{
			DebugLogWarning("Couldn't init .CFA file \"" + std::string(filename) + "\".");
			return false;
		}

		outTextures->init(cfa.getImageCount());
		for (int i = 0; i < cfa.getImageCount(); i++)
		{
			TextureBuilder textureBuilder = makePaletted(cfa.getWidth(), cfa.getHeight(), cfa.getPixels(i));
			outTextures->set(i, std::move(textureBuilder));
		}
	}
	else if (TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_CIF))
	{
		CIFFile cif;
		if (!cif.init(filename))
		{
			DebugLogWarning("Couldn't init .CIF file \"" + std::string(filename) + "\".");
			return false;
		}

		outTextures->init(cif.getImageCount());
		for (int i = 0; i < cif.getImageCount(); i++)
		{
			TextureBuilder textureBuilder = makePaletted(cif.getWidth(i), cif.getHeight(i), cif.getPixels(i));
			outTextures->set(i, std::move(textureBuilder));
		}
	}
	else if (TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_DFA))
	{
		DFAFile dfa;
		if (!dfa.init(filename))
		{
			DebugLogWarning("Couldn't init .DFA file \"" + std::string(filename) + "\".");
			return false;
		}

		outTextures->init(dfa.getImageCount());
		for (int i = 0; i < dfa.getImageCount(); i++)
		{
			TextureBuilder textureBuilder = makePaletted(dfa.getWidth(), dfa.getHeight(), dfa.getPixels(i));
			outTextures->set(i, std::move(textureBuilder));
		}
	}
	else if (TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_FLC) ||
		TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_CEL))
	{
		FLCFile flc;
		if (!flc.init(filename))
		{
			DebugLogWarning("Couldn't init .FLC/.CEL file \"" + std::string(filename) + "\".");
			return false;
		}

		outTextures->init(flc.getFrameCount());
		for (int i = 0; i < flc.getFrameCount(); i++)
		{
			TextureBuilder textureBuilder = makePaletted(flc.getWidth(), flc.getHeight(), flc.getPixels(i));
			outTextures->set(i, std::move(textureBuilder));
		}
	}
	else if (TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_IMG) ||
		TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_MNU))
	{
		IMGFile img;
		if (!img.init(filename))
		{
			DebugLogWarning("Couldn't init .IMG/.MNU file \"" + std::string(filename) + "\".");
			return false;
		}

		TextureBuilder textureBuilder = makePaletted(img.getWidth(), img.getHeight(), img.getPixels());
		outTextures->init(1);
		outTextures->set(0, std::move(textureBuilder));
	}
	else if (TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_LGT))
	{
		LGTFile lgt;
		if (!lgt.init(filename))
		{
			DebugLogWarning("Couldn't init .LGT file \"" + std::string(filename) + "\".");
			return false;
		}

		outTextures->init(LGTFile::PALETTE_COUNT);
		for (int i = 0; i < outTextures->getCount(); i++)
		{
			const BufferView<const uint8_t> lightPalette = lgt.getLightPalette(i);
			TextureBuilder textureBuilder = makePaletted(lightPalette.getCount(), 1, lightPalette.get());
			outTextures->set(i, std::move(textureBuilder));
		}
	}
	else if (TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_RCI))
	{
		RCIFile rci;
		if (!rci.init(filename))
		{
			DebugLogWarning("Couldn't init .RCI file \"" + std::string(filename) + "\".");
			return false;
		}

		outTextures->init(rci.getImageCount());
		for (int i = 0; i < rci.getImageCount(); i++)
		{
			TextureBuilder textureBuilder = makePaletted(RCIFile::WIDTH, RCIFile::HEIGHT, rci.getPixels(i));
			outTextures->set(i, std::move(textureBuilder));
		}
	}
	else if (TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_SET))
	{
		SETFile set;
		if (!set.init(filename))
		{
			DebugLogWarning("Couldn't init .SET file \"" + std::string(filename) + "\".");
			return false;
		}

		outTextures->init(set.getImageCount());
		for (int i = 0; i < set.getImageCount(); i++)
		{
			TextureBuilder textureBuilder = makePaletted(SETFile::CHUNK_WIDTH, SETFile::CHUNK_HEIGHT, set.getPixels(i));
			outTextures->set(i, std::move(textureBuilder));
		}
	}
	else
	{
		DebugLogWarning("Unrecognized texture builder file \"" + std::string(filename) + "\".");
		return false;
	}

	return true;
}

bool TextureManager::tryGetPaletteIDs(const char *filename, PaletteIdGroup *outIDs)
{
	if (String::isNullOrEmpty(filename))
	{
		DebugLogWarning("Missing palette filename.");
		return false;
	}

	std::string paletteName(filename);
	auto iter = this->paletteIDs.find(paletteName);
	if (iter == this->paletteIDs.end())
	{
		// Load palette(s) from file.
		Buffer<Palette> palettes;
		if (TextureManager::tryLoadPalettes(filename, &palettes))
		{
			const PaletteID id = static_cast<PaletteID>(this->palettes.size());
			PaletteIdGroup ids(id, 1);

			for (int i = 0; i < palettes.getCount(); i++)
			{
				this->palettes.emplace_back(std::move(palettes.get(i)));
			}

			iter = this->paletteIDs.emplace(
				std::make_pair(std::move(paletteName), std::move(ids))).first;
		}
		else
		{
			DebugLogWarning("Couldn't load palette file \"" + paletteName + "\".");
			return false;
		}
	}

	*outIDs = iter->second;
	return true;
}

bool TextureManager::tryGetImageIDs(const char *filename, const std::optional<PaletteID> &paletteID,
	TextureUtils::ImageIdGroup *outIDs)
{
	if (String::isNullOrEmpty(filename))
	{
		DebugLogWarning("Missing image filename.");
		return false;
	}

	std::string mappingName = TextureManager::makeTextureMappingName(filename, paletteID);
	auto iter = this->imageIDs.find(mappingName);
	if (iter == this->imageIDs.end())
	{
		// Load image(s) from file.
		Buffer<Image> images;
		if (TextureManager::tryLoadImages(filename, paletteID, &images))
		{
			const ImageID startID = static_cast<ImageID>(this->images.size());
			TextureUtils::ImageIdGroup ids(startID, images.getCount());

			for (int i = 0; i < images.getCount(); i++)
			{
				this->images.push_back(std::move(images.get(i)));
			}

			iter = this->imageIDs.emplace(
				std::make_pair(std::move(mappingName), std::move(ids))).first;
		}
		else
		{
			DebugLogWarning("Couldn't load image file \"" + std::string(filename) + "\".");
			return false;
		}
	}

	*outIDs = iter->second;
	return true;
}

bool TextureManager::tryGetImageIDs(const char *filename, TextureUtils::ImageIdGroup *outIDs)
{
	constexpr std::optional<PaletteID> paletteID;
	return this->tryGetImageIDs(filename, paletteID, outIDs);
}

bool TextureManager::tryGetSurfaceIDs(const char *filename, PaletteID paletteID,
	TextureUtils::SurfaceIdGroup *outIDs)
{
	if (String::isNullOrEmpty(filename))
	{
		DebugLogWarning("Missing surface filename.");
		return false;
	}

	std::string mappingName = TextureManager::makeTextureMappingName(filename, paletteID);
	auto iter = this->surfaceIDs.find(mappingName);
	if (iter == this->surfaceIDs.end())
	{
		PaletteRef palette = this->getPaletteRef(paletteID);

		// Load surface(s) from file.
		Buffer<Surface> surfaces;
		if (TextureManager::tryLoadSurfaces(filename, palette.get(), &surfaces))
		{
			const SurfaceID startID = static_cast<SurfaceID>(this->surfaces.size());
			TextureUtils::SurfaceIdGroup ids(startID, surfaces.getCount());

			for (int i = 0; i < surfaces.getCount(); i++)
			{
				this->surfaces.push_back(std::move(surfaces.get(i)));
			}

			iter = this->surfaceIDs.emplace(
				std::make_pair(std::move(mappingName), std::move(ids))).first;
		}
		else
		{
			DebugLogWarning("Couldn't load surface file \"" + std::string(filename) + "\".");
			return false;
		}
	}

	*outIDs = iter->second;
	return true;
}

bool TextureManager::tryGetTextureIDs(const char *filename, PaletteID paletteID,
	Renderer &renderer, TextureUtils::TextureIdGroup *outIDs)
{
	if (String::isNullOrEmpty(filename))
	{
		DebugLogWarning("Missing texture filename.");
		return false;
	}

	std::string mappingName = TextureManager::makeTextureMappingName(filename, paletteID);
	auto iter = this->textureIDs.find(mappingName);
	if (iter == this->textureIDs.end())
	{
		PaletteRef palette = this->getPaletteRef(paletteID);

		// Load texture(s) from file.
		Buffer<Texture> textures;
		if (TextureManager::tryLoadTextures(filename, palette.get(), renderer, &textures))
		{
			const TextureID startID = static_cast<TextureID>(this->textures.size());
			TextureUtils::TextureIdGroup ids(startID, textures.getCount());

			for (int i = 0; i < textures.getCount(); i++)
			{
				this->textures.push_back(std::move(textures.get(i)));
			}

			iter = this->textureIDs.emplace(
				std::make_pair(std::move(mappingName), std::move(ids))).first;
		}
		else
		{
			DebugLogWarning("Couldn't load texture file \"" + std::string(filename) + "\".");
			return false;
		}
	}

	*outIDs = iter->second;
	return true;
}

std::optional<TextureBuilderIdGroup> TextureManager::tryGetTextureBuilderIDs(const char *filename)
{
	if (String::isNullOrEmpty(filename))
	{
		DebugLogWarning("Missing texture builder filename.");
		return std::nullopt;
	}

	std::string filenameStr(filename);
	auto iter = this->textureBuilderIDs.find(filenameStr);
	if (iter == this->textureBuilderIDs.end())
	{
		Buffer<TextureBuilder> textureBuilders;
		if (!TextureManager::tryLoadTextureBuilders(filename, &textureBuilders))
		{
			DebugLogWarning("Couldn't load texture builders from \"" + filenameStr + "\".");
			return std::nullopt;
		}

		const TextureBuilderID startID = static_cast<TextureBuilderID>(this->textureBuilders.size());
		TextureBuilderIdGroup ids(startID, textureBuilders.getCount());

		for (int i = 0; i < textureBuilders.getCount(); i++)
		{
			this->textureBuilders.emplace_back(std::move(textureBuilders.get(i)));
		}

		iter = this->textureBuilderIDs.emplace(
			std::make_pair(std::move(filenameStr), std::move(ids))).first;
	}

	return iter->second;
}

bool TextureManager::tryGetPaletteID(const char *filename, PaletteID *outID)
{
	PaletteIdGroup ids;
	if (this->tryGetPaletteIDs(filename, &ids))
	{
		*outID = ids.getID(0);
		return true;
	}
	else
	{
		return false;
	}
}

bool TextureManager::tryGetImageID(const char *filename, const std::optional<PaletteID> &paletteID, ImageID *outID)
{
	TextureUtils::ImageIdGroup ids;
	if (this->tryGetImageIDs(filename, paletteID, &ids))
	{
		*outID = ids.getID(0);
		return true;
	}
	else
	{
		return false;
	}
}

bool TextureManager::tryGetImageID(const char *filename, ImageID *outID)
{
	constexpr std::optional<PaletteID> paletteID;
	return this->tryGetImageID(filename, paletteID, outID);
}

bool TextureManager::tryGetSurfaceID(const char *filename, PaletteID paletteID, SurfaceID *outID)
{
	TextureUtils::SurfaceIdGroup ids;
	if (this->tryGetSurfaceIDs(filename, paletteID, &ids))
	{
		*outID = ids.getID(0);
		return true;
	}
	else
	{
		return false;
	}
}

bool TextureManager::tryGetTextureID(const char *filename, PaletteID paletteID,
	Renderer &renderer, TextureID *outID)
{
	TextureUtils::TextureIdGroup ids;
	if (this->tryGetTextureIDs(filename, paletteID, renderer, &ids))
	{
		*outID = ids.getID(0);
		return true;
	}
	else
	{
		return false;
	}
}

std::optional<TextureBuilderID> TextureManager::tryGetTextureBuilderID(const char *filename)
{
	const std::optional<TextureBuilderIdGroup> idGroup = this->tryGetTextureBuilderIDs(filename);
	if (idGroup.has_value())
	{
		return idGroup->getID(0);
	}
	else
	{
		return std::nullopt;
	}
}

PaletteRef TextureManager::getPaletteRef(PaletteID id) const
{
	return PaletteRef(&this->palettes, static_cast<int>(id));
}

ImageRef TextureManager::getImageRef(ImageID id) const
{
	return ImageRef(&this->images, static_cast<int>(id));
}

SurfaceRef TextureManager::getSurfaceRef(SurfaceID id) const
{
	return SurfaceRef(&this->surfaces, static_cast<int>(id));
}

TextureRef TextureManager::getTextureRef(TextureID id) const
{
	return TextureRef(&this->textures, static_cast<int>(id));
}

TextureBuilderRef TextureManager::getTextureBuilderRef(TextureBuilderID id) const
{
	return TextureBuilderRef(&this->textureBuilders, static_cast<int>(id));
}

const Palette &TextureManager::getPaletteHandle(PaletteID id) const
{
	DebugAssertIndex(this->palettes, id);
	return this->palettes[id];
}

const Image &TextureManager::getImageHandle(ImageID id) const
{
	DebugAssertIndex(this->images, id);
	return this->images[id];
}

const Surface &TextureManager::getSurfaceHandle(SurfaceID id) const
{
	DebugAssertIndex(this->surfaces, id);
	return this->surfaces[id];
}

const Texture &TextureManager::getTextureHandle(TextureID id) const
{
	DebugAssertIndex(this->textures, id);
	return this->textures[id];
}

const TextureBuilder &TextureManager::getTextureBuilderHandle(TextureBuilderID id) const
{
	DebugAssertIndex(this->textureBuilders, id);
	return this->textureBuilders[id];
}
