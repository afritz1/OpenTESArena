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

bool TextureManager::tryLoadTextureData(const char *filename, Buffer<TextureBuilder> *outTextures, TextureMetadata *outMetadata)
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
			outMetadata->init(std::string(filename), makeDimensions(img.getWidth(), img.getHeight()), makeOffset(img.getXOffset(), img.getYOffset()));
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

PaletteIdGroup TextureManager::getPaletteIDs(const char *filename)
{
	if (String::isNullOrEmpty(filename))
	{
		DebugLogWarning("Missing palette filename.");
		return PaletteIdGroup();
	}

	std::string paletteName(filename);
	auto iter = this->paletteIDs.find(paletteName);
	if (iter == this->paletteIDs.end())
	{
		Buffer<Palette> loadedPalettes;
		if (TextureManager::tryLoadPalettes(filename, &loadedPalettes))
		{
			const PaletteID id = static_cast<PaletteID>(this->palettes.size());
			PaletteIdGroup ids(id, 1);

			for (Palette &palette : loadedPalettes)
			{
				this->palettes.emplace_back(std::move(palette));
			}

			iter = this->paletteIDs.emplace(std::move(paletteName), std::move(ids)).first;
		}
		else
		{
			DebugLogWarningFormat("Couldn't load palette file \"%s\".", paletteName.c_str());
			return PaletteIdGroup();
		}
	}

	return iter->second;
}

PaletteID TextureManager::getPaletteID(const char *filename)
{
	const PaletteIdGroup ids = this->getPaletteIDs(filename);
	if (!ids.isValid())
	{
		return -1;
	}

	return ids.getID(0);
}

PaletteID TextureManager::getPaletteID(const TextureAsset &textureAsset)
{
	const PaletteIdGroup ids = this->getPaletteIDs(textureAsset.filename.c_str());
	if (!ids.isValid())
	{
		return -1;
	}

	const int index = (textureAsset.index >= 0) ? textureAsset.index : 0;
	return ids.getID(index);
}

TextureBuilderIdGroup TextureManager::getTextureBuilderIDs(const char *filename)
{
	if (String::isNullOrEmpty(filename))
	{
		DebugLogWarning("Missing texture builder filename.");
		return TextureBuilderIdGroup();
	}

	std::string filenameStr(filename);
	auto iter = this->textureBuilderIDs.find(filenameStr);
	if (iter == this->textureBuilderIDs.end())
	{
		Buffer<TextureBuilder> loadedTextureBuilders;
		if (!TextureManager::tryLoadTextureData(filename, &loadedTextureBuilders, nullptr))
		{
			DebugLogWarningFormat("Couldn't load texture builders from \"%s\".", filename);
			return TextureBuilderIdGroup();
		}

		const TextureBuilderID startID = static_cast<TextureBuilderID>(this->textureBuilders.size());
		TextureBuilderIdGroup ids(startID, loadedTextureBuilders.getCount());

		for (TextureBuilder &textureBuilder : loadedTextureBuilders)
		{
			this->textureBuilders.emplace_back(std::move(textureBuilder));
		}

		iter = this->textureBuilderIDs.emplace(std::move(filenameStr), std::move(ids)).first;
	}

	return iter->second;
}

TextureBuilderID TextureManager::getTextureBuilderID(const char *filename)
{
	const TextureBuilderIdGroup ids = this->getTextureBuilderIDs(filename);
	if (!ids.isValid())
	{
		return -1;
	}

	return ids.getID(0);
}

TextureBuilderID TextureManager::getTextureBuilderID(const TextureAsset &textureAsset)
{
	const TextureBuilderIdGroup ids = this->getTextureBuilderIDs(textureAsset.filename.c_str());
	if (!ids.isValid())
	{
		return -1;
	}

	const int index = (textureAsset.index >= 0) ? textureAsset.index : 0;
	return ids.getID(index);
}

TextureMetadataID TextureManager::getMetadataID(const char *filename)
{
	if (String::isNullOrEmpty(filename))
	{
		DebugLogWarning("Missing texture file metadata filename.");
		return -1;
	}

	std::string filenameStr(filename);
	auto iter = this->metadataIndices.find(filenameStr);
	if (iter == this->metadataIndices.end())
	{
		TextureMetadata loadedMetadata;
		if (!TextureManager::tryLoadTextureData(filename, nullptr, &loadedMetadata))
		{
			DebugLogWarningFormat("Couldn't load texture file metadata from \"%s\".", filename);
			return -1;
		}

		const TextureMetadataID id = static_cast<TextureMetadataID>(this->metadatas.size());
		this->metadatas.emplace_back(std::move(loadedMetadata));

		iter = this->metadataIndices.emplace(std::move(filenameStr), id).first;
	}

	return static_cast<TextureMetadataID>(iter->second);
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

const TextureMetadata &TextureManager::getMetadataHandle(TextureMetadataID id) const
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
