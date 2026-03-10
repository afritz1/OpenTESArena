#include "FontLibrary.h"
#include "GuiUtils.h"
#include "Surface.h"
#include "TextRenderUtils.h"
#include "UiButton.h"
#include "UiCommand.h"
#include "UiContext.h"
#include "UiImage.h"
#include "UiLibrary.h"
#include "UiManager.h"
#include "UiRenderSpace.h"
#include "UiTextBox.h"
#include "../Game/Game.h"
#include "../Interface/AutomapUiState.h"
#include "../Interface/ChooseAttributesUiState.h"
#include "../Interface/ChooseClassUiState.h"
#include "../Interface/ChooseClassCreationUiState.h"
#include "../Interface/ChooseGenderUiState.h"
#include "../Interface/ChooseNameUiState.h"
#include "../Interface/ChooseRaceUiState.h"
#include "../Interface/GameWorldUiState.h"
#include "../Interface/LogbookUiState.h"
#include "../Interface/MainMenuUiState.h"
#include "../Interface/PauseMenuUiState.h"
#include "../Interface/ProvinceMapUiState.h"
#include "../Interface/WorldMapUiState.h"
#include "../Rendering/Window.h"

#include "components/debug/Debug.h"
#include "components/utilities/StringView.h"

namespace
{
	Int2 GetImageContentSize(const UiImage &image, const Renderer &renderer)
	{
		const std::optional<Int2> size = renderer.tryGetUiTextureDims(image.textureID);
		DebugAssert(size.has_value());
		return *size;
	}

	Int2 GetTextBoxContentSize(const UiTextBox &textBox)
	{
		return Int2(textBox.textureWidth, textBox.textureHeight);
	}

	Int2 GetListBoxContentSize(const UiListBox &listBox)
	{
		return Int2(listBox.textureWidth, listBox.textureHeight);
	}
}

#define REGISTER_SCOPE_CALLBACKS(contextName) \
this->setBeginContextCallback(contextName::ContextName, contextName::create); \
this->setEndContextCallback(contextName::ContextName, contextName::destroy); \
this->setUpdateContextCallback(contextName::ContextName, contextName::update);

LoadedUiTexture::LoadedUiTexture()
{
	this->textureID = -1;
}

GeneratedUiTexture::GeneratedUiTexture()
{
	this->patternType = static_cast<UiTexturePatternType>(-1);
	this->width = -1;
	this->height = -1;
	this->textureID = -1;
}

bool UiManager::init()
{
	REGISTER_SCOPE_CALLBACKS(AutomapUI);
	REGISTER_SCOPE_CALLBACKS(ChooseAttributesUI);
	REGISTER_SCOPE_CALLBACKS(ChooseClassUI);
	REGISTER_SCOPE_CALLBACKS(ChooseClassCreationUI);
	REGISTER_SCOPE_CALLBACKS(ChooseGenderUI);
	REGISTER_SCOPE_CALLBACKS(ChooseNameUI);
	REGISTER_SCOPE_CALLBACKS(ChooseRaceUI);
	REGISTER_SCOPE_CALLBACKS(GameWorldUI);
	REGISTER_SCOPE_CALLBACKS(LogbookUI);
	REGISTER_SCOPE_CALLBACKS(MainMenuUI);
	REGISTER_SCOPE_CALLBACKS(PauseMenuUI);
	REGISTER_SCOPE_CALLBACKS(ProvinceMapUI);
	REGISTER_SCOPE_CALLBACKS(WorldMapUI);
	return true;
}

void UiManager::shutdown(Renderer &renderer)
{
	this->contexts.clear();
	this->transforms.clear();
	this->elements.clear();
	this->images.clear();
	this->textBoxes.clear();
	this->listBoxes.clear();
	this->buttons.clear();

	for (LoadedUiTexture &texture : this->loadedTextures)
	{
		renderer.freeUiTexture(texture.textureID);
	}

	this->loadedTextures.clear();

	for (GeneratedUiTexture &texture : this->generatedTextures)
	{
		renderer.freeUiTexture(texture.textureID);
	}

	this->generatedTextures.clear();

	this->beginContextCallbacks.clear();
	this->updateContextCallbacks.clear();
	this->endContextCallbacks.clear();
	this->renderElementsCache.clear();
}

UiTextureID UiManager::getOrAddTexture(const TextureAsset &textureAsset, const TextureAsset &paletteAsset, TextureManager &textureManager, Renderer &renderer)
{
	for (const LoadedUiTexture &texture : this->loadedTextures)
	{
		if ((texture.textureAsset == textureAsset) && (texture.paletteAsset == paletteAsset))
		{
			return texture.textureID;
		}
	}

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteAsset, textureManager, renderer, &textureID))
	{
		DebugLogErrorFormat("Couldn't create UI texture \"%s\" with palette \"%s\".", textureAsset.filename.c_str(), paletteAsset.filename.c_str());
		return -1;
	}

	LoadedUiTexture newTexture;
	newTexture.textureAsset = textureAsset;
	newTexture.paletteAsset = paletteAsset;
	newTexture.textureID = textureID;
	this->loadedTextures.emplace_back(std::move(newTexture));

	return textureID;
}

UiTextureID UiManager::getOrAddTexture(UiTexturePatternType patternType, int width, int height, TextureManager &textureManager, Renderer &renderer)
{
	for (const GeneratedUiTexture &texture : this->generatedTextures)
	{
		if ((texture.patternType == patternType) && (texture.width == width) && (texture.height == height))
		{
			return texture.textureID;
		}
	}

	const Surface surface = TextureUtils::generate(patternType, width, height, textureManager, renderer);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTextureFromSurface(surface, textureManager, renderer, &textureID))
	{
		DebugLogErrorFormat("Couldn't generate UI texture with pattern %d and dimensions %dx%d.", patternType, width, height);
		return -1;
	}

	GeneratedUiTexture newTexture;
	newTexture.patternType = patternType;
	newTexture.width = width;
	newTexture.height = height;
	newTexture.textureID = textureID;
	this->generatedTextures.emplace_back(std::move(newTexture));

	return textureID;
}

