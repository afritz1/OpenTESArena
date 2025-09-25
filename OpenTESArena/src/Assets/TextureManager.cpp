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
#include "../Assets/TXTFile.h"
#include "../Assets/TextureAsset.h"
#include "../Math/Vector2.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/RendererUtils.h"
#include "../UI/Surface.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"
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

bool TextureManager::tryLoadTextureData(const char *filename, Buffer<TextureBuilder> *outTextures, TextureFileMetadata *outMetadata)
{
	// Need at least one non-null out parameter.
	DebugAssert((outTextures != nullptr) || (outMetadata != nullptr));

	auto makeDimensions = [](int width, int height)
	{
		Buffer<Int2> buffer(1);
		buffer.set(0, Int2(width, height));
		return buffer;
	};

	auto makeOffset = [](int xOffset, int yOffset)
	{
		Buffer<Int2> buffer(1);
		buffer.set(0, Int2(xOffset, yOffset));
		return buffer;
	};

	if (TextureManager::matchesExtension(filename, EXTENSION_BMP))
	{
		Surface surface = Surface::loadBMP(filename, RendererUtils::DEFAULT_PIXELFORMAT);
		if (surface.get() == nullptr)
		{
			DebugLogWarning("Couldn't load .BMP file \"" + std::string(filename) + "\".");
			return false;
		}

		if (outTextures != nullptr)
		{
			TextureBuilder textureBuilder;
			textureBuilder.initTrueColor(surface.getWidth(), surface.getHeight(), surface.getPixels().begin());
			outTextures->init(1);
			outTextures->set(0, std::move(textureBuilder));
		}
		
		if (outMetadata != nullptr)
		{
			outMetadata->init(std::string(filename), makeDimensions(surface.getWidth(), surface.getHeight()));
		}
	}
	else if (TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_CFA))
	{
		CFAFile cfa;
		if (!cfa.init(filename))
		{
			DebugLogWarning("Couldn't init .CFA file \"" + std::string(filename) + "\".");
			return false;
		}

		if (outTextures != nullptr)
		{
			outTextures->init(cfa.getImageCount());
			for (int i = 0; i < cfa.getImageCount(); i++)
			{
				TextureBuilder textureBuilder;
				textureBuilder.initPaletted(cfa.getWidth(), cfa.getHeight(), cfa.getPixels(i));
				outTextures->set(i, std::move(textureBuilder));
			}
		}
		
		if (outMetadata != nullptr)
		{
			Buffer<Int2> dimensions(cfa.getImageCount());
			Buffer<Int2> offsets(cfa.getImageCount());
			for (int i = 0; i < cfa.getImageCount(); i++)
			{
				dimensions.set(i, Int2(cfa.getWidth(), cfa.getHeight()));
				offsets.set(i, Int2(cfa.getXOffset(), cfa.getYOffset()));
			}

			outMetadata->init(std::string(filename), std::move(dimensions), std::move(offsets));
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

		if (outTextures != nullptr)
		{
			outTextures->init(cif.getImageCount());
			for (int i = 0; i < cif.getImageCount(); i++)
			{
				TextureBuilder textureBuilder;
				textureBuilder.initPaletted(cif.getWidth(i), cif.getHeight(i), cif.getPixels(i));
				outTextures->set(i, std::move(textureBuilder));
			}
		}

		if (outMetadata != nullptr)
		{
			Buffer<Int2> dimensions(cif.getImageCount());
			Buffer<Int2> offsets(cif.getImageCount());
			for (int i = 0; i < cif.getImageCount(); i++)
			{
				dimensions.set(i, Int2(cif.getWidth(i), cif.getHeight(i)));
				offsets.set(i, Int2(cif.getXOffset(i), cif.getYOffset(i)));
			}

			outMetadata->init(std::string(filename), std::move(dimensions), std::move(offsets));
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

		if (outTextures != nullptr)
		{
			outTextures->init(dfa.getImageCount());
			for (int i = 0; i < dfa.getImageCount(); i++)
			{
				TextureBuilder textureBuilder;
				textureBuilder.initPaletted(dfa.getWidth(), dfa.getHeight(), dfa.getPixels(i));
				outTextures->set(i, std::move(textureBuilder));
			}
		}

		if (outMetadata != nullptr)
		{
			Buffer<Int2> dimensions(dfa.getImageCount());
			for (int i = 0; i < dfa.getImageCount(); i++)
			{
				dimensions.set(i, Int2(dfa.getWidth(), dfa.getHeight()));
			}

			outMetadata->init(std::string(filename), std::move(dimensions));
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

		if (outTextures != nullptr)
		{
			outTextures->init(flc.getFrameCount());
			for (int i = 0; i < flc.getFrameCount(); i++)
			{
				TextureBuilder textureBuilder;
				textureBuilder.initPaletted(flc.getWidth(), flc.getHeight(), flc.getPixels(i));
				outTextures->set(i, std::move(textureBuilder));
			}
		}

		if (outMetadata != nullptr)
		{
			Buffer<Int2> dimensions(flc.getFrameCount());
			for (int i = 0; i < flc.getFrameCount(); i++)
			{
				dimensions.set(i, Int2(flc.getWidth(), flc.getHeight()));
			}

			outMetadata->init(std::string(filename), std::move(dimensions), flc.getSecondsPerFrame());
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

		if (outTextures != nullptr)
		{
			TextureBuilder textureBuilder;
			textureBuilder.initPaletted(img.getWidth(), img.getHeight(), img.getPixels());
			outTextures->init(1);
			outTextures->set(0, std::move(textureBuilder));
		}

		if (outMetadata != nullptr)
		{
			outMetadata->init(std::string(filename), makeDimensions(img.getWidth(), img.getHeight()));
		}
	}
	else if (TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_LGT))
	{
		LGTFile lgt;
		if (!lgt.init(filename))
		{
			DebugLogWarning("Couldn't init .LGT file \"" + std::string(filename) + "\".");
			return false;
		}

		if (outTextures != nullptr)
		{
			Span2D<const uint8_t> lightPalettes = lgt.getAllLightPalettes();
			TextureBuilder textureBuilder;
			textureBuilder.initPaletted(lightPalettes.getWidth(), lightPalettes.getHeight(), lightPalettes.begin());
			outTextures->init(1);
			outTextures->set(0, std::move(textureBuilder));
		}

		if (outMetadata != nullptr)
		{
			outMetadata->init(std::string(filename), makeDimensions(LGTFile::ELEMENTS_PER_PALETTE, LGTFile::PALETTE_COUNT));
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

		if (outTextures != nullptr)
		{
			outTextures->init(rci.getImageCount());
			for (int i = 0; i < rci.getImageCount(); i++)
			{
				TextureBuilder textureBuilder;
				textureBuilder.initPaletted(RCIFile::WIDTH, RCIFile::HEIGHT, rci.getPixels(i));
				outTextures->set(i, std::move(textureBuilder));
			}
		}

		if (outMetadata != nullptr)
		{
			Buffer<Int2> dimensions(rci.getImageCount());
			for (int i = 0; i < rci.getImageCount(); i++)
			{
				dimensions.set(i, Int2(RCIFile::WIDTH, RCIFile::HEIGHT));
			}

			outMetadata->init(std::string(filename), std::move(dimensions));
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

		if (outTextures != nullptr)
		{
			outTextures->init(set.getImageCount());
			for (int i = 0; i < set.getImageCount(); i++)
			{
				TextureBuilder textureBuilder;
				textureBuilder.initPaletted(SETFile::CHUNK_WIDTH, SETFile::CHUNK_HEIGHT, set.getPixels(i));
				outTextures->set(i, std::move(textureBuilder));
			}
		}

		if (outMetadata != nullptr)
		{
			Buffer<Int2> dimensions(set.getImageCount());
			for (int i = 0; i < set.getImageCount(); i++)
			{
				dimensions.set(i, Int2(SETFile::CHUNK_WIDTH, SETFile::CHUNK_HEIGHT));
			}

			outMetadata->init(std::string(filename), std::move(dimensions));
		}
	}
	else if (TextureManager::matchesExtension(filename, ArenaAssetUtils::EXTENSION_TXT))
	{
		TXTFile txt;
		if (!txt.init(filename))
		{
			DebugLogWarning("Couldn't init .TXT file \"" + std::string(filename) + "\".");
			return false;
		}

		if (outTextures != nullptr)
		{
			TextureBuilder textureBuilder;
			textureBuilder.initHighColor(TXTFile::WIDTH, TXTFile::HEIGHT, txt.getPixels());
			outTextures->init(1);
			outTextures->set(0, std::move(textureBuilder));
		}
		
		if (outMetadata != nullptr)
		{
			outMetadata->init(std::string(filename), makeDimensions(TXTFile::WIDTH, TXTFile::HEIGHT));
		}
	}
	else
	{
		DebugLogWarning("Unrecognized texture builder file \"" + std::string(filename) + "\".");
		return false;
	}

	return true;
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

			for (Palette &palette : palettes)
			{
				this->palettes.emplace_back(std::move(palette));
			}

			iter = this->paletteIDs.emplace(std::move(paletteName), std::move(ids)).first;
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

std::optional<PaletteID> TextureManager::tryGetPaletteID(const TextureAsset &textureAsset)
{
	const std::optional<PaletteIdGroup> ids = this->tryGetPaletteIDs(textureAsset.filename.c_str());
	if (ids.has_value())
	{
		const int index = textureAsset.index.has_value() ? *textureAsset.index : 0;
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
		if (!TextureManager::tryLoadTextureData(filename, &textureBuilders, nullptr))
		{
			DebugLogWarning("Couldn't load texture builders from \"" + filenameStr + "\".");
			return std::nullopt;
		}

		const TextureBuilderID startID = static_cast<TextureBuilderID>(this->textureBuilders.size());
		TextureBuilderIdGroup ids(startID, textureBuilders.getCount());

		for (TextureBuilder &textureBuilder : textureBuilders)
		{
			this->textureBuilders.emplace_back(std::move(textureBuilder));
		}

		iter = this->textureBuilderIDs.emplace(std::move(filenameStr), std::move(ids)).first;
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

std::optional<PaletteID> TextureManager::tryGetTextureBuilderID(const TextureAsset &textureAsset)
{
	const std::optional<TextureBuilderIdGroup> ids = this->tryGetTextureBuilderIDs(textureAsset.filename.c_str());
	if (ids.has_value())
	{
		const int index = textureAsset.index.has_value() ? *textureAsset.index : 0;
		return ids->getID(index);
	}
	else
	{
		return std::nullopt;
	}
}

std::optional<TextureFileMetadataID> TextureManager::tryGetMetadataID(const char *filename)
{
	if (String::isNullOrEmpty(filename))
	{
		DebugLogWarning("Missing texture file metadata filename.");
		return std::nullopt;
	}

	std::string filenameStr(filename);
	auto iter = this->metadataIndices.find(filenameStr);
	if (iter == this->metadataIndices.end())
	{
		TextureFileMetadata metadata;
		if (!TextureManager::tryLoadTextureData(filename, nullptr, &metadata))
		{
			DebugLogWarning("Couldn't load texture file metadata from \"" + filenameStr + "\".");
			return std::nullopt;
		}

		const TextureFileMetadataID id = static_cast<TextureFileMetadataID>(this->metadatas.size());
		this->metadatas.emplace_back(std::move(metadata));

		iter = this->metadataIndices.emplace(std::move(filenameStr), id).first;
	}

	return static_cast<TextureFileMetadataID>(iter->second);
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

const TextureFileMetadata &TextureManager::getMetadataHandle(TextureFileMetadataID id) const
{
	DebugAssertIndex(this->metadatas, id);
	return this->metadatas[id];
}

int TextureManager::getTotalBytes() const
{
	int byteCount = 0;
	for (const Palette &palette : this->palettes)
	{
		byteCount += sizeof(Palette);
	}

	for (const TextureBuilder &textureBuilder : this->textureBuilders)
	{
		byteCount += textureBuilder.bytes.getCount();
	}

	return byteCount;
}
