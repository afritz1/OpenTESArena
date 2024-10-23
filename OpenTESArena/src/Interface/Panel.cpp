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
#include "../Rendering/Renderer.h"
#include "../UI/CursorAlignment.h"
#include "../UI/CursorData.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"
#include "../Utilities/Color.h"

#include "components/vfs/manager.hpp"

Panel::Panel(Game &game)
	: game(game)
{
	this->paused = false;
}

Panel::~Panel()
{
	InputManager &inputManager = this->game.inputManager;

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

	for (const InputManager::ListenerID listenerID : this->textInputListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}
}

BufferView<const ButtonProxy> Panel::getButtonProxies() const
{
	return BufferView<const ButtonProxy>(this->buttonProxies);
}

BufferView<const UiDrawCall> Panel::getDrawCalls() const
{
	return BufferView<const UiDrawCall>(this->drawCalls);
}

void Panel::onPauseChanged(bool paused)
{
	this->paused = paused;

	InputManager &inputManager = this->game.inputManager;
	auto setListenersEnabled = [paused, &inputManager](std::vector<InputManager::ListenerID> &listenerIDs)
	{
		for (const InputManager::ListenerID listenerID : listenerIDs)
		{
			inputManager.setListenerEnabled(listenerID, !paused);
		}
	};

	// Update listener active states so paused panels aren't hearing input callbacks.
	setListenersEnabled(this->inputActionListenerIDs);
	setListenersEnabled(this->mouseButtonChangedListenerIDs);
	setListenersEnabled(this->mouseButtonHeldListenerIDs);
	setListenersEnabled(this->mouseScrollChangedListenerIDs);
	setListenersEnabled(this->mouseMotionListenerIDs);
	setListenersEnabled(this->textInputListenerIDs);
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

bool Panel::isPaused() const
{
	return this->paused;
}

CursorData Panel::getDefaultCursor() const
{
	auto &game = this->getGame();
	auto &renderer = game.renderer;
	auto &textureManager = game.textureManager;

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
	auto &inputManager = this->game.inputManager;
	this->inputActionListenerIDs.emplace_back(inputManager.addInputActionListener(actionName, callback));
}

void Panel::addMouseButtonChangedListener(const MouseButtonChangedCallback &callback)
{
	auto &inputManager = this->game.inputManager;
	this->mouseButtonChangedListenerIDs.emplace_back(inputManager.addMouseButtonChangedListener(callback));
}

void Panel::addMouseButtonHeldListener(const MouseButtonHeldCallback &callback)
{
	auto &inputManager = this->game.inputManager;
	this->mouseButtonHeldListenerIDs.emplace_back(inputManager.addMouseButtonHeldListener(callback));
}

void Panel::addMouseScrollChangedListener(const MouseScrollChangedCallback &callback)
{
	auto &inputManager = this->game.inputManager;
	this->mouseScrollChangedListenerIDs.emplace_back(inputManager.addMouseScrollChangedListener(callback));
}

void Panel::addMouseMotionListener(const MouseMotionCallback &callback)
{
	auto &inputManager = this->game.inputManager;
	this->mouseMotionListenerIDs.emplace_back(inputManager.addMouseMotionListener(callback));
}

void Panel::addTextInputListener(const TextInputCallback &callback)
{
	auto &inputManager = this->game.inputManager;
	this->textInputListenerIDs.emplace_back(inputManager.addTextInputListener(callback));
}

void Panel::addButtonProxy(MouseButtonType buttonType, const ButtonProxy::RectFunction &rectFunc,
	const ButtonProxy::Callback &callback, const ButtonProxy::ActiveFunction &isActiveFunc)
{
	this->buttonProxies.emplace_back(buttonType, rectFunc, callback, isActiveFunc);
}

void Panel::addButtonProxy(MouseButtonType buttonType, const Rect &rect, const ButtonProxy::Callback &callback,
	const ButtonProxy::ActiveFunction &isActiveFunc)
{
	auto rectFunc = [rect]() { return rect; };
	this->addButtonProxy(buttonType, rectFunc, callback, isActiveFunc);
}

void Panel::clearButtonProxies()
{
	this->buttonProxies.clear();
}

void Panel::addDrawCall(const UiDrawCall::TextureFunc &textureFunc, const UiDrawCall::PositionFunc &positionFunc,
	const UiDrawCall::SizeFunc &sizeFunc, const UiDrawCall::PivotFunc &pivotFunc,
	const UiDrawCall::ActiveFunc &activeFunc, const std::optional<Rect> &clipRect, RenderSpace renderSpace)
{
	this->drawCalls.emplace_back(textureFunc, positionFunc, sizeFunc, pivotFunc, activeFunc, clipRect, renderSpace);
}

void Panel::addDrawCall(const UiDrawCall::TextureFunc &textureFunc, const Int2 &position, const Int2 &size,
	PivotType pivotType, const std::optional<Rect> &clipRect)
{
	this->drawCalls.emplace_back(
		textureFunc,
		UiDrawCall::makePositionFunc(position),
		UiDrawCall::makeSizeFunc(size),
		UiDrawCall::makePivotFunc(pivotType),
		UiDrawCall::defaultActiveFunc,
		clipRect);
}

void Panel::addDrawCall(UiTextureID textureID, const Int2 &position, const Int2 &size, PivotType pivotType,
	const std::optional<Rect> &clipRect)
{
	this->drawCalls.emplace_back(
		UiDrawCall::makeTextureFunc(textureID),
		UiDrawCall::makePositionFunc(position),
		UiDrawCall::makeSizeFunc(size),
		UiDrawCall::makePivotFunc(pivotType),
		UiDrawCall::defaultActiveFunc,
		clipRect);
}

void Panel::addCursorDrawCall(UiTextureID textureID, PivotType pivotType, const UiDrawCall::ActiveFunc &activeFunc)
{
	UiDrawCall::TextureFunc textureFunc = [textureID]()
	{
		return textureID;
	};

	UiDrawCall::PositionFunc positionFunc = [this]()
	{
		auto &game = this->getGame();
		const auto &inputManager = game.inputManager;
		return inputManager.getMousePosition();
	};

	UiDrawCall::SizeFunc sizeFunc = [this, textureID]()
	{
		auto &game = this->getGame();
		auto &renderer = game.renderer;
		const std::optional<Int2> dims = renderer.tryGetUiTextureDims(textureID);
		if (!dims.has_value())
		{
			DebugCrash("Couldn't get cursor texture dimensions for UI draw call.");
		}

		const auto &options = game.options;
		const double scale = options.getGraphics_CursorScale();
		const Int2 scaledDims(
			static_cast<int>(static_cast<double>(dims->x) * scale),
			static_cast<int>(static_cast<double>(dims->y) * scale));
		return scaledDims;
	};

	UiDrawCall::PivotFunc pivotFunc = [pivotType]()
	{
		return pivotType;
	};

	const std::optional<Rect> clipRect = std::nullopt;
	constexpr RenderSpace renderSpace = RenderSpace::Native;

	this->drawCalls.emplace_back(
		textureFunc,
		positionFunc,
		sizeFunc,
		pivotFunc,
		activeFunc,
		clipRect,
		renderSpace);
}

void Panel::addCursorDrawCall(UiTextureID textureID, PivotType pivotType)
{
	this->addCursorDrawCall(textureID, pivotType, UiDrawCall::defaultActiveFunc);
}

void Panel::clearDrawCalls()
{
	this->drawCalls.clear();
}

void Panel::tick(double dt)
{
	// Do nothing by default.
	static_cast<void>(dt);
}