void UiManager::setBeginContextCallback(const char *contextName, const UiContextBeginCallback &callback)
{
	const std::string contextNameStr = contextName;
	const auto iter = this->beginContextCallbacks.find(contextNameStr);
	if (iter != this->beginContextCallbacks.end())
	{
		DebugLogErrorFormat("Already set begin context callback for %s.", contextName);
		return;
	}

	this->beginContextCallbacks.emplace(contextNameStr, callback);
}

void UiManager::setEndContextCallback(const char *contextName, const UiContextEndCallback &callback)
{
	const std::string contextNameStr = contextName;
	const auto iter = this->endContextCallbacks.find(contextNameStr);
	if (iter != this->endContextCallbacks.end())
	{
		DebugLogErrorFormat("Already set end context callback for %s.", contextName);
		return;
	}

	this->endContextCallbacks.emplace(contextNameStr, callback);
}

void UiManager::setUpdateContextCallback(const char *contextName, const UiContextUpdateCallback &callback)
{
	const std::string contextNameStr = contextName;
	const auto iter = this->updateContextCallbacks.find(contextNameStr);
	if (iter != this->updateContextCallbacks.end())
	{
		DebugLogErrorFormat("Already set update context callback for %s.", contextName);
		return;
	}

	this->updateContextCallbacks.emplace(contextNameStr, callback);
}

UiContextInstanceID UiManager::getContextByName(const char *name) const
{
	if (String::isNullOrEmpty(name))
	{
		DebugLogError("Can't search for context with no name.");
		return -1;
	}

	for (const UiContextInstanceID contextInstID : this->contexts.keys)
	{
		const UiContext &context = this->contexts.get(contextInstID);
		if (StringView::equals(context.name, name))
		{
			return contextInstID;
		}
	}

	return -1;
}

UiElementInstanceID UiManager::getElementByName(const char *name) const
{
	if (String::isNullOrEmpty(name))
	{
		DebugLogError("Can't search for element with no name.");
		return -1;
	}

	for (const UiElementInstanceID elementInstID : this->elements.keys)
	{
		const UiElement &element = this->elements.get(elementInstID);
		if (StringView::equals(element.name, name))
		{
			return elementInstID;
		}
	}

	DebugLogErrorFormat("Couldn't find element \"%s\".", name);
	return -1;
}

void UiManager::setElementActive(UiElementInstanceID elementInstID, bool active)
{
	UiElement &element = this->elements.get(elementInstID);
	element.active = active;
}

