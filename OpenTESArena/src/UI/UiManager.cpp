#include "FontLibrary.h"
#include "GuiUtils.h"
#include "TextRenderUtils.h"
#include "UiButton.h"
#include "UiCommand.h"
#include "UiContext.h"
#include "UiImage.h"
#include "UiManager.h"
#include "UiRenderSpace.h"
#include "UiTextBox.h"
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

Rect UiManager::getTransformGlobalRect(UiElementInstanceID elementInstID) const
{
	const UiElement &element = this->elements.get(elementInstID);
	const UiTransform &transform = this->transforms.get(element.transformInstID);
	// @todo parent transform calculation if any

	const Rect globalRect = GuiUtils::getPivotCorrectedRect(transform.position, transform.size, transform.pivotType);
	return globalRect;
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

const UiButtonCallback &UiManager::getButtonCallback(UiElementInstanceID elementInstID) const
{
	const UiElement &element = this->elements.get(elementInstID);
	DebugAssert(element.type == UiElementType::Button);
	const UiButton &button = this->buttons.get(element.buttonInstID);
	return button.callback;
}

bool UiManager::isMouseButtonValidForButton(MouseButtonType mouseButtonType, UiElementInstanceID elementInstID) const
{
	const UiElement &element = this->elements.get(elementInstID);
	DebugAssert(element.type == UiElementType::Button);
	const UiButton &button = this->buttons.get(element.buttonInstID);
	return MouseButtonTypeFlags(mouseButtonType).any(button.mouseButtonFlags);
}

std::vector<UiElementInstanceID> UiManager::getActiveButtonInstIDs() const
{
	std::vector<UiElementInstanceID> activeButtonInstIDs;

	for (const UiElementInstanceID instID : this->elements.keys)
	{
		const UiElement &element = this->elements.get(instID);
		if (!element.active)
		{
			continue;
		}

		if (element.type != UiElementType::Button)
		{
			continue;
		}

		if (!this->isContextActive(element.contextType))
		{
			continue;
		}

		activeButtonInstIDs.emplace_back(instID);
	}

	return activeButtonInstIDs;
}

UiElementInstanceID UiManager::createImage(const UiElementInitInfo &initInfo, UiTextureID textureID, UiContextElements &contextElements)
{
	const UiElementInstanceID elementInstID = this->elements.alloc();
	if (elementInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate element for image (context %d, texture ID %d).", initInfo.contextType, textureID);
		return -1;
	}

	const UiTransformInstanceID transformInstID = this->transforms.alloc();
	if (transformInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate transform for image (context %d, texture ID %d).", initInfo.contextType, textureID);
		this->elements.free(elementInstID);
		return -1;
	}

	const UiImageInstanceID imageInstID = this->images.alloc();
	if (imageInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate image (context %d, texture ID %d).", initInfo.contextType, textureID);
		this->transforms.free(transformInstID);
		this->elements.free(elementInstID);
		return -1;
	}

	UiImage &image = this->images.get(imageInstID);
	image.init(textureID);

	UiTransform &transform = this->transforms.get(transformInstID);
	transform.init(initInfo.position, initInfo.size, initInfo.sizeType, initInfo.pivotType);

	UiElement &element = this->elements.get(elementInstID);
	element.initImage(initInfo.contextType, initInfo.drawOrder, initInfo.renderSpace, transformInstID, imageInstID);

	contextElements.imageElementInstIDs.emplace_back(elementInstID);

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

UiElementInstanceID UiManager::createTextBox(const UiElementInitInfo &initInfo, const UiTextBoxInitInfo &textBoxInitInfo, UiContextElements &contextElements, Renderer &renderer)
{
	const UiElementInstanceID elementInstID = this->elements.alloc();
	if (elementInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate element for text box (context %d).", initInfo.contextType);
		return -1;
	}

	const UiTransformInstanceID transformInstID = this->transforms.alloc();
	if (transformInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate transform for text box (context %d).", initInfo.contextType);
		this->elements.free(elementInstID);
		return -1;
	}

	const UiTextBoxInstanceID textBoxInstID = this->textBoxes.alloc();
	if (textBoxInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate text box (context %d).", initInfo.contextType);
		this->transforms.free(transformInstID);
		this->elements.free(elementInstID);
		return -1;
	}

	const FontLibrary &fontLibrary = FontLibrary::getInstance();
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(textBoxInitInfo.fontName, &fontDefIndex))
	{
		DebugLogErrorFormat("Couldn't get font definition index for \"%s\".", textBoxInitInfo.fontName);
		this->textBoxes.free(textBoxInstID);
		this->transforms.free(transformInstID);
		this->elements.free(elementInstID);
		return -1;
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(textBoxInitInfo.worstCaseText, fontDef, textBoxInitInfo.shadowInfo, textBoxInitInfo.lineSpacing);
	const UiTextureID textBoxTextureID = renderer.createUiTexture(textureGenInfo.width, textureGenInfo.height);

	UiTextBox &textBox = this->textBoxes.get(textBoxInstID);
	textBox.init(textBoxTextureID, textureGenInfo.width, textureGenInfo.height, fontDefIndex, textBoxInitInfo.defaultColor, textBoxInitInfo.alignment, textBoxInitInfo.lineSpacing);
	textBox.text = textBoxInitInfo.text;

	UiTransform &transform = this->transforms.get(transformInstID);
	transform.init(initInfo.position, initInfo.size, initInfo.sizeType, initInfo.pivotType);

	UiElement &element = this->elements.get(elementInstID);
	element.initTextBox(initInfo.contextType, initInfo.drawOrder, initInfo.renderSpace, transformInstID, textBoxInstID);

	contextElements.textBoxElementInstIDs.emplace_back(elementInstID);

	return elementInstID;
}

void UiManager::setTextBoxText(UiElementInstanceID elementInstID, const char *str)
{
	UiElement &element = this->elements.get(elementInstID);

	DebugAssert(element.type == UiElementType::TextBox);
	UiTextBox &textBox = this->textBoxes.get(element.textBoxInstID);
	textBox.text = str;
	textBox.dirty = true;
}

void UiManager::freeTextBox(UiElementInstanceID elementInstID, Renderer &renderer)
{
	UiElement *element = this->elements.tryGet(elementInstID);
	if (element == nullptr)
	{
		return;
	}

	DebugAssert(element->type == UiElementType::TextBox);
	UiTextBox &textBox = this->textBoxes.get(element->textBoxInstID);
	textBox.free(renderer);

	this->textBoxes.free(element->textBoxInstID);
	this->transforms.free(element->transformInstID);
	this->elements.free(elementInstID);
}

UiElementInstanceID UiManager::createButton(const UiElementInitInfo &initInfo, const UiButtonInitInfo &buttonInitInfo, UiContextElements &contextElements)
{
	const UiElementInstanceID elementInstID = this->elements.alloc();
	if (elementInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate element for button (context %d).", initInfo.contextType);
		return -1;
	}

	const UiTransformInstanceID transformInstID = this->transforms.alloc();
	if (transformInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate transform for button (context %d).", initInfo.contextType);
		this->elements.free(elementInstID);
		return -1;
	}

	const UiButtonInstanceID buttonInstID = this->buttons.alloc();
	if (buttonInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate button (context %d).", initInfo.contextType);
		this->transforms.free(transformInstID);
		this->elements.free(elementInstID);
		return -1;
	}

	UiButton &button = this->buttons.get(buttonInstID);
	button.init(buttonInitInfo.mouseButtonFlags, buttonInitInfo.callback, buttonInitInfo.contentElementInstID);

	UiTransform &transform = this->transforms.get(transformInstID);
	transform.init(initInfo.position, initInfo.size, initInfo.sizeType, initInfo.pivotType);

	UiElement &element = this->elements.get(elementInstID);
	element.initButton(initInfo.contextType, initInfo.drawOrder, initInfo.renderSpace, transformInstID, buttonInstID);

	contextElements.buttonElementInstIDs.emplace_back(elementInstID);

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

	Renderer &renderer = game.renderer;

	// Update dirty text boxes.
	for (UiTextBox &textBox : this->textBoxes.values)
	{
		if (!textBox.dirty)
		{
			continue;
		}

		const UiTextureID textBoxTextureID = textBox.textureID;
		LockedTexture lockedTexture = renderer.lockUiTexture(textBoxTextureID);
		if (!lockedTexture.isValid())
		{
			DebugLogError("Couldn't lock text box UI texture for updating.");
			return;
		}

		Span2D<uint32_t> texels = lockedTexture.getTexels32();
		texels.fill(0);

		if (!textBox.text.empty())
		{
			const FontLibrary &fontLibrary = FontLibrary::getInstance();
			const FontDefinition &fontDef = fontLibrary.getDefinition(textBox.fontDefIndex);

			const Buffer<std::string_view> textLines = TextRenderUtils::getTextLines(textBox.text);
			const TextRenderColorOverrideInfo *colorOverrideInfoPtr = (textBox.colorOverrideInfo.getEntryCount() > 0) ? &textBox.colorOverrideInfo : nullptr;
			const TextRenderShadowInfo *shadowInfoPtr = textBox.shadowInfo.has_value() ? &(*textBox.shadowInfo) : nullptr;
			TextRenderUtils::drawTextLines(textLines, fontDef, 0, 0, textBox.defaultColor, textBox.alignment, textBox.lineSpacing,
				colorOverrideInfoPtr, shadowInfoPtr, texels);
		}

		renderer.unlockUiTexture(textBoxTextureID);
		textBox.dirty = false;
	}

	// Update element sizes with no dependency.
	for (UiElement &element : this->elements.values)
	{
		UiTransform &transform = this->transforms.get(element.transformInstID);
		if (transform.sizeType == UiTransformSizeType::Content)
		{
			switch (element.type)
			{
			case UiElementType::Image:
			{
				const UiImage &image = this->images.get(element.imageInstID);
				const std::optional<Int2> imageDims = renderer.tryGetUiTextureDims(image.textureID);
				DebugAssert(imageDims.has_value());
				transform.size = *imageDims;
				break;
			}
			case UiElementType::TextBox:
			{
				const UiTextBox &textBox = this->textBoxes.get(element.textBoxInstID);
				transform.size = Int2(textBox.textureWidth, textBox.textureHeight);
				break;
			}
			case UiElementType::Button:
				break;
			default:
				DebugNotImplementedMsg(std::to_string(static_cast<int>(element.type)));
				break;
			}
		}
	}

	// Update element sizes with dependency (after independent ones).
	for (UiElement &element : this->elements.values)
	{
		UiTransform &transform = this->transforms.get(element.transformInstID);
		if (transform.sizeType == UiTransformSizeType::Content)
		{
			switch (element.type)
			{
			case UiElementType::Image:
				break;
			case UiElementType::TextBox:
				break;
			case UiElementType::Button:
			{
				const UiButton &button = this->buttons.get(element.buttonInstID);
				const UiElement &contentElement = this->elements.get(button.contentElementInstID);
				const UiTransform &contentTransform = this->transforms.get(contentElement.transformInstID);
				transform.size = contentTransform.size;
				break;
			}
			default:
				DebugNotImplementedMsg(std::to_string(static_cast<int>(element.type)));
				break;
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
			const UiTextureID textBoxTextureID = textBox.textureID;
			renderElement.id = textBoxTextureID;
			break;
		}
		case UiElementType::Button:
			break;
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
