#include <algorithm>

#include "CommonUiView.h"
#include "GameWorldUiController.h"
#include "GameWorldUiView.h"
#include "LootSubPanel.h"
#include "../Assets/TextureManager.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"
#include "../Items/ItemInstance.h"
#include "../Items/ItemLibrary.h"
#include "../Math/Rect.h"
#include "../Rendering/Renderer.h"
#include "../UI/CursorAlignment.h"
#include "../UI/FontLibrary.h"

namespace
{
	std::string GetItemDisplayNameWithQty(const ItemDefinition &itemDef, int stackAmount)
	{
		std::string displayName = itemDef.getDisplayName(stackAmount);
		if (itemDef.type == ItemType::Gold)
		{
			size_t goldCountIndex = displayName.find("%u");
			if (goldCountIndex != std::string::npos)
			{
				displayName.replace(goldCountIndex, 2, std::to_string(stackAmount));
			}
		}

		return displayName;
	}
}

LootSubPanel::LootSubPanel(Game &game)
	: Panel(game)
{
}

bool LootSubPanel::init(ItemInventory &itemInventory, const OnClosedFunction &onClosed)
{
	Game &game = this->getGame();

	this->closeButton = Button<Game&>(
		0,
		0,
		ArenaRenderUtils::SCREEN_WIDTH,
		ArenaRenderUtils::SCREEN_HEIGHT,
		onClosed);

	this->addButtonProxy(MouseButtonType::Right, this->closeButton.getRect(),
		[this, &game]() { this->closeButton.click(game); });

	const Rect scrollUpRect(65, 19, 9, 9);
	const Rect scrollDownRect(65, 92, 9, 9);
	this->addButtonProxy(MouseButtonType::Left, scrollUpRect,
		[this]() { this->listBox.scrollUp(); });
	this->addButtonProxy(MouseButtonType::Left, scrollDownRect,
		[this]() { this->listBox.scrollDown(); });

	this->addInputActionListener(InputActionName::Back,
		[this](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			this->closeButton.click(values.game);
		}
	});

	this->addMouseScrollChangedListener([this](Game &game, MouseWheelScrollType type, const Int2 &position)
	{
		if (type == MouseWheelScrollType::Down)
		{
			this->listBox.scrollDown();
		}
		else if (type == MouseWheelScrollType::Up)
		{
			this->listBox.scrollUp();
		}
	});

	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	UiTextureID containerInventoryTextureID = GameWorldUiView::allocContainerInventoryTexture(textureManager, renderer);
	this->textureRef.init(containerInventoryTextureID, renderer);

	UiDrawCallInitInfo containerTextureDrawCallInitInfo;
	containerTextureDrawCallInitInfo.textureID = this->textureRef.get();
	containerTextureDrawCallInitInfo.position = Int2(56, 10);
	containerTextureDrawCallInitInfo.size = this->textureRef.getDimensions();
	this->addDrawCall(containerTextureDrawCallInitInfo);

	constexpr Int2 listBoxTopLeft(85, 34);
	ListBoxProperties listBoxProperties = GameWorldUiView::getLootListBoxProperties();
	const Rect listBoxRect(listBoxTopLeft.x, listBoxTopLeft.y, listBoxProperties.textureGenInfo.width, listBoxProperties.textureGenInfo.height);
	if (!this->listBox.init(listBoxRect, listBoxProperties, renderer))
	{
		DebugLogErrorFormat("Couldn't init loot list box with %d elements.", itemInventory.getOccupiedSlotCount());
	}

	for (int i = 0; i < itemInventory.getTotalSlotCount(); i++)
	{
		const ItemInstance &itemInst = itemInventory.getSlot(i);
		if (!itemInst.isValid())
		{
			continue;
		}

		const int listBoxItemIndex = this->listBox.getCount();

		LootUiItemMapping itemMapping;
		itemMapping.inventoryItemIndex = i;
		itemMapping.listBoxItemIndex = listBoxItemIndex;
		this->itemMappings.emplace_back(itemMapping);

		const ItemLibrary &itemLibrary = ItemLibrary::getInstance();
		const ItemDefinition &itemDef = itemLibrary.getDefinition(itemInst.defID);
		const ItemDefinitionID goldItemDefID = itemLibrary.getGoldDefinitionID();
		std::string itemDisplayName = GetItemDisplayNameWithQty(itemDef, itemInventory.getCountOf(goldItemDefID));

		this->listBox.add(std::move(itemDisplayName));

		this->listBox.setCallback(listBoxItemIndex,
			[this, &game, &itemInventory, &itemLibrary, listBoxItemIndex, goldItemDefID]()
		{
			// Find which inventory item slot this list box item points to.
			int itemMappingsIndex = -1;
			for (int curItemMappingsIndex = 0; curItemMappingsIndex < static_cast<int>(this->itemMappings.size()); curItemMappingsIndex++)
			{
				if (this->itemMappings[curItemMappingsIndex].listBoxItemIndex == listBoxItemIndex)
				{
					itemMappingsIndex = curItemMappingsIndex;
					break;
				}
			}

			DebugAssert(itemMappingsIndex >= 0);
			const int mappedInventoryItemIndex = this->itemMappings[itemMappingsIndex].inventoryItemIndex;
			if (mappedInventoryItemIndex < 0)
			{
				// This list box item was emptied previously.
				return;
			}

			ItemInstance &selectedItemInst = itemInventory.getSlot(mappedInventoryItemIndex);
			const ItemDefinitionID selectedItemDefID = selectedItemInst.defID;
			DebugAssert(selectedItemDefID >= 0);
			const ItemDefinition &selectedItemDef = itemLibrary.getDefinition(selectedItemDefID);

			Player &player = game.player;
			if (selectedItemDef.type == ItemType::Gold)
			{
				player.gold += itemInventory.getCountOf(goldItemDefID);
			}
			else
			{
				ItemInventory &playerInventory = player.inventory;
				playerInventory.insert(selectedItemDefID);
			}

			selectedItemInst.defID = -1;

			if (itemInventory.getOccupiedSlotCount() == 0)
			{
				this->closeButton.click(game);
			}

			// Shift mappings forward by one.
			for (int curItemMappingsIndex = itemMappingsIndex; curItemMappingsIndex < static_cast<int>(this->itemMappings.size()); curItemMappingsIndex++)
			{
				LootUiItemMapping &curItemMapping = this->itemMappings[curItemMappingsIndex];

				const int nextItemMappingsIndex = curItemMappingsIndex + 1;
				if (nextItemMappingsIndex < static_cast<int>(itemMappings.size()))
				{
					LootUiItemMapping &nextItemMapping = this->itemMappings[nextItemMappingsIndex];
					curItemMapping.inventoryItemIndex = nextItemMapping.inventoryItemIndex;
				}
				else
				{
					curItemMapping.inventoryItemIndex = -1;
				}

				std::string newListBoxItemText;
				if (curItemMapping.inventoryItemIndex >= 0)
				{
					const ItemInstance &curItemInst = itemInventory.getSlot(curItemMapping.inventoryItemIndex);
					const ItemDefinition &curItemDef = itemLibrary.getDefinition(curItemInst.defID);
					newListBoxItemText = GetItemDisplayNameWithQty(curItemDef, 1); // @todo implement stacking
				}

				this->listBox.setText(curItemMapping.listBoxItemIndex, newListBoxItemText);
			}
		});

		auto itemRectFunc = [this, listBoxItemIndex]() { return this->listBox.getItemGlobalRect(listBoxItemIndex); };
		auto itemCallback = this->listBox.getCallback(listBoxItemIndex);
		this->addButtonProxy(MouseButtonType::Left, itemRectFunc, itemCallback, this->listBox.getRect());
	}

	UiDrawCallInitInfo listBoxDrawCallInitInfo;
	listBoxDrawCallInitInfo.textureFunc = [this]() { return this->listBox.getTextureID(); };
	listBoxDrawCallInitInfo.position = listBoxTopLeft;
	listBoxDrawCallInitInfo.size = listBoxRect.getSize();
	this->addDrawCall(listBoxDrawCallInitInfo);

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), CommonUiView::DefaultCursorPivotType);

	return true;
}