std::vector<UiElementInstanceID> UiManager::getTopMostActiveElementsOfType(UiElementType elementType) const
{
	std::vector<UiElementInstanceID> activeInstIDs;

	const UiContextInstanceID topMostContextInstID = this->getTopMostActiveContext();

	for (const UiElementInstanceID instID : this->elements.keys)
	{
		const UiElement &element = this->elements.get(instID);
		if (!element.active)
		{
			continue;
		}

		if (element.type != elementType)
		{
			continue;
		}

		const UiContextInstanceID contextInstID = this->getContextByName(element.contextName);
		if (contextInstID != topMostContextInstID)
		{
			continue;
		}

		activeInstIDs.emplace_back(instID);
	}

	return activeInstIDs;
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

void UiManager::setTransformPivot(UiElementInstanceID elementInstID, UiPivotType pivotType)
{
	UiElement &element = this->elements.get(elementInstID);
	UiTransform &transform = this->transforms.get(element.transformInstID);
	transform.pivotType = pivotType;
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

UiElementInstanceID UiManager::createImage(const UiElementInitInfo &initInfo, UiTextureID textureID, UiContextInstanceID contextInstID, const Renderer &renderer)
{
	UiContext &context = this->contexts.get(contextInstID);
	const char *contextName = context.name.c_str();

	const UiElementInstanceID elementInstID = this->elements.alloc();
	if (elementInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate element for image (context %s, texture ID %d).", contextName, textureID);
		return -1;
	}

	const UiTransformInstanceID transformInstID = this->transforms.alloc();
	if (transformInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate transform for image (context %s, texture ID %d).", contextName, textureID);
		this->elements.free(elementInstID);
		return -1;
	}

	const UiImageInstanceID imageInstID = this->images.alloc();
	if (imageInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate image (context %s, texture ID %d).", contextName, textureID);
		this->transforms.free(transformInstID);
		this->elements.free(elementInstID);
		return -1;
	}

	UiImage &image = this->images.get(imageInstID);
	image.init(textureID);

	Int2 contentSize = initInfo.size;
	if (initInfo.sizeType == UiTransformSizeType::Content)
	{
		contentSize = GetImageContentSize(image, renderer);
	}

	UiTransform &transform = this->transforms.get(transformInstID);
	transform.init(initInfo.position, contentSize, initInfo.sizeType, initInfo.pivotType);

	UiElement &element = this->elements.get(elementInstID);
	element.initImage(initInfo.name.c_str(), contextName, initInfo.clipRect, initInfo.drawOrder, initInfo.renderSpace, transformInstID, imageInstID);

	context.imageElementInstIDs.emplace_back(elementInstID);

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

	const UiContextInstanceID contextInstID = this->getContextByName(element->contextName);
	if (contextInstID >= 0)
	{
		UiContext &context = this->contexts.get(contextInstID);
		const auto iter = std::find(context.imageElementInstIDs.begin(), context.imageElementInstIDs.end(), elementInstID);
		if (iter != context.imageElementInstIDs.end())
		{
			context.imageElementInstIDs.erase(iter);
		}
	}

	this->images.free(element->imageInstID);
	this->transforms.free(element->transformInstID);
	this->elements.free(elementInstID);
}

UiElementInstanceID UiManager::createTextBox(const UiElementInitInfo &initInfo, const UiTextBoxInitInfo &textBoxInitInfo,
	UiContextInstanceID contextInstID, Renderer &renderer)
{
	UiContext &context = this->contexts.get(contextInstID);
	const char *contextName = context.name.c_str();

	const UiElementInstanceID elementInstID = this->elements.alloc();
	if (elementInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate element for text box (context %s).", contextName);
		return -1;
	}

	const UiTransformInstanceID transformInstID = this->transforms.alloc();
	if (transformInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate transform for text box (context %s).", contextName);
		this->elements.free(elementInstID);
		return -1;
	}

	const UiTextBoxInstanceID textBoxInstID = this->textBoxes.alloc();
	if (textBoxInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate text box (context %s).", contextName);
		this->transforms.free(transformInstID);
		this->elements.free(elementInstID);
		return -1;
	}

	const FontLibrary &fontLibrary = FontLibrary::getInstance();
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(textBoxInitInfo.fontName.c_str(), &fontDefIndex))
	{
		DebugLogErrorFormat("Couldn't get font definition index for \"%s\".", textBoxInitInfo.fontName.c_str());
		this->textBoxes.free(textBoxInstID);
		this->transforms.free(transformInstID);
		this->elements.free(elementInstID);
		return -1;
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);

	TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(textBoxInitInfo.text, fontDef, textBoxInitInfo.shadowInfo, textBoxInitInfo.lineSpacing);
	if (!textBoxInitInfo.worstCaseText.empty())
	{
		const TextRenderTextureGenInfo worstCaseTextureGenInfo = TextRenderUtils::makeTextureGenInfo(textBoxInitInfo.worstCaseText, fontDef, textBoxInitInfo.shadowInfo, textBoxInitInfo.lineSpacing);

		if ((textureGenInfo.width > worstCaseTextureGenInfo.width) || (textureGenInfo.height > worstCaseTextureGenInfo.height))
		{
			DebugLogWarningFormat("Text box texture %dx%d is bigger than worst case text %dx%d, worst case text should be lengthened (text: %s).",
				textureGenInfo.width, textureGenInfo.height, worstCaseTextureGenInfo.width, worstCaseTextureGenInfo.height, textBoxInitInfo.text.c_str());
		}

		textureGenInfo.width = std::max(textureGenInfo.width, worstCaseTextureGenInfo.width);
		textureGenInfo.height = std::max(textureGenInfo.height, worstCaseTextureGenInfo.height);
	}

	const UiTextureID textBoxTextureID = renderer.createUiTexture(textureGenInfo.width, textureGenInfo.height);

	UiTextBox &textBox = this->textBoxes.get(textBoxInstID);
	textBox.init(textBoxTextureID, textureGenInfo.width, textureGenInfo.height, fontDefIndex, textBoxInitInfo.defaultColor, textBoxInitInfo.alignment, textBoxInitInfo.shadowInfo, textBoxInitInfo.lineSpacing);
	textBox.text = textBoxInitInfo.text;

	Int2 contentSize = initInfo.size;
	if (initInfo.sizeType == UiTransformSizeType::Content)
	{
		contentSize = GetTextBoxContentSize(textBox);
	}

	UiTransform &transform = this->transforms.get(transformInstID);
	transform.init(initInfo.position, contentSize, initInfo.sizeType, initInfo.pivotType);

	UiElement &element = this->elements.get(elementInstID);
	element.initTextBox(initInfo.name.c_str(), contextName, initInfo.clipRect, initInfo.drawOrder, initInfo.renderSpace, transformInstID, textBoxInstID);

	context.textBoxElementInstIDs.emplace_back(elementInstID);

	return elementInstID;
}

std::string UiManager::getTextBoxText(UiElementInstanceID elementInstID) const
{
	const UiElement &element = this->elements.get(elementInstID);

	DebugAssert(element.type == UiElementType::TextBox);
	const UiTextBox &textBox = this->textBoxes.get(element.textBoxInstID);
	return textBox.text;
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

	const UiContextInstanceID contextInstID = this->getContextByName(element->contextName);
	if (contextInstID >= 0)
	{
		UiContext &context = this->contexts.get(contextInstID);
		const auto iter = std::find(context.textBoxElementInstIDs.begin(), context.textBoxElementInstIDs.end(), elementInstID);
		if (iter != context.textBoxElementInstIDs.end())
		{
			context.textBoxElementInstIDs.erase(iter);
		}
	}

	UiTextBox &textBox = this->textBoxes.get(element->textBoxInstID);
	textBox.free(renderer);

	this->textBoxes.free(element->textBoxInstID);
	this->transforms.free(element->transformInstID);
	this->elements.free(elementInstID);
}

UiElementInstanceID UiManager::createListBox(const UiElementInitInfo &initInfo, const UiListBoxInitInfo &listBoxInitInfo,
	UiContextInstanceID contextInstID, Renderer &renderer)
{
	UiContext &context = this->contexts.get(contextInstID);
	const char *contextName = context.name.c_str();

	const UiElementInstanceID elementInstID = this->elements.alloc();
	if (elementInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate element for list box (context %s).", contextName);
		return -1;
	}

	const UiTransformInstanceID transformInstID = this->transforms.alloc();
	if (transformInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate transform for list box (context %s).", contextName);
		this->elements.free(elementInstID);
		return -1;
	}

	const UiListBoxInstanceID listBoxInstID = this->listBoxes.alloc();
	if (listBoxInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate list box (context %s).", contextName);
		this->transforms.free(transformInstID);
		this->elements.free(elementInstID);
		return -1;
	}

	const FontLibrary &fontLibrary = FontLibrary::getInstance();
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(listBoxInitInfo.fontName.c_str(), &fontDefIndex))
	{
		DebugLogErrorFormat("Couldn't get font definition index for \"%s\".", listBoxInitInfo.fontName.c_str());
		this->listBoxes.free(listBoxInstID);
		this->transforms.free(transformInstID);
		this->elements.free(elementInstID);
		return -1;
	}

	const UiTextureID listBoxTextureID = renderer.createUiTexture(listBoxInitInfo.textureWidth, listBoxInitInfo.textureHeight);

	UiListBox &listBox = this->listBoxes.get(listBoxInstID);
	listBox.init(listBoxTextureID, listBoxInitInfo.textureWidth, listBoxInitInfo.textureHeight, listBoxInitInfo.itemPixelSpacing, fontDefIndex,
		listBoxInitInfo.defaultTextColor, listBoxInitInfo.scrollDeltaScale);

	Int2 contentSize = initInfo.size;
	if (initInfo.sizeType == UiTransformSizeType::Content)
	{
		contentSize = GetListBoxContentSize(listBox);
	}

	UiTransform &transform = this->transforms.get(transformInstID);
	transform.init(initInfo.position, initInfo.size, initInfo.sizeType, initInfo.pivotType);

	UiElement &element = this->elements.get(elementInstID);
	element.initListBox(initInfo.name.c_str(), contextName, initInfo.clipRect, initInfo.drawOrder, initInfo.renderSpace, transformInstID, listBoxInstID);

	context.listBoxElementInstIDs.emplace_back(elementInstID);

	return elementInstID;
}

