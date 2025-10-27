#include "GuiUtils.h"
#include "RenderSpace.h"
#include "UiCommand.h"
#include "UiManager.h"
#include "../Game/Game.h"

#include "components/debug/Debug.h"

bool UiManager::init(const char *folderPath, TextureManager &textureManager, Renderer &renderer)
{
	// @todo eventually load UI asset txt files from folderPath

	// @todo preload some global things like cursor images

	return true;
}

void UiManager::shutdown(Renderer &renderer)
{
	this->transforms.clear();
	this->elements.clear();
	this->images.clear();
	this->textBoxes.clear();
	this->buttons.clear();
	this->beginScopeCallbackLists.clear();
	this->updateScopeCallbackLists.clear();
	this->endScopeCallbackLists.clear();
	this->renderElementsCache.clear();
}

void UiManager::setElementActive(UiElementInstanceID elementInstID, bool active)
{
	UiElement &element = this->elements.get(elementInstID);
	element.active = active;
}

UiElementInstanceID UiManager::createImage(UiScope scope, UiTextureID textureID)
{
	const UiElementInstanceID elementInstID = this->elements.alloc();
	if (elementInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate element for image (scope %d, texture ID %d).", scope, textureID);
		return -1;
	}

	const UiTransformInstanceID transformInstID = this->transforms.alloc();
	if (transformInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate transform for image (scope %d, texture ID %d).", scope, textureID);
		this->elements.free(elementInstID);
		return -1;
	}

	const UiImageInstanceID imageInstID = this->images.alloc();
	if (imageInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate image (scope %d, texture ID %d).", scope, textureID);
		this->transforms.free(transformInstID);
		this->elements.free(elementInstID);
		return -1;
	}

	UiImage &image = this->images.get(imageInstID);
	image.init(textureID);

	UiElement &element = this->elements.get(elementInstID);
	element.initImage(scope, transformInstID, imageInstID);

	return elementInstID;
}

void UiManager::setImageTexture(UiElementInstanceID elementInstID, UiTextureID textureID)
{
	UiElement &element = this->elements.get(elementInstID);

	DebugAssert(element.type == UiElementType::Image);
	UiImage &image = this->images.get(element.imageInstID);
	image.textureID = textureID;
}

void UiManager::freeImage(UiElementInstanceID elementInstID)
{
	UiElement *element = this->elements.tryGet(elementInstID);
	if (element == nullptr)
	{
		return;
	}

	DebugAssert(element->type == UiElementType::Image);
	this->images.free(element->imageInstID);
	this->transforms.free(element->transformInstID);
	this->elements.free(elementInstID);
}

UiElementInstanceID UiManager::createTextBox(UiScope scope)
{
	const UiElementInstanceID elementInstID = this->elements.alloc();
	if (elementInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate element for text box (scope %d).", scope);
		return -1;
	}

	const UiTransformInstanceID transformInstID = this->transforms.alloc();
	if (transformInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate transform for text box (scope %d).", scope);
		this->elements.free(elementInstID);
		return -1;
	}

	const UiTextBoxInstanceID textBoxInstID = this->textBoxes.alloc();
	if (textBoxInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate text box (scope %d).", scope);
		this->transforms.free(transformInstID);
		this->elements.free(elementInstID);
		return -1;
	}

	UiTextBox &textBox = this->textBoxes.get(textBoxInstID);
	textBox.init();

	UiElement &element = this->elements.get(elementInstID);
	element.initTextBox(scope, transformInstID, textBoxInstID);

	return elementInstID;
}

void UiManager::setTextBoxText(UiElementInstanceID elementInstID, const char *str)
{
	UiElement &element = this->elements.get(elementInstID);
	
	DebugAssert(element.type == UiElementType::TextBox);
	UiTextBox &textBox = this->textBoxes.get(element.textBoxInstID);
	DebugNotImplemented();
	//textBox.text = str;
}

void UiManager::freeTextBox(UiElementInstanceID elementInstID)
{
	UiElement *element = this->elements.tryGet(elementInstID);
	if (element == nullptr)
	{
		return;
	}

	DebugAssert(element->type == UiElementType::TextBox);
	this->textBoxes.free(element->textBoxInstID);
	this->transforms.free(element->transformInstID);
	this->elements.free(elementInstID);
}

UiElementInstanceID UiManager::createButton(UiScope scope)
{
	const UiElementInstanceID elementInstID = this->elements.alloc();
	if (elementInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate element for text box (scope %d).", scope);
		return -1;
	}

	const UiTransformInstanceID transformInstID = this->transforms.alloc();
	if (transformInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate transform for text box (scope %d).", scope);
		this->elements.free(elementInstID);
		return -1;
	}

	const UiButtonInstanceID buttonInstID = this->buttons.alloc();
	if (buttonInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate button (scope %d).", scope);
		this->transforms.free(transformInstID);
		this->elements.free(elementInstID);
		return -1;
	}

	UiButton &button = this->buttons.get(buttonInstID);
	button.init();

	UiElement &element = this->elements.get(elementInstID);
	element.initButton(scope, transformInstID, buttonInstID);

	return elementInstID;
}

void UiManager::freeButton(UiElementInstanceID elementInstID)
{
	UiElement *element = this->elements.tryGet(elementInstID);
	if (element == nullptr)
	{
		return;
	}

	DebugAssert(element->type == UiElementType::Button);
	this->buttons.free(element->buttonInstID);
	this->transforms.free(element->transformInstID);
	this->elements.free(elementInstID);
}

