#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "ChooseNamePanel.h"
#include "CommonUiView.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"
#include "../UI/FontLibrary.h"

ChooseNamePanel::ChooseNamePanel(Game &game)
	: Panel(game) { }

bool ChooseNamePanel::init()
{
	auto &game = this->getGame();
	auto &renderer = game.renderer;

	const auto &fontLibrary = FontLibrary::getInstance();
	const std::string titleText = ChooseNameUiModel::getTitleText(game);
	const TextBoxInitInfo titleTextBoxInitInfo = ChooseNameUiView::getTitleTextBoxInitInfo(titleText, fontLibrary);
	if (!this->titleTextBox.init(titleTextBoxInitInfo, titleText, renderer))
	{
		DebugLogError("Couldn't init title text box.");
		return false;
	}

	const TextBoxInitInfo entryTextBoxInitInfo = ChooseNameUiView::getEntryTextBoxInitInfo(fontLibrary);
	if (!this->entryTextBox.init(entryTextBoxInitInfo, renderer))
	{
		DebugLogError("Couldn't init entry text box.");
		return false;
	}

	this->addInputActionListener(InputActionName::Back, ChooseNameUiController::onBackToChooseClassInputAction);
	this->addInputActionListener(InputActionName::Accept,
		[this](const InputActionCallbackValues &values)
	{
		ChooseNameUiController::onAcceptInputAction(values, this->name);
	});

	this->addInputActionListener(InputActionName::Backspace,
		[this](const InputActionCallbackValues &values)
	{
		bool dirty;
		ChooseNameUiController::onBackspaceInputAction(values, this->name, &dirty);

		if (dirty)
		{
			this->entryTextBox.setText(this->name);
		}
	});

	this->addTextInputListener([this](const std::string_view text)
	{
		bool dirty;
		ChooseNameUiController::onTextInput(text, this->name, &dirty);

		if (dirty)
		{
			this->entryTextBox.setText(this->name);
		}
	});

	auto &inputManager = game.inputManager;
	inputManager.setTextInputMode(true);

	auto &textureManager = game.textureManager;
	const UiTextureID nightSkyTextureID = CharacterCreationUiView::allocNightSkyTexture(textureManager, renderer);
	const UiTextureID parchmentTextureID = ChooseNameUiView::allocParchmentTexture(textureManager, renderer);
	this->nightSkyTextureRef.init(nightSkyTextureID, renderer);
	this->parchmentTextureRef.init(parchmentTextureID, renderer);

	this->addDrawCall(
		this->nightSkyTextureRef.get(),
		Int2::Zero,
		Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT),
		PivotType::TopLeft);
	this->addDrawCall(
		this->parchmentTextureRef.get(),
		ChooseNameUiView::getTitleTextureCenter(),
		Int2(this->parchmentTextureRef.getWidth(), this->parchmentTextureRef.getHeight()),
		PivotType::Middle);

	const Rect &titleTextBoxRect = this->titleTextBox.getRect();
	this->addDrawCall(
		this->titleTextBox.getTextureID(),
		titleTextBoxRect.getCenter(),
		titleTextBoxRect.getSize(),
		PivotType::Middle);

	// Need a texture func for the name text box due to the non-constness of the getter.
	UiDrawCallTextureFunc entryTextureFunc = [this]()
	{
		return this->entryTextBox.getTextureID();
	};

	const Rect &entryTextBoxRect = this->entryTextBox.getRect();
	this->addDrawCall(
		entryTextureFunc,
		entryTextBoxRect.getCenter(),
		entryTextBoxRect.getSize(),
		PivotType::Middle);

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), CommonUiView::DefaultCursorPivotType);

	return true;
}
