#include "SDL.h"

#include "CinematicPanel.h"
#include "ImageSequencePanel.h"
#include "ImagePanel.h"
#include "MainMenuPanel.h"
#include "Panel.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Rendering/Renderer.h"
#include "../UI/CursorAlignment.h"
#include "../UI/CursorData.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"

#include "components/vfs/manager.hpp"

Panel::Panel(Game &game)
	: game(game) { }

Panel::~Panel()
{
	InputManager &inputManager = this->game.getInputManager();

	// Free all the input listener IDs.
	for (const InputManager::ListenerID listenerID : this->inputActionListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	for (const InputManager::ListenerID listenerID : this->mouseButtonChangedListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	for (const InputManager::ListenerID listenerID : this->mouseButtonHeldListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	for (const InputManager::ListenerID listenerID : this->mouseScrollChangedListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	for (const InputManager::ListenerID listenerID : this->mouseMotionListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}
}

std::optional<CursorData> Panel::getCurrentCursor() const
{
	// Empty by default.
	return std::nullopt;
}

void Panel::handleEvent(const SDL_Event &e)
{
	// Do nothing by default.
	static_cast<void>(e);
}

void Panel::onPauseChanged(bool paused)
{
	// Do nothing by default.
	static_cast<void>(paused);
}

void Panel::resize(int windowWidth, int windowHeight)
{
	// Do nothing by default.
	static_cast<void>(windowWidth);
	static_cast<void>(windowHeight);
}

Game &Panel::getGame() const
{
	return this->game;
}

CursorData Panel::getDefaultCursor() const
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &textureManager = game.getTextureManager();

	const std::string &paletteFilename = ArenaPaletteName::Default;
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteFilename.c_str());
	if (!paletteID.has_value())
	{
		DebugCrash("Couldn't get palette ID for \"" + paletteFilename + "\".");
	}

	const std::string &textureFilename = ArenaTextureName::SwordCursor;
	const std::optional<TextureBuilderID> textureBuilderID =
		textureManager.tryGetTextureBuilderID(textureFilename.c_str());
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + textureFilename + "\".");
	}

	return CursorData(*textureBuilderID, *paletteID, CursorAlignment::TopLeft);
}

void Panel::addInputActionListener(const std::string_view &actionName, const InputActionCallback &callback)
{
	auto &inputManager = this->game.getInputManager();
	this->inputActionListenerIDs.emplace_back(inputManager.addInputActionListener(actionName, callback));
}

void Panel::addMouseButtonChangedListener(const MouseButtonChangedCallback &callback)
{
	auto &inputManager = this->game.getInputManager();
	this->mouseButtonChangedListenerIDs.emplace_back(inputManager.addMouseButtonChangedListener(callback));
}

void Panel::addMouseButtonHeldListener(const MouseButtonHeldCallback &callback)
{
	auto &inputManager = this->game.getInputManager();
	this->mouseButtonHeldListenerIDs.emplace_back(inputManager.addMouseButtonHeldListener(callback));
}

void Panel::addMouseScrollChangedListener(const MouseScrollChangedCallback &callback)
{
	auto &inputManager = this->game.getInputManager();
	this->mouseScrollChangedListenerIDs.emplace_back(inputManager.addMouseScrollChangedListener(callback));
}

void Panel::addMouseMotionListener(const MouseMotionCallback &callback)
{
	auto &inputManager = this->game.getInputManager();
	this->mouseMotionListenerIDs.emplace_back(inputManager.addMouseMotionListener(callback));
}

void Panel::tick(double dt)
{
	// Do nothing by default.
	static_cast<void>(dt);
}

void Panel::renderSecondary(Renderer &renderer)
{
	// Do nothing by default.
	static_cast<void>(renderer);
}
