#include "SDL.h"

#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "ChooseGenderPanel.h"
#include "ChooseNamePanel.h"
#include "ChooseRacePanel.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/ExeData.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Input/InputActionName.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../UI/CursorAlignment.h"
#include "../UI/CursorData.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"

ChooseGenderPanel::ChooseGenderPanel(Game &game)
	: Panel(game) { }

bool ChooseGenderPanel::init()
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();

	this->parchment = TextureUtils::generate(
		ChooseGenderUiView::TexturePatternType,
		ChooseGenderUiView::TextureWidth,
		ChooseGenderUiView::TextureHeight,
		game.getTextureManager(),
		renderer);

	const auto &fontLibrary = game.getFontLibrary();
	const std::string titleText = ChooseGenderUiModel::getTitleText(game);
	const TextBox::InitInfo titleTextBoxInitInfo =
		ChooseGenderUiView::getTitleTextBoxInitInfo(titleText, fontLibrary);
	if (!this->titleTextBox.init(titleTextBoxInitInfo, titleText, renderer))
	{
		DebugLogError("Couldn't init title text box.");
		return false;
	}

	const std::string maleText = ChooseGenderUiModel::getMaleText(game);
	const TextBox::InitInfo maleTextBoxInitInfo =
		ChooseGenderUiView::getMaleTextBoxInitInfo(maleText, fontLibrary);
	if (!this->maleTextBox.init(maleTextBoxInitInfo, maleText, renderer))
	{
		DebugLogError("Couldn't init male text box.");
		return false;
	}

	const std::string femaleText = ChooseGenderUiModel::getFemaleText(game);
	const TextBox::InitInfo femaleTextBoxInitInfo =
		ChooseGenderUiView::getFemaleTextBoxInitInfo(femaleText, fontLibrary);
	if (!this->femaleTextBox.init(femaleTextBoxInitInfo, femaleText, renderer))
	{
		DebugLogError("Couldn't init female text box.");
		return false;
	}

	this->maleButton = Button<Game&>(
		ChooseGenderUiView::MaleButtonCenter,
		ChooseGenderUiView::MaleButtonWidth,
		ChooseGenderUiView::MaleButtonHeight,
		ChooseGenderUiController::onMaleButtonSelected);
	this->femaleButton = Button<Game&>(
		ChooseGenderUiView::FemaleButtonCenter,
		ChooseGenderUiView::FemaleButtonWidth,
		ChooseGenderUiView::FemaleButtonHeight,
		ChooseGenderUiController::onFemaleButtonSelected);

	this->addButtonProxy(MouseButtonType::Left, this->maleButton.getRect(),
		[this, &game]() { this->maleButton.click(game); });
	this->addButtonProxy(MouseButtonType::Left, this->femaleButton.getRect(),
		[this, &game]() { this->femaleButton.click(game); });

	this->addInputActionListener(InputActionName::Back, ChooseGenderUiController::onBackToChooseNameInputAction);

	return true;
}

std::optional<CursorData> ChooseGenderPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void ChooseGenderPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Draw background.
	auto &textureManager = this->getGame().getTextureManager();
	const TextureAssetReference backgroundTextureAssetRef = CharacterCreationUiView::getNightSkyTextureAssetRef();
	const std::optional<PaletteID> backgroundPaletteID = textureManager.tryGetPaletteID(backgroundTextureAssetRef);
	if (!backgroundPaletteID.has_value())
	{
		DebugLogError("Couldn't get background palette ID for \"" + backgroundTextureAssetRef.filename + "\".");
		return;
	}

	const std::optional<TextureBuilderID> backgroundTextureBuilderID = textureManager.tryGetTextureBuilderID(backgroundTextureAssetRef);
	if (!backgroundTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get background texture builder ID for \"" + backgroundTextureAssetRef.filename + "\".");
		return;
	}

	renderer.drawOriginal(*backgroundTextureBuilderID, *backgroundPaletteID, textureManager);

	// Draw parchments: title, male, and female.
	const int parchmentX = ChooseGenderUiView::getTitleTextureX(this->parchment.getWidth());
	const int parchmentY = ChooseGenderUiView::getTitleTextureY(this->parchment.getHeight());
	renderer.drawOriginal(this->parchment, parchmentX, parchmentY - 20);
	renderer.drawOriginal(this->parchment, parchmentX, parchmentY + 20);
	renderer.drawOriginal(this->parchment, parchmentX, parchmentY + 60);

	// Draw text: title, male, and female.
	const Rect &titleTextBoxRect = this->titleTextBox.getRect();
	const Rect &maleTextBoxRect = this->maleTextBox.getRect();
	const Rect &femaleTextBoxRect = this->femaleTextBox.getRect();
	renderer.drawOriginal(this->titleTextBox.getTextureID(), titleTextBoxRect.getLeft(), titleTextBoxRect.getTop());
	renderer.drawOriginal(this->maleTextBox.getTextureID(), maleTextBoxRect.getLeft(), maleTextBoxRect.getTop());
	renderer.drawOriginal(this->femaleTextBox.getTextureID(), femaleTextBoxRect.getLeft(), femaleTextBoxRect.getTop());
}