int UiManager::getListBoxItemCount(UiElementInstanceID elementInstID) const
{
	const UiElement &element = this->elements.get(elementInstID);

	DebugAssert(element.type == UiElementType::ListBox);
	const UiListBox &listBox = this->listBoxes.get(element.listBoxInstID);
	return static_cast<int>(listBox.items.size());
}

Rect UiManager::getListBoxItemGlobalRect(UiElementInstanceID elementInstID, int itemIndex) const
{
	const UiElement &element = this->elements.get(elementInstID);
	const UiTransform &listBoxTransform = this->transforms.get(element.transformInstID);

	DebugAssert(element.type == UiElementType::ListBox);
	const UiListBox &listBox = this->listBoxes.get(element.listBoxInstID);

	const FontLibrary &fontLibrary = FontLibrary::getInstance();
	const FontDefinition &fontDef = fontLibrary.getDefinition(listBox.fontDefIndex);
	const int itemHeight = fontDef.getCharacterHeight();
	const double itemCurrentLocalY = listBox.getItemCurrentLocalY(itemIndex);
	const Rect itemLocalRect(
		0,
		static_cast<int>(itemCurrentLocalY),
		listBoxTransform.size.x,
		itemHeight);
	const Rect itemGlobalRect(
		listBoxTransform.position.x + itemLocalRect.getLeft(),
		listBoxTransform.position.y + itemLocalRect.getTop(),
		itemLocalRect.width,
		itemLocalRect.height);

	return itemGlobalRect;
}

const UiListBoxItemCallback &UiManager::getListBoxItemCallback(UiElementInstanceID elementInstID, int itemIndex) const
{
	const UiElement &element = this->elements.get(elementInstID);

	DebugAssert(element.type == UiElementType::ListBox);
	const UiListBox &listBox = this->listBoxes.get(element.listBoxInstID);
	DebugAssertIndex(listBox.items, itemIndex);
	return listBox.items[itemIndex].callback;
}

int UiManager::getListBoxHoveredItemIndex(UiElementInstanceID elementInstID, const InputManager &inputManager, const Window &window) const
{
	const UiElement &element = this->elements.get(elementInstID);

	DebugAssert(element.type == UiElementType::ListBox);
	const UiListBox &listBox = this->listBoxes.get(element.listBoxInstID);

	Int2 mousePosition = inputManager.getMousePosition();
	if (element.renderSpace == UiRenderSpace::Classic)
	{
		const Int2 classicMousePosition = window.nativeToOriginal(mousePosition);
		mousePosition = classicMousePosition;
	}

	const Rect listBoxRect = this->getTransformGlobalRect(elementInstID);
	if (!listBoxRect.contains(mousePosition))
	{
		return -1;
	}

	const int listBoxItemCount = this->getListBoxItemCount(elementInstID);
	for (int i = 0; i < listBoxItemCount; i++)
	{
		const Rect itemGlobalRect = this->getListBoxItemGlobalRect(elementInstID, i);
		if (itemGlobalRect.contains(mousePosition))
		{
			return i;
		}
	}

	return -1;
}

void UiManager::insertListBoxItem(UiElementInstanceID elementInstID, int index, UiListBoxItem &&item)
{
	UiElement &element = this->elements.get(elementInstID);

	DebugAssert(element.type == UiElementType::ListBox);
	UiListBox &listBox = this->listBoxes.get(element.listBoxInstID);

	DebugAssert(index >= 0);
	DebugAssert(index <= listBox.items.size());
	listBox.items.emplace(listBox.items.begin() + index, std::move(item));
	listBox.dirty = true;
}

void UiManager::insertBackListBoxItem(UiElementInstanceID elementInstID, UiListBoxItem &&item)
{
	UiElement &element = this->elements.get(elementInstID);

	DebugAssert(element.type == UiElementType::ListBox);
	UiListBox &listBox = this->listBoxes.get(element.listBoxInstID);
	listBox.items.emplace_back(std::move(item));
	listBox.dirty = true;
}

void UiManager::eraseListBoxItem(UiElementInstanceID elementInstID, int index)
{
	UiElement &element = this->elements.get(elementInstID);

	DebugAssert(element.type == UiElementType::ListBox);
	UiListBox &listBox = this->listBoxes.get(element.listBoxInstID);

	DebugAssert(index >= 0);
	DebugAssert(index < listBox.items.size());
	listBox.items.erase(listBox.items.begin() + index);
	listBox.dirty = true;
}

void UiManager::clearListBox(UiElementInstanceID elementInstID)
{
	UiElement &element = this->elements.get(elementInstID);

	DebugAssert(element.type == UiElementType::ListBox);
	UiListBox &listBox = this->listBoxes.get(element.listBoxInstID);
	listBox.items.clear();
	listBox.dirty = true;
}

void UiManager::scrollListBoxDown(UiElementInstanceID elementInstID)
{
	UiElement &element = this->elements.get(elementInstID);
	UiTransform &listBoxTransform = this->transforms.get(element.transformInstID);

	DebugAssert(element.type == UiElementType::ListBox);
	UiListBox &listBox = this->listBoxes.get(element.listBoxInstID);
	const int itemCount = static_cast<int>(listBox.items.size());

	const FontLibrary &fontLibrary = FontLibrary::getInstance();
	const FontDefinition &fontDef = fontLibrary.getDefinition(listBox.fontDefIndex);
	const int itemHeight = fontDef.getCharacterHeight();

	const int itemHeightSum = itemHeight * itemCount;
	const int itemPaddingSum = listBox.itemPixelSpacing * std::max(0, itemCount - 1);
	const int totalItemSizeSum = itemHeightSum + itemPaddingSum;
	const double maxScrollPixelOffset = std::max(0.0, static_cast<double>(totalItemSizeSum - listBoxTransform.size.y));
	listBox.scrollPixelOffset = std::min(listBox.scrollPixelOffset + listBox.getScrollDeltaPixels(), maxScrollPixelOffset);
	listBox.dirty = true;
}

