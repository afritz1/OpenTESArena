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
#include "../UI/FontLibrary.h"
#include "../UI/GuiUtils.h"
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"
#include "../UI/UiCommand.h"
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
	for (const InputListenerID listenerID : this->inputActionListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	for (const InputListenerID listenerID : this->mouseButtonChangedListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	for (const InputListenerID listenerID : this->mouseButtonHeldListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	for (const InputListenerID listenerID : this->mouseScrollChangedListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	for (const InputListenerID listenerID : this->mouseMotionListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	for (const InputListenerID listenerID : this->textInputListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}
}

Span<const ButtonProxy> Panel::getButtonProxies() const
{
	return Span<const ButtonProxy>(this->buttonProxies);
}

void Panel::populateCommandList(UiCommandList &commandList)
{
	this->renderElementsCache.clear();

	const Window &window = this->game.window;
	const Int2 windowDims = window.getPixelDimensions();
	const Rect letterboxRect = window.getLetterboxRect();

	for (const UiDrawCall &drawCall : this->drawCalls)
	{
		if (!drawCall.activeFunc())
		{
			continue;
		}

		const UiTextureID textureID = drawCall.textureFunc();
		const Int2 position = drawCall.positionFunc();
		const Int2 size = drawCall.sizeFunc();
		const UiPivotType pivotType = drawCall.pivotFunc();
		const UiRenderSpace renderSpace = drawCall.renderSpace;
		const std::optional<Rect> &clipRect = drawCall.clipRect;

		const Rect presentRect = GuiUtils::makeWindowSpaceRect(position.x, position.y, size.x, size.y, pivotType, renderSpace, windowDims.x, windowDims.y, letterboxRect);

		Rect presentClipRect;
		if (clipRect.has_value())
		{
			presentClipRect = GuiUtils::makeWindowSpaceRect(*clipRect, UiPivotType::TopLeft, renderSpace, windowDims.x, windowDims.y, letterboxRect);
		}

		this->renderElementsCache.emplace_back(RenderElement2D(textureID, presentRect, presentClipRect));
	}

	if (!this->renderElementsCache.empty())
	{
		commandList.addElements(this->renderElementsCache);
	}
}

void Panel::onPauseChanged(bool paused)
{
	this->paused = paused;

	InputManager &inputManager = this->game.inputManager;
	auto setListenersEnabled = [paused, &inputManager](std::vector<InputListenerID> &listenerIDs)
	{
		for (const InputListenerID listenerID : listenerIDs)
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

void Panel::addInputActionListener(const std::string_view actionName, const InputActionCallback &callback)
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
	const ButtonProxy::Callback &callback, const Rect &parentRect, const ButtonProxy::ActiveFunction &isActiveFunc)
{
	this->buttonProxies.emplace_back(buttonType, rectFunc, callback, parentRect, isActiveFunc);
}

void Panel::addButtonProxy(MouseButtonType buttonType, const Rect &rect, const ButtonProxy::Callback &callback,
	const Rect &parentRect, const ButtonProxy::ActiveFunction &isActiveFunc)
{
	auto rectFunc = [rect]() { return rect; };
	this->addButtonProxy(buttonType, rectFunc, callback, parentRect, isActiveFunc);
}

void Panel::clearButtonProxies()
{
	this->buttonProxies.clear();
}

void Panel::addDrawCall(const UiDrawCallInitInfo &initInfo)
{
	this->drawCalls.emplace_back(initInfo);
}

void Panel::addCursorDrawCall(UiTextureID textureID, UiPivotType pivotType, const UiDrawCallActiveFunc &activeFunc)
{
	UiDrawCallTextureFunc textureFunc = UiDrawCall::makeTextureFunc(textureID);

	UiDrawCallPositionFunc positionFunc = [this]()
	{
		auto &game = this->getGame();
		const auto &inputManager = game.inputManager;
		return inputManager.getMousePosition();
	};

	UiDrawCallSizeFunc sizeFunc = [this, textureID]()
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

	UiDrawCallPivotFunc pivotFunc = UiDrawCall::makePivotFunc(pivotType);
	const std::optional<Rect> clipRect = std::nullopt;
	constexpr UiRenderSpace renderSpace = UiRenderSpace::Native;

	this->drawCalls.emplace_back(
		textureFunc,
		positionFunc,
		sizeFunc,
		pivotFunc,
		activeFunc,
		clipRect,
		renderSpace);
}

void Panel::addCursorDrawCall(UiTextureID textureID, UiPivotType pivotType)
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
