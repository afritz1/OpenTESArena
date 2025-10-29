#include "GuiUtils.h"
#include "UiCommand.h"
#include "UiContext.h"
#include "UiManager.h"
#include "UiRenderSpace.h"
#include "../Game/Game.h"
#include "../Interface/MainMenuUiState.h"

#include "components/debug/Debug.h"

bool UiManager::init(const char *folderPath, TextureManager &textureManager, Renderer &renderer)
{
	// @todo eventually load UI asset txt files from folderPath

	// @todo preload some global things like cursor images

	this->addBeginContextCallback(MainMenuUI::ContextType, MainMenuUI::create);
	this->addEndContextCallback(MainMenuUI::ContextType, MainMenuUI::destroy);

	return true;
}

void UiManager::shutdown(Renderer &renderer)
{
	this->transforms.clear();
	this->elements.clear();
	this->images.clear();
	this->textBoxes.clear();
	this->buttons.clear();
	this->beginContextCallbackLists.clear();
	this->updateContextCallbackLists.clear();
	this->endContextCallbackLists.clear();
	this->activeContextType = std::nullopt;
	this->renderElementsCache.clear();
}

void UiManager::setElementActive(UiElementInstanceID elementInstID, bool active)
{
	UiElement &element = this->elements.get(elementInstID);
	element.active = active;
}

void UiManager::setTransformPosition(UiElementInstanceID elementInstID, Int2 position)
{
	UiElement &element = this->elements.get(elementInstID);
	UiTransform &transform = this->transforms.get(element.transformInstID);
	transform.position = position;
}

void UiManager::setTransformSize(UiElementInstanceID elementInstID, Int2 size)
{
	UiElement &element = this->elements.get(elementInstID);
	UiTransform &transform = this->transforms.get(element.transformInstID);
	transform.size = size;
}