void UiManager::scrollListBoxUp(UiElementInstanceID elementInstID)
{
	UiElement &element = this->elements.get(elementInstID);

	DebugAssert(element.type == UiElementType::ListBox);
	UiListBox &listBox = this->listBoxes.get(element.listBoxInstID);
	listBox.scrollPixelOffset = std::max(0.0, listBox.scrollPixelOffset - listBox.getScrollDeltaPixels());
	listBox.dirty = true;
}

void UiManager::freeListBox(UiElementInstanceID elementInstID, Renderer &renderer)
{
	UiElement *element = this->elements.tryGet(elementInstID);
	if (element == nullptr)
	{
		return;
	}

	DebugAssert(element->type == UiElementType::ListBox);

	const UiContextInstanceID contextInstID = this->getContextByName(element->contextName);
	if (contextInstID >= 0)
	{
		UiContext &context = this->contexts.get(contextInstID);
		const auto iter = std::find(context.listBoxElementInstIDs.begin(), context.listBoxElementInstIDs.end(), elementInstID);
		if (iter != context.listBoxElementInstIDs.end())
		{
			context.listBoxElementInstIDs.erase(iter);
		}
	}

	UiListBox &listBox = this->listBoxes.get(element->listBoxInstID);
	listBox.free(renderer);

	this->listBoxes.free(element->listBoxInstID);
	this->transforms.free(element->transformInstID);
	this->elements.free(elementInstID);
}

UiElementInstanceID UiManager::createButton(const UiElementInitInfo &initInfo, const UiButtonInitInfo &buttonInitInfo, UiContextInstanceID contextInstID)
{
	UiContext &context = this->contexts.get(contextInstID);
	const char *contextName = context.name.c_str();

	const UiElementInstanceID elementInstID = this->elements.alloc();
	if (elementInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate element for button (context %s).", contextName);
		return -1;
	}

	const UiTransformInstanceID transformInstID = this->transforms.alloc();
	if (transformInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate transform for button (context %s).", contextName);
		this->elements.free(elementInstID);
		return -1;
	}

	const UiButtonInstanceID buttonInstID = this->buttons.alloc();
	if (buttonInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate button (context %s).", contextName);
		this->transforms.free(transformInstID);
		this->elements.free(elementInstID);
		return -1;
	}

	UiButton &button = this->buttons.get(buttonInstID);
	button.init(buttonInitInfo.mouseButtonFlags, buttonInitInfo.callback, buttonInitInfo.contentElementName);

	UiTransform &transform = this->transforms.get(transformInstID);
	transform.init(initInfo.position, initInfo.size, initInfo.sizeType, initInfo.pivotType);

	UiElement &element = this->elements.get(elementInstID);
	element.initButton(initInfo.name.c_str(), contextName, initInfo.clipRect, initInfo.drawOrder, initInfo.renderSpace, transformInstID, buttonInstID);

	context.buttonElementInstIDs.emplace_back(elementInstID);

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

	const UiContextInstanceID contextInstID = this->getContextByName(element->contextName);
	if (contextInstID >= 0)
	{
		UiContext &context = this->contexts.get(contextInstID);
		const auto iter = std::find(context.buttonElementInstIDs.begin(), context.buttonElementInstIDs.end(), elementInstID);
		if (iter != context.buttonElementInstIDs.end())
		{
			context.buttonElementInstIDs.erase(iter);
		}
	}

	this->buttons.free(element->buttonInstID);
	this->transforms.free(element->transformInstID);
	this->elements.free(elementInstID);
}

void UiManager::addInputActionListener(const char *actionName, const InputActionCallback &callback, const char *contextName, InputManager &inputManager)
{
	const InputListenerID listenerID = inputManager.addInputActionListener(actionName, callback, contextName);
	const UiContextInstanceID contextInstID = this->getContextByName(contextName);
	UiContext &context = this->contexts.get(contextInstID);
	context.inputActionListenerIDs.emplace_back(listenerID);
}

void UiManager::addMouseButtonChangedListener(const MouseButtonChangedCallback &callback, const char *contextName, InputManager &inputManager)
{
	const InputListenerID listenerID = inputManager.addMouseButtonChangedListener(callback, contextName);
	const UiContextInstanceID contextInstID = this->getContextByName(contextName);
	UiContext &context = this->contexts.get(contextInstID);
	context.mouseButtonChangedListenerIDs.emplace_back(listenerID);
}

void UiManager::addMouseButtonHeldListener(const MouseButtonHeldCallback &callback, const char *contextName, InputManager &inputManager)
{
	const InputListenerID listenerID = inputManager.addMouseButtonHeldListener(callback, contextName);
	const UiContextInstanceID contextInstID = this->getContextByName(contextName);
	UiContext &context = this->contexts.get(contextInstID);
	context.mouseButtonHeldListenerIDs.emplace_back(listenerID);
}

void UiManager::addMouseScrollChangedListener(const MouseScrollChangedCallback &callback, const char *contextName, InputManager &inputManager)
{
	const InputListenerID listenerID = inputManager.addMouseScrollChangedListener(callback, contextName);
	const UiContextInstanceID contextInstID = this->getContextByName(contextName);
	UiContext &context = this->contexts.get(contextInstID);
	context.mouseScrollChangedListenerIDs.emplace_back(listenerID);
}

void UiManager::addMouseMotionListener(const MouseMotionCallback &callback, const char *contextName, InputManager &inputManager)
{
	const InputListenerID listenerID = inputManager.addMouseMotionListener(callback, contextName);
	const UiContextInstanceID contextInstID = this->getContextByName(contextName);
	UiContext &context = this->contexts.get(contextInstID);
	context.mouseMotionListenerIDs.emplace_back(listenerID);
}

void UiManager::addApplicationExitListener(const ApplicationExitCallback &callback, const char *contextName, InputManager &inputManager)
{
	const InputListenerID listenerID = inputManager.addApplicationExitListener(callback, contextName);
	const UiContextInstanceID contextInstID = this->getContextByName(contextName);
	UiContext &context = this->contexts.get(contextInstID);
	context.applicationExitListenerIDs.emplace_back(listenerID);
}

void UiManager::addWindowResizedListener(const WindowResizedCallback &callback, const char *contextName, InputManager &inputManager)
{
	const InputListenerID listenerID = inputManager.addWindowResizedListener(callback, contextName);
	const UiContextInstanceID contextInstID = this->getContextByName(contextName);
	UiContext &context = this->contexts.get(contextInstID);
	context.windowResizedListenerIDs.emplace_back(listenerID);
}

