#include "SDL.h"

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
#include "../Assets/TextureAssetReference.h"
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

std::optional<TextureFileMetadata> TextureManager::tryGetMetadata(const char *filename)
{
	if (String::isNullOrEmpty(filename))
	{
		DebugLogWarning("Missing filename for texture file metadata.");
		return std::nullopt;
	}

	// @todo: this function should ideally do the bare minimum decompression work to extract the metadata
	// and never actually load texel data into memory. I imagine it would have per-file-format branches
	// and query each one in a similar way to .IMG file palette extraction.

	const std::optional<TextureBuilderIdGroup> ids = this->tryGetTextureBuilderIDs(filename);
	if (ids.has_value())
	{
		Buffer<std::pair<int, int>> dimensions(ids->getCount());
		for (int i = 0; i < ids->getCount(); i++)
		{
			const TextureBuilderID id = ids->getID(i);
			const TextureBuilder &textureBuilder = this->getTextureBuilderHandle(id);
			dimensions.set(i, std::make_pair(textureBuilder.getWidth(), textureBuilder.getHeight()));
		}

		return TextureFileMetadata(std::string(filename), std::move(dimensions));
	}
	else
	{
		return std::nullopt;
	}
}

std::optional<PaletteIdGroup> TextureManager::tryGetPaletteIDs(const char *filename)
{
	if (String::isNullOrEmpty(filename))
	{
		DebugLogWarning("Missing palette filename.");
		return std::nullopt;
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
			return std::nullopt;
		}
	}

	return iter->second;
}

std::optional<PaletteID> TextureManager::tryGetPaletteID(const char *filename)
{
	const std::optional<PaletteIdGroup> ids = this->tryGetPaletteIDs(filename);
	if (ids.has_value())
	{
		return ids->getID(0);
	}
	else
	{
		return std::nullopt;
	}
}

std::optional<PaletteID> TextureManager::tryGetPaletteID(const TextureAssetReference &textureAssetRef)
{
	const std::optional<PaletteIdGroup> ids = this->tryGetPaletteIDs(textureAssetRef.filename.c_str());
	if (ids.has_value())
	{
		const int index = textureAssetRef.index.has_value() ? *textureAssetRef.index : 0;
		return ids->getID(index);
	}
	else
	{
		return std::nullopt;
	}
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

std::optional<TextureBuilderID> TextureManager::tryGetTextureBuilderID(const char *filename)
{
	const std::optional<TextureBuilderIdGroup> ids = this->tryGetTextureBuilderIDs(filename);
	if (ids.has_value())
	{
		return ids->getID(0);
	}
	else
	{
		return std::nullopt;
	}
}

std::optional<PaletteID> TextureManager::tryGetTextureBuilderID(const TextureAssetReference &textureAssetRef)
{
	const std::optional<TextureBuilderIdGroup> ids = this->tryGetTextureBuilderIDs(textureAssetRef.filename.c_str());
	if (ids.has_value())
	{
		const int index = textureAssetRef.index.has_value() ? *textureAssetRef.index : 0;
		return ids->getID(index);
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

TextureBuilderRef TextureManager::getTextureBuilderRef(TextureBuilderID id) const
{
	return TextureBuilderRef(&this->textureBuilders, static_cast<int>(id));
}

const Palette &TextureManager::getPaletteHandle(PaletteID id) const
{
	DebugAssertIndex(this->palettes, id);
	return this->palettes[id];
}

const TextureBuilder &TextureManager::getTextureBuilderHandle(TextureBuilderID id) const
{
	DebugAssertIndex(this->textureBuilders, id);
	return this->textureBuilders[id];
}