UiElementInstanceID UiManager::createImage(const UiElementInitInfo &initInfo, UiTextureID textureID)
{
	const UiElementInstanceID elementInstID = this->elements.alloc();
	if (elementInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate element for image (scope %d, texture ID %d).", initInfo.contextType, textureID);
		return -1;
	}

	const UiTransformInstanceID transformInstID = this->transforms.alloc();
	if (transformInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate transform for image (scope %d, texture ID %d).", initInfo.contextType, textureID);
		this->elements.free(elementInstID);
		return -1;
	}

	const UiImageInstanceID imageInstID = this->images.alloc();
	if (imageInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate image (scope %d, texture ID %d).", initInfo.contextType, textureID);
		this->transforms.free(transformInstID);
		this->elements.free(elementInstID);
		return -1;
	}

	UiImage &image = this->images.get(imageInstID);
	image.init(textureID);

	UiTransform &transform = this->transforms.get(transformInstID);
	transform.init(initInfo.position, initInfo.size, initInfo.pivotType);

	UiElement &element = this->elements.get(elementInstID);
	element.initImage(initInfo.contextType, initInfo.drawOrder, initInfo.renderSpace, transformInstID, imageInstID);

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

UiElementInstanceID UiManager::createTextBox(const UiElementInitInfo &initInfo)
{
	const UiElementInstanceID elementInstID = this->elements.alloc();
	if (elementInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate element for text box (scope %d).", initInfo.contextType);
		return -1;
	}

	const UiTransformInstanceID transformInstID = this->transforms.alloc();
	if (transformInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate transform for text box (scope %d).", initInfo.contextType);
		this->elements.free(elementInstID);
		return -1;
	}

	const UiTextBoxInstanceID textBoxInstID = this->textBoxes.alloc();
	if (textBoxInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate text box (scope %d).", initInfo.contextType);
		this->transforms.free(transformInstID);
		this->elements.free(elementInstID);
		return -1;
	}

	UiTextBox &textBox = this->textBoxes.get(textBoxInstID);
	textBox.init();

	UiTransform &transform = this->transforms.get(transformInstID);
	transform.init(initInfo.position, initInfo.size, initInfo.pivotType);

	UiElement &element = this->elements.get(elementInstID);
	element.initTextBox(initInfo.contextType, initInfo.drawOrder, initInfo.renderSpace, transformInstID, textBoxInstID);

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

UiElementInstanceID UiManager::createButton(const UiElementInitInfo &initInfo)
{
	const UiElementInstanceID elementInstID = this->elements.alloc();
	if (elementInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate element for button (scope %d).", initInfo.contextType);
		return -1;
	}

	const UiTransformInstanceID transformInstID = this->transforms.alloc();
	if (transformInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate transform for button (scope %d).", initInfo.contextType);
		this->elements.free(elementInstID);
		return -1;
	}

	const UiButtonInstanceID buttonInstID = this->buttons.alloc();
	if (buttonInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate button (scope %d).", initInfo.contextType);
		this->transforms.free(transformInstID);
		this->elements.free(elementInstID);
		return -1;
	}

	UiButton &button = this->buttons.get(buttonInstID);
	button.init();

	UiTransform &transform = this->transforms.get(transformInstID);
	transform.init(initInfo.position, initInfo.size, initInfo.pivotType);

	UiElement &element = this->elements.get(elementInstID);
	element.initButton(initInfo.contextType, initInfo.drawOrder, initInfo.renderSpace, transformInstID, buttonInstID);

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

void UiManager::addBeginContextCallback(UiContextType contextType, const UiContextCallback &callback)
{
	auto iter = this->beginContextCallbackLists.find(contextType);
	if (iter == this->beginContextCallbackLists.end())
	{
		iter = this->beginContextCallbackLists.emplace(contextType, std::vector<UiContextCallback>()).first;
	}

	std::vector<UiContextCallback> &callbacks = iter->second;
	callbacks.emplace_back(callback);
}

void UiManager::addUpdateContextCallback(UiContextType contextType, const UiContextUpdateCallback &callback)
{
	auto iter = this->updateContextCallbackLists.find(contextType);
	if (iter == this->updateContextCallbackLists.end())
	{
		iter = this->updateContextCallbackLists.emplace(contextType, std::vector<UiContextUpdateCallback>()).first;
	}

	std::vector<UiContextUpdateCallback> &callbacks = iter->second;
	callbacks.emplace_back(callback);
}

void UiManager::addEndContextCallback(UiContextType contextType, const UiContextCallback &callback)
{
	auto iter = this->endContextCallbackLists.find(contextType);
	if (iter == this->endContextCallbackLists.end())
	{
		iter = this->endContextCallbackLists.emplace(contextType, std::vector<UiContextCallback>()).first;
	}

	std::vector<UiContextCallback> &callbacks = iter->second;
	callbacks.emplace_back(callback);
}

void UiManager::clearContextCallbacks(UiContextType contextType)
{
	this->beginContextCallbackLists.erase(contextType);
	this->updateContextCallbackLists.erase(contextType);
	this->endContextCallbackLists.erase(contextType);
}

void UiManager::beginContext(UiContextType contextType, Game &game)
{
	if (this->activeContextType == contextType)
	{
		DebugLogErrorFormat("UI context %d already active.", contextType);
		return;
	}

	this->activeContextType = contextType;

	const auto beginIter = this->beginContextCallbackLists.find(contextType);
	if (beginIter != this->beginContextCallbackLists.end())
	{
		for (const UiContextCallback &callback : beginIter->second)
		{
			callback(game);
		}
	}
}

void UiManager::endContext(UiContextType contextType, Game &game)
{
	if (this->activeContextType != contextType)
	{
		DebugLogErrorFormat("Expected UI context %d to be active.", contextType);
		return;
	}

	const auto endIter = this->endContextCallbackLists.find(contextType);
	if (endIter != this->endContextCallbackLists.end())
	{
		for (const UiContextCallback &callback : endIter->second)
		{
			callback(game);
		}
	}

	this->activeContextType = std::nullopt;
}

bool UiManager::isContextActive(UiContextType contextType) const
{
	if (contextType == UiContextType::Global)
	{
		return true;
	}

	return this->activeContextType == contextType;
}

void UiManager::populateCommandList(UiCommandList &commandList)
{
	commandList.addElements(this->renderElementsCache);
}

void UiManager::update(double dt, Game &game)
{
	if (this->activeContextType.has_value())
	{
		const auto updateIter = this->updateContextCallbackLists.find(*this->activeContextType);
		if (updateIter != this->updateContextCallbackLists.end())
		{
			for (const UiContextUpdateCallback &callback : updateIter->second)
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

		if (!this->isContextActive(element.contextType))
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
		const Int2 size = transform.size;
		const UiRenderSpace renderSpace = element->renderSpace;

		RenderElement2D renderElement;
		switch (element->type)
		{
		case UiElementType::Image:
		{
			const UiImage &image = this->images.get(element->imageInstID);
			const UiTextureID imageTextureID = image.textureID;
			renderElement.id = imageTextureID;
			break;
		}
		case UiElementType::TextBox:
		{
			const UiTextBox &textBox = this->textBoxes.get(element->textBoxInstID);
			// @todo text box texture
			DebugNotImplemented();
			break;
		}
		case UiElementType::Button:
		{
			const UiButton &button = this->buttons.get(element->buttonInstID);
			// @todo optional UiTextureID
			DebugNotImplemented();
			break;
		}
		default:
			DebugNotImplementedMsg(std::to_string(static_cast<int>(element->type)));
			break;
		}

		renderElement.rect = GuiUtils::makeWindowSpaceRect(position.x, position.y, size.x, size.y, transform.pivotType, renderSpace, windowDims.x, windowDims.y, letterboxRect);

		if (renderElement.id >= 0)
		{
			this->renderElementsCache.emplace_back(std::move(renderElement));
		}
	}
}