void UiManager::addRenderTargetsResetListener(const RenderTargetsResetCallback &callback, const char *contextName, InputManager &inputManager)
{
	const InputListenerID listenerID = inputManager.addRenderTargetsResetListener(callback, contextName);
	const UiContextInstanceID contextInstID = this->getContextByName(contextName);
	UiContext &context = this->contexts.get(contextInstID);
	context.renderTargetsResetListenerIDs.emplace_back(listenerID);
}

void UiManager::addTextInputListener(const TextInputCallback &callback, const char *contextName, InputManager &inputManager)
{
	const InputListenerID listenerID = inputManager.addTextInputListener(callback, contextName);
	const UiContextInstanceID contextInstID = this->getContextByName(contextName);
	UiContext &context = this->contexts.get(contextInstID);
	context.textInputListenerIDs.emplace_back(listenerID);
}

UiContextInstanceID UiManager::createContext(const UiContextInitInfo &initInfo)
{
	if (initInfo.name.empty())
	{
		DebugLogError("Can't create context with no name.");
		return -1;
	}

	if (initInfo.drawOrder < 0)
	{
		DebugLogErrorFormat("Can't create context with negative draw order %d.", initInfo.drawOrder);
		return -1;
	}

	const UiContextInstanceID contextInstID = this->contexts.alloc();
	if (contextInstID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate context for %s.", initInfo.name.c_str());
		return -1;
	}

	UiContext &context = this->contexts.get(contextInstID);
	context.name = initInfo.name;
	context.drawOrder = initInfo.drawOrder;
	context.enabled = true;

	return contextInstID;
}

UiContextInstanceID UiManager::createContext(const UiContextDefinition &contextDef, InputManager &inputManager, TextureManager &textureManager, Renderer &renderer)
{
	UiContextInitInfo contextInitInfo;
	contextInitInfo.name = contextDef.name;
	contextInitInfo.drawOrder = 0; // Always first when coming from UI asset (no popups in assets yet).

	UiContextInstanceID contextInstID = this->createContext(contextInitInfo);
	if (contextInstID < 0)
	{
		return -1;
	}

	auto elementDefToInitInfo = [](const UiElementDefinition &def)
	{
		UiElementInitInfo initInfo;
		initInfo.name = def.name;
		initInfo.position = def.position;
		initInfo.sizeType = def.sizeType;
		initInfo.size = def.size;
		initInfo.pivotType = def.pivotType;
		initInfo.clipRect = def.clipRect;
		initInfo.drawOrder = def.drawOrder;
		initInfo.renderSpace = def.renderSpace;
		return initInfo;
	};

	auto textBoxDefToInitInfo = [](const UiTextBoxDefinition &def)
	{
		UiTextBoxInitInfo initInfo;
		initInfo.worstCaseText = def.worstCaseText;
		initInfo.text = def.text;
		initInfo.fontName = def.fontName;
		initInfo.defaultColor = def.defaultColor;
		initInfo.alignment = def.alignment;
		initInfo.shadowInfo = def.shadowInfo;
		initInfo.lineSpacing = def.lineSpacing;
		return initInfo;
	};

	auto listBoxDefToInitInfo = [](const UiListBoxDefinition &def)
	{
		UiListBoxInitInfo initInfo;
		initInfo.textureWidth = def.textureWidth;
		initInfo.textureHeight = def.textureHeight;
		initInfo.itemPixelSpacing = def.itemPixelSpacing;
		initInfo.fontName = def.fontName;
		initInfo.defaultTextColor = def.defaultTextColor;
		initInfo.scrollDeltaScale = def.scrollDeltaScale;
		return initInfo;
	};

	auto buttonDefToInitInfo = [](const UiButtonDefinition &def)
	{
		UiButtonInitInfo initInfo;
		initInfo.mouseButtonFlags = def.buttonFlags;
		initInfo.callback = def.callback;
		initInfo.contentElementName = def.contentElementName;
		return initInfo;
	};

	for (const UiImageDefinition &imageDef : contextDef.imageDefs)
	{
		const UiElementInitInfo elementInitInfo = elementDefToInitInfo(imageDef.element);

		UiTextureID textureID = -1;
		switch (imageDef.type)
		{
		case UiImageDefinitionType::Asset:
			textureID = this->getOrAddTexture(imageDef.texture, imageDef.palette, textureManager, renderer);
			break;
		case UiImageDefinitionType::Generated:
			textureID = this->getOrAddTexture(imageDef.patternType, imageDef.generatedWidth, imageDef.generatedHeight, textureManager, renderer);
			break;
		default:
			DebugNotImplementedMsg(std::to_string(static_cast<int>(imageDef.type)));
			break;
		}

		this->createImage(elementInitInfo, textureID, contextInstID, renderer);
	}

	for (const UiTextBoxDefinition &textBoxDef : contextDef.textBoxDefs)
	{
		const UiElementInitInfo elementInitInfo = elementDefToInitInfo(textBoxDef.element);
		const UiTextBoxInitInfo textBoxInitInfo = textBoxDefToInitInfo(textBoxDef);
		this->createTextBox(elementInitInfo, textBoxInitInfo, contextInstID, renderer);
	}

	for (const UiListBoxDefinition &listBoxDef : contextDef.listBoxDefs)
	{
		const UiElementInitInfo elementInitInfo = elementDefToInitInfo(listBoxDef.element);
		const UiListBoxInitInfo listBoxInitInfo = listBoxDefToInitInfo(listBoxDef);
		this->createListBox(elementInitInfo, listBoxInitInfo, contextInstID, renderer);
	}

	for (const UiButtonDefinition &buttonDef : contextDef.buttonDefs)
	{
		const UiElementInitInfo elementInitInfo = elementDefToInitInfo(buttonDef.element);
		const UiButtonInitInfo buttonInitInfo = buttonDefToInitInfo(buttonDef);
		this->createButton(elementInitInfo, buttonInitInfo, contextInstID);
	}

	for (const UiInputListenerDefinition &inputListenerDef : contextDef.inputListenerDefs)
	{
		const char *inputActionName = inputListenerDef.inputActionName.c_str();
		const InputActionCallback inputActionCallback = inputListenerDef.callback;
		this->addInputActionListener(inputActionName, inputActionCallback, contextInitInfo.name.c_str(), inputManager);
	}

	return contextInstID;
}