void UiManager::addBeginScopeCallback(UiScope scope, const UiScopeCallback &callback)
{
	auto iter = this->beginScopeCallbackLists.find(scope);
	if (iter == this->beginScopeCallbackLists.end())
	{
		iter = this->beginScopeCallbackLists.emplace(scope, std::vector<UiScopeCallback>()).first;
	}

	std::vector<UiScopeCallback> &callbacks = iter->second;
	callbacks.emplace_back(callback);
}

void UiManager::addUpdateScopeCallback(UiScope scope, const UiScopeUpdateCallback &callback)
{
	auto iter = this->updateScopeCallbackLists.find(scope);
	if (iter == this->updateScopeCallbackLists.end())
	{
		iter = this->updateScopeCallbackLists.emplace(scope, std::vector<UiScopeUpdateCallback>()).first;
	}

	std::vector<UiScopeUpdateCallback> &callbacks = iter->second;
	callbacks.emplace_back(callback);
}

void UiManager::addEndScopeCallback(UiScope scope, const UiScopeCallback &callback)
{
	auto iter = this->endScopeCallbackLists.find(scope);
	if (iter == this->endScopeCallbackLists.end())
	{
		iter = this->endScopeCallbackLists.emplace(scope, std::vector<UiScopeCallback>()).first;
	}

	std::vector<UiScopeCallback> &callbacks = iter->second;
	callbacks.emplace_back(callback);
}

void UiManager::clearScopeCallbacks(UiScope scope)
{
	this->beginScopeCallbackLists.erase(scope);
	this->updateScopeCallbackLists.erase(scope);
	this->endScopeCallbackLists.erase(scope);
}

void UiManager::beginScope(UiScope scope, Game &game)
{
	const auto activeIter = std::find(this->activeScopes.begin(), this->activeScopes.end(), scope);
	if (activeIter != this->activeScopes.end())
	{
		DebugLogErrorFormat("UI scope %d already active.", scope);
		return;
	}

	this->activeScopes.emplace_back(scope);

	const auto beginIter = this->beginScopeCallbackLists.find(scope);
	if (beginIter != this->beginScopeCallbackLists.end())
	{
		for (const UiScopeCallback &callback : beginIter->second)
		{
			callback(game);
		}
	}
}

void UiManager::endScope(UiScope scope, Game &game)
{
	const auto activeIter = std::find(this->activeScopes.begin(), this->activeScopes.end(), scope);
	if (activeIter == this->activeScopes.end())
	{
		DebugLogErrorFormat("Expected UI scope %d to be active.", scope);
		return;
	}

	const auto endIter = this->endScopeCallbackLists.find(scope);
	if (endIter != this->endScopeCallbackLists.end())
	{
		for (const UiScopeCallback &callback : endIter->second)
		{
			callback(game);
		}
	}

	this->activeScopes.erase(activeIter);
}

bool UiManager::isScopeActive(UiScope scope) const
{
	const auto iter = std::find(this->activeScopes.begin(), this->activeScopes.end(), scope);
	return iter != this->activeScopes.end();
}

void UiManager::populateCommandList(UiCommandList &commandList)
{
	commandList.addElements(this->renderElementsCache);
}

void UiManager::update(double dt, Game &game)
{
	for (const UiScope scope : this->activeScopes)
	{
		const auto updateIter = this->updateScopeCallbackLists.find(scope);
		if (updateIter != this->updateScopeCallbackLists.end())
		{
			for (const UiScopeUpdateCallback &callback : updateIter->second)
			{
				callback(dt, game);
			}
		}
	}

	this->renderElementsCache.clear();

	std::vector<const UiElement*> elementsToDraw;
	for (const UiElement &element : this->elements.values)
	{
		if (!element.active)
		{
			continue;
		}

		if (element.drawOrder < 0)
		{
			continue;
		}

		if (!this->isScopeActive(element.scope))
		{
			continue;
		}

		elementsToDraw.emplace_back(&element);
	}

	std::sort(elementsToDraw.begin(), elementsToDraw.end(),
		[](const UiElement *a, const UiElement *b)
	{
		return a->drawOrder < b->drawOrder;
	});

	const Renderer &renderer = game.renderer;
	const Window &window = game.window;
	const Int2 windowDims = window.getPixelDimensions();
	const Rect letterboxRect = window.getLetterboxRect();

	for (const UiElement *element : elementsToDraw)
	{
		const UiTransform &transform = this->transforms.get(element->transformInstID);
		const Int2 position = transform.position;
		constexpr RenderSpace renderSpace = RenderSpace::Classic; // @todo put in UiElement

		int width = 0;
		int height = 0;

		RenderElement2D renderElement;
		switch (element->type)
		{
		case UiElementType::Image:
		{
			const UiImage &image = this->images.get(element->imageInstID);
			const UiTextureID imageTextureID = image.textureID;
			renderElement.id = imageTextureID;

			const std::optional<Int2> imageDims = renderer.tryGetUiTextureDims(imageTextureID);
			DebugAssert(imageDims.has_value());
			width = imageDims->x;
			height = imageDims->y;
			break;
		}
		case UiElementType::TextBox:
		{
			const UiTextBox &textBox = this->textBoxes.get(element->textBoxInstID);
			// @todo text box texture
			break;
		}
		case UiElementType::Button:
		{
			const UiButton &button = this->buttons.get(element->buttonInstID);
			// @todo optional UiTextureID
			break;
		}
		default:
			DebugNotImplementedMsg(std::to_string(static_cast<int>(element->type)));
			break;
		}

		renderElement.rect = GuiUtils::makeWindowSpaceRect(position.x, position.y, width, height, transform.pivotType, renderSpace, windowDims.x, windowDims.y, letterboxRect);

		if (renderElement.id >= 0)
		{
			this->renderElementsCache.emplace_back(std::move(renderElement));
		}
	}
}
