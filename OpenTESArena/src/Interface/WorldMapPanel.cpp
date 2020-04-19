#include "SDL.h"

#include "CursorAlignment.h"
#include "GameWorldPanel.h"
#include "ProvinceMapPanel.h"
#include "TextBox.h"
#include "WorldMapPanel.h"
#include "../Assets/CIFFile.h"
#include "../Assets/MiscAssets.h"
#include "../Assets/WorldMapMask.h"
#include "../Game/Game.h"
#include "../Game/GameData.h"
#include "../Game/Options.h"
#include "../Math/Rect.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Texture.h"

#include "components/debug/Debug.h"

WorldMapPanel::WorldMapPanel(Game &game, std::unique_ptr<ProvinceMapPanel::TravelData> travelData)
	: Panel(game), travelData(std::move(travelData))
{
	this->backToGameButton = []()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH - 22, Renderer::ORIGINAL_HEIGHT - 7);
		int width = 36;
		int height = 9;
		auto function = [](Game &game)
		{
			game.setPanel<GameWorldPanel>(game);
		};
		return Button<Game&>(center, width, height, function);
	}();

	this->provinceButton = []()
	{
		auto function = [](Game &game, int provinceID,
			std::unique_ptr<ProvinceMapPanel::TravelData> travelData)
		{
			game.setPanel<ProvinceMapPanel>(game, provinceID, std::move(travelData));
		};
		return Button<Game&, int, std::unique_ptr<ProvinceMapPanel::TravelData>>(function);
	}();

	// Load province name offsets.
	const std::string cifName = "OUTPROV.CIF";
	CIFFile cif;
	if (!cif.init(cifName.c_str()))
	{
		DebugCrash("Could not init .CIF file \"" + cifName + "\".");
	}

	for (int i = 0; i < static_cast<int>(this->provinceNameOffsets.size()); i++)
	{
		this->provinceNameOffsets.at(i) = Int2(cif.getXOffset(i), cif.getYOffset(i));
	}
}

Panel::CursorData WorldMapPanel::getCurrentCursor() const
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &textureManager = game.getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor),
		PaletteFile::fromName(PaletteName::Default), renderer);
	return CursorData(&texture, CursorAlignment::TopLeft);
}

void WorldMapPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	bool mPressed = inputManager.keyPressed(e, SDLK_m);

	if (escapePressed || mPressed)
	{
		this->backToGameButton.click(this->getGame());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 originalPoint = this->getGame().getRenderer()
			.nativeToOriginal(mousePosition);

		// Listen for clicks on the map and exit button.
		const auto &worldMapMasks = this->getGame().getMiscAssets().getWorldMapMasks();
		const int maskCount = static_cast<int>(worldMapMasks.size());
		for (int maskID = 0; maskID < maskCount; maskID++)
		{
			const WorldMapMask &mapMask = worldMapMasks.at(maskID);
			const Rect &maskRect = mapMask.getRect();

			if (maskRect.contains(originalPoint))
			{
				// See if the clicked pixel is set in the bitmask.
				const bool success = mapMask.get(originalPoint.x, originalPoint.y);

				if (success)
				{
					// Mask IDs 0 through 8 are provinces, and 9 is the "Exit" button.
					if (maskID < 9)
					{
						// Go to the selected province panel.
						this->provinceButton.click(this->getGame(), maskID,
							std::move(this->travelData));
					}
					else
					{
						// Exit the world map panel.
						this->backToGameButton.click(this->getGame());
					}
					
					break;
				}
			}
		}
	}	
}

void WorldMapPanel::render(Renderer &renderer)
{
	DebugAssert(this->getGame().gameDataIsActive());

	// Clear full screen.
	renderer.clear();

	// Set palette.
	auto &textureManager = this->getGame().getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw world map background. This one has "Exit" at the bottom right.
	const std::string &backgroundFilename = TextureFile::fromName(TextureName::WorldMap);
	const auto &mapBackground = textureManager.getTexture(
		backgroundFilename, PaletteFile::fromName(PaletteName::BuiltIn), renderer);
	renderer.drawOriginal(mapBackground);

	// Draw yellow text over current province name.
	const auto &gameData = this->getGame().getGameData();
	const int provinceID = gameData.getProvinceDefinition().getRaceID();
	const auto &provinceText = textureManager.getTextures(
		TextureFile::fromName(TextureName::ProvinceNames),
		backgroundFilename, renderer).at(provinceID);
	const Int2 &nameOffset = this->provinceNameOffsets.at(provinceID);
	renderer.drawOriginal(provinceText, nameOffset.x, nameOffset.y);
}