bool UiManager::isContextEnabled(const char *contextName) const
{
	const UiContextInstanceID contextInstID = this->getContextByName(contextName);
	if (contextInstID < 0)
	{
		return false;
	}

	const UiContext &context = this->contexts.get(contextInstID);
	return context.enabled;
}

void UiManager::setContextEnabled(UiContextInstanceID contextInstID, bool enabled)
{
	UiContext &context = this->contexts.get(contextInstID);
	context.enabled = enabled;
}

UiContextInstanceID UiManager::getTopMostActiveContext() const
{
	UiContextInstanceID topMostContextInstID = -1;

	for (const UiContextInstanceID contextInstID : this->contexts.keys)
	{
		const UiContext &context = this->contexts.get(contextInstID);
		if (!context.enabled)
		{
			continue;
		}

		if (StringView::equals(context.name, UiLibrary::GlobalContextName))
		{
			// Global cannot block other contexts.
			continue;
		}

		if (topMostContextInstID < 0)
		{
			topMostContextInstID = contextInstID;
			continue;
		}

		const UiContext &topMostContext = this->contexts.get(topMostContextInstID);
		if (context.drawOrder > topMostContext.drawOrder)
		{
			topMostContextInstID = contextInstID;
		}
	}

	if (topMostContextInstID < 0)
	{
		topMostContextInstID = this->getContextByName(UiLibrary::GlobalContextName);
	}

	return topMostContextInstID;
}

bool UiManager::isContextTopMostActive(const char *contextName) const
{
	if (String::isNullOrEmpty(contextName))
	{
		return false;
	}

	const UiContextInstanceID contextInstID = this->getContextByName(contextName);
	const UiContext &context = this->contexts.get(contextInstID);
	if (!context.enabled)
	{
		return false;
	}

	// Global behaves as always top-most active but doesn't block the one below it.
	if (context.name == UiLibrary::GlobalContextName)
	{
		return true;
	}

	const UiContextInstanceID topMostContextInstID = this->getTopMostActiveContext();
	return contextInstID == topMostContextInstID;
}

void UiManager::disableTopMostContext()
{
	UiContextInstanceID topMostContextInstID = this->getTopMostActiveContext();
	if (topMostContextInstID < 0)
	{
		DebugLogError("No top-most context to disable.");
		return;
	}

	this->setContextEnabled(topMostContextInstID, false);
}

void UiManager::clearContextElements(UiContextInstanceID contextInstID, InputManager &inputManager, Renderer &renderer)
{
	UiContext &context = this->contexts.get(contextInstID);

	// Reverse iterate due to free functions modifying the ID lists.
	for (int i = static_cast<int>(context.imageElementInstIDs.size()) - 1; i >= 0; i--)
	{
		const UiElementInstanceID instID = context.imageElementInstIDs[i];
		this->freeImage(instID);
	}

	for (int i = static_cast<int>(context.textBoxElementInstIDs.size()) - 1; i >= 0; i--)
	{
		const UiElementInstanceID instID = context.textBoxElementInstIDs[i];
		this->freeTextBox(instID, renderer);
	}

	for (int i = static_cast<int>(context.listBoxElementInstIDs.size()) - 1; i >= 0; i--)
	{
		const UiElementInstanceID instID = context.listBoxElementInstIDs[i];
		this->freeListBox(instID, renderer);
	}

	for (int i = static_cast<int>(context.buttonElementInstIDs.size()) - 1; i >= 0; i--)
	{
		const UiElementInstanceID instID = context.buttonElementInstIDs[i];
		this->freeButton(instID);
	}

	for (const InputListenerID listenerID : context.inputActionListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	context.inputActionListenerIDs.clear();

	for (const InputListenerID listenerID : context.mouseButtonChangedListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	context.mouseButtonChangedListenerIDs.clear();

	for (const InputListenerID listenerID : context.mouseButtonHeldListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	context.mouseButtonHeldListenerIDs.clear();

	for (const InputListenerID listenerID : context.mouseScrollChangedListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	context.mouseScrollChangedListenerIDs.clear();

	for (const InputListenerID listenerID : context.mouseMotionListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	context.mouseMotionListenerIDs.clear();

	for (const InputListenerID listenerID : context.applicationExitListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	context.applicationExitListenerIDs.clear();

	for (const InputListenerID listenerID : context.windowResizedListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	context.windowResizedListenerIDs.clear();

	for (const InputListenerID listenerID : context.renderTargetsResetListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	context.renderTargetsResetListenerIDs.clear();

	for (const InputListenerID listenerID : context.textInputListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	context.textInputListenerIDs.clear();
}

void UiManager::freeContext(UiContextInstanceID contextInstID, InputManager &inputManager, Renderer &renderer)
{
	this->clearContextElements(contextInstID, inputManager, renderer);
	this->contexts.free(contextInstID);
}

void UiManager::beginContext(const char *contextName, Game &game)
{
	if (this->isContextEnabled(contextName))
	{
		DebugLogErrorFormat("UI context %s already enabled.", contextName);
		//return; // @temp: due to Panel ctor/dtor design and queued panel change, can't assume active one is always right; have to play safe for now.
	}

	const std::string contextNameStr = contextName;
	const auto callbackIter = this->beginContextCallbacks.find(contextNameStr);
	if (callbackIter != this->beginContextCallbacks.end())
	{
		const UiContextBeginCallback &callback = callbackIter->second;
		callback(game);
	}
}

void UiManager::endContext(const char *contextName, Game &game)
{
	const bool isEnabled = this->isContextEnabled(contextName);
	if (!isEnabled)
	{
		// @temp: due to Panel ctor/dtor design and queued panel change, can't assume active one is always right; have to play safe for now.
		//DebugLogErrorFormat("Expected UI context %d to be active.", contextType);
		//return;
	}

	const std::string contextNameStr = contextName;
	const auto callbackIter = this->endContextCallbacks.find(contextNameStr);
	if (callbackIter != this->endContextCallbacks.end())
	{
		const UiContextEndCallback &callback = callbackIter->second;
		callback();
	}

	// @todo clear loaded textures for this context

	if (isEnabled) // @temp: due to Panel ctor/dtor design and queued panel change, can't assume active one needs clearing
	{
		//this->activeContextName.clear();
	}
}

void UiManager::populateCommandList(UiCommandList &commandList)
{
	commandList.addElements(this->renderElementsCache);
}

void UiManager::update(double dt, Game &game)
{
	// Gather all visible contexts. Only the top-most context can update (plus global context).
	std::vector<UiContextInstanceID> activeContextInstIDs;
	for (const UiContextInstanceID contextInstID : this->contexts.keys)
	{
		const UiContext &context = this->contexts.get(contextInstID);
		if (context.enabled)
		{
			activeContextInstIDs.emplace_back(contextInstID);
		}
	}

	std::sort(activeContextInstIDs.begin(), activeContextInstIDs.end(),
		[this](const UiContextInstanceID a, const UiContextInstanceID b)
	{
		const UiContext &contextA = this->contexts.get(a);
		const UiContext &contextB = this->contexts.get(b);
		return contextA.drawOrder < contextB.drawOrder;
	});

	if (!activeContextInstIDs.empty())
	{
		// Update global context as it's always technically top-most, then update the one below it.
		const UiContextInstanceID globalContextInstID = activeContextInstIDs.back();
		DebugAssert(globalContextInstID == this->getContextByName(UiLibrary::GlobalContextName));

		UiContextInstanceID updatableContextInstIDs[] = { globalContextInstID, -1 };
		if (activeContextInstIDs.size() > 1)
		{
			updatableContextInstIDs[1] = activeContextInstIDs[activeContextInstIDs.size() - 2];
		}

		for (const UiContextInstanceID contextInstID : updatableContextInstIDs)
		{
			if (contextInstID < 0)
			{
				continue;
			}

			const UiContext &context = this->contexts.get(contextInstID);
			const auto updateIter = this->updateContextCallbacks.find(context.name);
			if (updateIter != this->updateContextCallbacks.end())
			{
				const UiContextUpdateCallback &updateCallback = updateIter->second;
				updateCallback(dt);
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

	// Update dirty list boxes.
	for (UiListBox &listBox : this->listBoxes.values)
	{
		if (!listBox.dirty)
		{
			continue;
		}

		const UiTextureID listBoxTextureID = listBox.textureID;
		LockedTexture lockedTexture = renderer.lockUiTexture(listBoxTextureID);
		if (!lockedTexture.isValid())
		{
			DebugLogError("Couldn't lock list box UI texture for updating.");
			return;
		}

		Span2D<uint32_t> texels = lockedTexture.getTexels32();
		texels.fill(0);

		const FontLibrary &fontLibrary = FontLibrary::getInstance();
		const FontDefinition &fontDef = fontLibrary.getDefinition(listBox.fontDefIndex);
		const int itemHeight = fontDef.getCharacterHeight();

		for (int i = 0; i < static_cast<int>(listBox.items.size()); i++)
		{
			const UiListBoxItem &item = listBox.items[i];
			const double itemCurrentLocalY = listBox.getItemCurrentLocalY(i);
			const Rect itemLocalRect(
				0,
				static_cast<int>(itemCurrentLocalY),
				listBox.textureWidth,
				itemHeight);

			const Color itemColor = item.overrideColor.value_or(listBox.defaultTextColor);
			constexpr TextRenderColorOverrideInfo *colorOverrideInfo = nullptr;
			constexpr TextRenderShadowInfo *shadowInfo = nullptr;
			TextRenderUtils::drawTextLine(item.text, fontDef, itemLocalRect.getLeft(), itemLocalRect.getTop(), itemColor, colorOverrideInfo, shadowInfo, texels);
		}

		renderer.unlockUiTexture(listBoxTextureID);
		listBox.dirty = false;
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
				transform.size = GetImageContentSize(image, renderer);
				break;
			}
			case UiElementType::TextBox:
			{
				const UiTextBox &textBox = this->textBoxes.get(element.textBoxInstID);
				transform.size = GetTextBoxContentSize(textBox);
				break;
			}
			case UiElementType::ListBox:
			{
				const UiListBox &listBox = this->listBoxes.get(element.listBoxInstID);
				transform.size = GetListBoxContentSize(listBox);
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
			case UiElementType::TextBox:
			case UiElementType::ListBox:
				break;
			case UiElementType::Button:
			{
				const UiButton &button = this->buttons.get(element.buttonInstID);
				const UiElementInstanceID contentElementInstID = this->getElementByName(button.contentElementName.c_str());
				const UiElement &contentElement = this->elements.get(contentElementInstID);
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

		if (!this->isContextEnabled(element.contextName))
		{
			continue;
		}

		elementsToDraw.emplace_back(&element);
	}

	std::sort(elementsToDraw.begin(), elementsToDraw.end(),
		[this](const UiElement *a, const UiElement *b)
	{
		const UiContextInstanceID aContextInstID = this->getContextByName(a->contextName);
		const UiContextInstanceID bContextInstID = this->getContextByName(b->contextName);
		const UiContext &aContext = this->contexts.get(aContextInstID);
		const UiContext &bContext = this->contexts.get(bContextInstID);
		if (aContext.drawOrder != bContext.drawOrder)
		{
			return aContext.drawOrder < bContext.drawOrder;
		}

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
		const Rect clipRect = element->clipRect;
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
		case UiElementType::ListBox:
		{
			const UiListBox &listBox = this->listBoxes.get(element->listBoxInstID);
			const UiTextureID listBoxTextureID = listBox.textureID;
			renderElement.id = listBoxTextureID;
			break;
		}
		case UiElementType::Button:
			break;
		default:
			DebugNotImplementedMsg(std::to_string(static_cast<int>(element->type)));
			break;
		}

		renderElement.rect = GuiUtils::makeWindowSpaceRect(position.x, position.y, size.x, size.y, transform.pivotType, renderSpace, windowDims.x, windowDims.y, letterboxRect);
		renderElement.clipRect = GuiUtils::makeWindowSpaceRect(clipRect.x, clipRect.y, clipRect.width, clipRect.height, transform.pivotType, renderSpace, windowDims.x, windowDims.y, letterboxRect);

		if (renderElement.id >= 0)
		{
			this->renderElementsCache.emplace_back(std::move(renderElement));
		}
	}
}
