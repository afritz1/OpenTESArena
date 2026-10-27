#include <numeric>

#include "components/utilities/String.h"

#include "CharacterCreationUiMVC.h"
#include "ChooseClassQuestionsUiState.h"
#include "ChooseClassUiState.h"
#include "ChooseNameUiState.h"
#include "CommonUiMVC.h"
#include "../Assets/ArenaSoundName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../Math/RandomUtils.h"
#include "../Stats/CharacterClassLibrary.h"

namespace
{
	static constexpr const char ContextName_InitialPopUp[] = "ChooseClassQuestionsInitialPopUp";
	static constexpr const char ContextName_SuggestedClassPopUp[] = "ChooseClassQuestionsSuggestedClassPopUp";

	static constexpr const char ElementName_QuestionsTextBox[] = "ChooseClassQuestionsTextBox";
	static constexpr const char ElementName_VfxImage[] = "ChooseClassQuestionsVfxImage";

	constexpr double SCROLL_DELTA_PER_SECOND = 5.0;
}

ChooseClassQuestionsUiState::ChooseClassQuestionsUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	this->initialPopUpContextInstID = -1;
	this->suggestedClassPopUpContextInstID = -1;
	this->bgImageTextureBuilderID = -1;
	this->bgImagePaletteID = -1;
	this->bgImageTextureID = -1;
	std::fill(std::begin(this->questionIndices), std::end(this->questionIndices), -1);
	this->questionIndexCount = 0;
	this->currentQuestion = -1;
	std::fill(std::begin(this->categoryAnswerCounts), std::end(this->categoryAnswerCounts), 0);
	this->suggestedCharClassDefID = -1;
	this->currentScrollAmount = 0.0;
	this->vfxClassCategoryID = -1;
	this->vfxCurrentSeconds = 0.0;
}

void ChooseClassQuestionsUiState::init(Game &game)
{
	this->game = &game;
}

void ChooseClassQuestionsUI::create(Game &game)
{
	ChooseClassQuestionsUiState &state = ChooseClassQuestionsUI::state;
	state.init(game);

	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(ChooseClassQuestionsUI::ContextName);
	state.contextInstID = uiManager.createContext(contextDef, inputManager, textureManager, renderer);

	const std::string &bgImageTextureFilename = ArenaTextureName::CharacterCreation;
	const std::optional<TextureBuilderID> bgImageTextureBuilderID = textureManager.tryGetTextureBuilderID(bgImageTextureFilename.c_str());
	const std::optional<PaletteID> bgImagePaletteID = textureManager.tryGetPaletteID(bgImageTextureFilename.c_str());
	if (!bgImageTextureBuilderID.has_value() || !bgImagePaletteID.has_value())
	{
		DebugLogErrorFormat("Missing background texture or palette.");
		return;
	}

	const TextureBuilder &bgImageTextureBuilder = textureManager.getTextureBuilderHandle(*bgImageTextureBuilderID);
	state.bgImageTextureBuilderID = *bgImageTextureBuilderID;
	state.bgImagePaletteID = *bgImagePaletteID;
	state.bgImageTextureID = renderer.createUiTexture(bgImageTextureBuilder.width, bgImageTextureBuilder.height);

	UiElementInitInfo bgImageElementInitInfo;
	bgImageElementInitInfo.name = "ChooseClassQuestionsBackground";
	uiManager.createImage(bgImageElementInitInfo, state.bgImageTextureID, state.contextInstID, renderer);

	UiContextInitInfo initialPopUpContextInitInfo;
	initialPopUpContextInitInfo.name = ContextName_InitialPopUp;
	initialPopUpContextInitInfo.drawOrder = 1;
	state.initialPopUpContextInstID = uiManager.createContext(initialPopUpContextInitInfo);
	uiManager.setContextEnabled(state.initialPopUpContextInstID, false);

	UiContextInitInfo suggestedClassPopUpContextInitInfo;
	suggestedClassPopUpContextInitInfo.name = ContextName_SuggestedClassPopUp;
	suggestedClassPopUpContextInitInfo.drawOrder = 1;
	state.suggestedClassPopUpContextInstID = uiManager.createContext(suggestedClassPopUpContextInitInfo);
	uiManager.setContextEnabled(state.suggestedClassPopUpContextInstID, false);

	Span<const CharacterQuestion> questions = TextAssetLibrary::getInstance().questionTxt;
	Buffer<int> questionIndices(questions.getCount());
	std::iota(questionIndices.begin(), questionIndices.end(), 0);
	RandomUtils::shuffle<int>(questionIndices, game.random);

	state.questionIndexCount = std::min(questions.getCount(), ChooseClassQuestionsUiState::MAX_QUESTIONS);
	for (int i = 0; i < state.questionIndexCount; i++)
	{
		state.questionIndices[i] = questionIndices[i];
	}

	state.currentQuestion = 0;
	std::fill(std::begin(state.categoryAnswerCounts), std::end(state.categoryAnswerCounts), 0);
	state.suggestedCharClassDefID = -1;

	const CharacterQuestion &firstQuestion = questions[state.questionIndices[0]];
	ChooseClassQuestionsUI::setQuestionsTextBoxContent(firstQuestion);
	ChooseClassQuestionsUI::setQuestionsPresentationVisible(false);

	state.currentScrollAmount = 0.0;
	state.vfxClassCategoryID = -1;
	state.vfxCurrentSeconds = 0.0;

	uiManager.addMouseScrollChangedListener(ChooseClassQuestionsUI::onMouseScrollChanged, contextDef.name.c_str(), inputManager);

	game.setCursorOverride(std::nullopt);

	inputManager.setInputActionMapActive(InputActionMapName::CharacterCreation, true);

	ChooseClassQuestionsUI::updateBackgroundTexture();
	ChooseClassQuestionsUI::showInitialPopUp();
}

void ChooseClassQuestionsUI::destroy()
{
	ChooseClassQuestionsUiState &state = ChooseClassQuestionsUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	Renderer &renderer = game.renderer;

	if (state.contextInstID >= 0)
	{
		uiManager.freeContext(state.contextInstID, inputManager, renderer);
		state.contextInstID = -1;
	}

	if (state.initialPopUpContextInstID >= 0)
	{
		uiManager.freeContext(state.initialPopUpContextInstID, inputManager, renderer);
		state.initialPopUpContextInstID = -1;
	}

	if (state.suggestedClassPopUpContextInstID >= 0)
	{
		uiManager.freeContext(state.suggestedClassPopUpContextInstID, inputManager, renderer);
		state.suggestedClassPopUpContextInstID = -1;
	}

	state.bgImageTextureBuilderID = -1;
	state.bgImagePaletteID = -1;

	if (state.bgImageTextureID >= 0)
	{
		renderer.freeUiTexture(state.bgImageTextureID);
		state.bgImageTextureID = -1;
	}

	inputManager.setInputActionMapActive(InputActionMapName::CharacterCreation, false);
}

void ChooseClassQuestionsUI::update(double dt)
{
	ChooseClassQuestionsUiState &state = ChooseClassQuestionsUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const UiElementInstanceID questionsTextBoxElementInstID = uiManager.getElementByName(ElementName_QuestionsTextBox);
	const int currentScrollPixelAmount = static_cast<int>(state.currentScrollAmount);
	const Int2 questionsTextBoxPosition(ArenaRenderUtils::SCREEN_WIDTH / 2, 140 - currentScrollPixelAmount);
	uiManager.setTransformPosition(questionsTextBoxElementInstID, questionsTextBoxPosition);

	if (state.vfxClassCategoryID >= 0)
	{
		const char *vfxSequenceNames[] =
		{
			ArenaTextureSequenceName::Mage.c_str(),
			ArenaTextureSequenceName::Rogue.c_str(),
			ArenaTextureSequenceName::Warrior.c_str()
		};

		constexpr Int2 vfxSequencePositions[] =
		{
			Int2(79, 1),
			Int2(1, 1),
			Int2(110, 1)
		};

		DebugAssertIndex(vfxSequenceNames, state.vfxClassCategoryID);
		const char *vfxSequenceName = vfxSequenceNames[state.vfxClassCategoryID];

		const std::optional<TextureFileMetadataID> vfxTextureFileMetadataID = textureManager.tryGetMetadataID(vfxSequenceName);
		if (!vfxTextureFileMetadataID.has_value())
		{
			DebugLogErrorFormat("Couldn't get VFX texture sequence %s.", vfxSequenceName);
			return;
		}

		const TextureFileMetadata &vfxTextureFileMetadata = textureManager.getMetadataHandle(*vfxTextureFileMetadataID);
		const int vfxFrameCount = vfxTextureFileMetadata.getTextureCount();
		const double vfxTargetSeconds = vfxTextureFileMetadata.getSecondsPerFrame() * vfxFrameCount;
		const double vfxAnimPercent = std::clamp(state.vfxCurrentSeconds / vfxTargetSeconds, 0.0, 1.0);
		const int vfxTextureIndex = MathUtils::getRealIndex(vfxFrameCount, vfxAnimPercent);
		const TextureAsset vfxAnimTextureAsset(vfxSequenceName, vfxTextureIndex);
		const TextureAsset vfxAnimPaletteAsset(ArenaTextureName::CharacterCreation);
		const UiTextureID vfxAnimTextureID = uiManager.getOrAddTexture(vfxAnimTextureAsset, vfxAnimPaletteAsset, textureManager, renderer);

		UiElementInstanceID vfxImageElementInstID = uiManager.getElementByName(ElementName_VfxImage);
		if (vfxImageElementInstID < 0)
		{
			UiElementInitInfo vfxImageElementInitInfo;
			vfxImageElementInitInfo.name = ElementName_VfxImage;
			vfxImageElementInstID = uiManager.createImage(vfxImageElementInitInfo, vfxAnimTextureID, state.contextInstID, renderer);
		}
		else
		{
			uiManager.setElementActive(vfxImageElementInstID, true);
			uiManager.setImageTexture(vfxImageElementInstID, vfxAnimTextureID);
		}

		DebugAssertIndex(vfxSequencePositions, state.vfxClassCategoryID);
		const Int2 vfxAnimPosition = vfxSequencePositions[state.vfxClassCategoryID];
		uiManager.setTransformPosition(vfxImageElementInstID, vfxAnimPosition);

		state.vfxCurrentSeconds += dt;
		if (state.vfxCurrentSeconds >= vfxTargetSeconds)
		{
			ChooseClassQuestionsUI::onVfxFinished();
			uiManager.setElementActive(vfxImageElementInstID, false);

			state.vfxClassCategoryID = -1;
			state.vfxCurrentSeconds = 0.0;
		}
	}
}

void ChooseClassQuestionsUI::onAnswerChosen(int optionIndex)
{
	ChooseClassQuestionsUiState &state = ChooseClassQuestionsUI::state;
	if (state.vfxClassCategoryID >= 0)
	{
		// Currently animating.
		return;
	}

	Span<const CharacterQuestion> questions = TextAssetLibrary::getInstance().questionTxt;
	DebugAssertIndex(state.questionIndices, state.currentQuestion);
	const int currentQuestionIndex = state.questionIndices[state.currentQuestion];
	const CharacterQuestion &currentQuestion = questions[currentQuestionIndex];

	const CharacterQuestionChoice *choice = nullptr;
	switch (optionIndex)
	{
	case 0:
		choice = &currentQuestion.a;
		break;
	case 1:
		choice = &currentQuestion.b;
		break;
	case 2:
		choice = &currentQuestion.c;
		break;
	default:
		DebugLogErrorFormat("Invalid question choice index %d.", optionIndex);
		return;
	}

	state.categoryAnswerCounts[choice->categoryID]++;
	state.vfxClassCategoryID = choice->categoryID;

	AudioManager &audioManager = state.game->audioManager;
	audioManager.playSoundOneShot(ArenaSoundName::Burst);
}

void ChooseClassQuestionsUI::onVfxFinished()
{
	ChooseClassQuestionsUiState &state = ChooseClassQuestionsUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;	

	ChooseClassQuestionsUI::updateBackgroundTexture();

	if (state.currentQuestion < (ChooseClassQuestionsUiState::MAX_QUESTIONS - 1))
	{
		state.currentQuestion++;

		const int nextQuestionIndex = state.questionIndices[state.currentQuestion];
		Span<const CharacterQuestion> questions = TextAssetLibrary::getInstance().questionTxt;
		const CharacterQuestion &nextQuestion = questions[nextQuestionIndex];

		const UiElementInstanceID questionsTextBoxElementInstID = uiManager.getElementByName(ElementName_QuestionsTextBox);
		ChooseClassQuestionsUI::setQuestionsTextBoxContent(nextQuestion);
		state.currentScrollAmount = 0.0;
	}
	else
	{
		const CharacterClassGeneration &charClassGen = BinaryAssetLibrary::getInstance().getClassGenData();
		static_assert(std::size(state.categoryAnswerCounts) == 3);
		const int suggestedClassIndex = charClassGen.getClassIndex(state.categoryAnswerCounts[0], state.categoryAnswerCounts[1], state.categoryAnswerCounts[2]);

		const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
		state.suggestedCharClassDefID = charClassLibrary.findDefinitionIndexIf(
			[suggestedClassIndex](const CharacterClassDefinition &charClassDef)
		{
			return charClassDef.originalClassIndex == suggestedClassIndex;
		});

		if (state.suggestedCharClassDefID < 0)
		{
			DebugLogErrorFormat("Couldn't find character class definition matching original class index %d.", suggestedClassIndex);
			return;
		}

		const CharacterClassDefinition &suggestedCharClassDef = charClassLibrary.getDefinition(state.suggestedCharClassDefID);
		ChooseClassQuestionsUI::showSuggestedClassPopUp(suggestedCharClassDef);
	}
}

void ChooseClassQuestionsUI::setQuestionsPresentationVisible(bool visible)
{
	ChooseClassQuestionsUiState &state = ChooseClassQuestionsUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID textBoxElementInstID = uiManager.getElementByName(ElementName_QuestionsTextBox);
	uiManager.setElementActive(textBoxElementInstID, visible);

	// @todo parchment/scroll bars too
}

void ChooseClassQuestionsUI::setQuestionsTextBoxContent(const CharacterQuestion &question)
{
	ChooseClassQuestionsUiState &state = ChooseClassQuestionsUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;

	std::string text;
	text += question.description;
	text += question.a.text;
	text += question.b.text;
	text += question.c.text;

	const UiElementInstanceID textBoxElementInstID = uiManager.getElementByName(ElementName_QuestionsTextBox);
	uiManager.setTextBoxText(textBoxElementInstID, text.c_str());
}

void ChooseClassQuestionsUI::updateBackgroundTexture()
{
	ChooseClassQuestionsUiState &state = ChooseClassQuestionsUI::state;
	Game &game = *state.game;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	// Modify background palette based on constellation intensities.
	Palette palette = textureManager.getPaletteHandle(state.bgImagePaletteID);
	constexpr uint8_t classCategoryColors[] = { 128, 160, 192 };
	for (int i = 0; i < static_cast<int>(std::size(classCategoryColors)); i++)
	{
		const uint8_t classCategoryColor = classCategoryColors[i];
		const uint8_t replacementColor = 60 + state.categoryAnswerCounts[i];
		DebugAssertIndex(palette, classCategoryColor);
		DebugAssertIndex(palette, replacementColor);
		palette[classCategoryColor] = palette[replacementColor];
	}

	const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(state.bgImageTextureBuilderID);
	Span2D<const uint8_t> srcTexels = textureBuilder.getTexels8();

	LockedTexture lockedTexture = renderer.lockUiTexture(state.bgImageTextureID);
	Span2D<uint32_t> dstTexels = lockedTexture.getTexels32();

	std::transform(srcTexels.begin(), srcTexels.end(), dstTexels.begin(),
		[&palette](const uint8_t color)
	{
		return palette[color].toRGBA();
	});

	renderer.unlockUiTexture(state.bgImageTextureID);
}

void ChooseClassQuestionsUI::showInitialPopUp()
{
	ChooseClassQuestionsUiState &state = ChooseClassQuestionsUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	uiManager.clearContextElements(state.initialPopUpContextInstID, inputManager, renderer);

	UiElementInitInfo textBoxElementInitInfo;
	textBoxElementInitInfo.name = "ChooseClassQuestionsInitialPopUpTextBox";
	textBoxElementInitInfo.position = ChooseClassQuestionsUiView::InitialPopUpTextCenterPoint;
	textBoxElementInitInfo.pivotType = UiPivotType::Middle;
	textBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo textBoxInitInfo;
	textBoxInitInfo.text = ChooseClassQuestionsUiModel::getInitialPopUpText(game);
	textBoxInitInfo.fontName = ChooseClassQuestionsUiView::InitialPopUpFontName;
	textBoxInitInfo.defaultColor = ChooseClassQuestionsUiView::InitialPopUpColor;
	textBoxInitInfo.alignment = ChooseClassQuestionsUiView::InitialPopUpAlignment;
	textBoxInitInfo.lineSpacing = ChooseClassQuestionsUiView::InitialPopUpLineSpacing;
	const UiElementInstanceID initialPopUpTextBoxElementInstID = uiManager.createTextBox(textBoxElementInitInfo, textBoxInitInfo, state.initialPopUpContextInstID, renderer);
	const Rect initialPopUpTextBoxGlobalRect = uiManager.getTransformGlobalRect(initialPopUpTextBoxElementInstID);

	const int textureWidth = initialPopUpTextBoxGlobalRect.width + 14;
	const int textureHeight = initialPopUpTextBoxGlobalRect.height + 18;
	const UiTextureID textureID = uiManager.getOrAddTexture(ChooseClassQuestionsUiView::InitialPopUpPatternType, textureWidth, textureHeight, textureManager, renderer);

	UiElementInitInfo textureElementInitInfo;
	textureElementInitInfo.name = "ChooseClassQuestionsInitialPopUpTexture";
	textureElementInitInfo.position = ChooseClassQuestionsUiView::InitialPopUpTextureCenterPoint;
	textureElementInitInfo.pivotType = UiPivotType::Middle;
	textureElementInitInfo.drawOrder = 0;
	uiManager.createImage(textureElementInitInfo, textureID, state.initialPopUpContextInstID, renderer);

	UiElementInitInfo backButtonElementInitInfo;
	backButtonElementInitInfo.name = "ChooseClassQuestionsInitialPopUpBackButton";
	backButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	backButtonElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	backButtonElementInitInfo.drawOrder = 2;

	UiButtonInitInfo backButtonInitInfo;
	backButtonInitInfo.mouseButtonFlags = CommonUiView::PopUpMouseButtonTypeFlags;
	backButtonInitInfo.callback = ChooseClassQuestionsUI::onInitialPopUpBackButtonSelected;
	uiManager.createButton(backButtonElementInitInfo, backButtonInitInfo, state.initialPopUpContextInstID);

	uiManager.addInputActionListener(InputActionName::Back, ChooseClassQuestionsUI::onInitialPopUpBackInputAction, ContextName_InitialPopUp, inputManager);
	uiManager.setContextEnabled(state.initialPopUpContextInstID, true);
}

void ChooseClassQuestionsUI::showSuggestedClassPopUp(const CharacterClassDefinition &charClassDef)
{
	ChooseClassQuestionsUiState &state = ChooseClassQuestionsUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	uiManager.clearContextElements(state.suggestedClassPopUpContextInstID, inputManager, renderer);

	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();

	UiElementInitInfo descriptionTextBoxElementInitInfo;
	descriptionTextBoxElementInitInfo.name = "ChooseClassQuestionsSuggestedClassPopUpDescriptionTextBox";
	descriptionTextBoxElementInitInfo.position = ChooseClassQuestionsUiView::SuggestedClassPopUpTextCenterPoint;
	descriptionTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	descriptionTextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo descriptionTextBoxInitInfo;
	descriptionTextBoxInitInfo.text = ChooseClassQuestionsUiModel::getSuggestedClassPopUpText(charClassDef);
	descriptionTextBoxInitInfo.fontName = ChooseClassQuestionsUiView::SuggestedClassPopUpFontName;
	descriptionTextBoxInitInfo.defaultColor = ChooseClassQuestionsUiView::SuggestedClassPopUpTextColor;
	descriptionTextBoxInitInfo.alignment = ChooseClassQuestionsUiView::SuggestedClassPopUpTextAlignment;
	descriptionTextBoxInitInfo.lineSpacing = ChooseClassQuestionsUiView::SuggestedClassPopUpLineSpacing;
	const UiElementInstanceID descriptionTextBoxElementInstID = uiManager.createTextBox(descriptionTextBoxElementInitInfo, descriptionTextBoxInitInfo, state.suggestedClassPopUpContextInstID, renderer);
	const Rect descriptionTextBoxRect = uiManager.getTransformGlobalRect(descriptionTextBoxElementInstID);

	UiElementInitInfo descriptionImageElementInitInfo;
	descriptionImageElementInitInfo.name = "ChooseClassQuestionsSuggestedClassPopUpDescriptionImage";
	descriptionImageElementInitInfo.position = descriptionTextBoxRect.getCenter();
	descriptionImageElementInitInfo.pivotType = UiPivotType::Middle;
	descriptionImageElementInitInfo.drawOrder = 0;

	const int descriptionImageTextureWidth = descriptionTextBoxRect.width + 26;
	const int descriptionImageTextureHeight = std::max(descriptionTextBoxRect.height + 12, 40); // Parchment minimum height
	const UiTextureID descriptionImageTextureID = uiManager.getOrAddTexture(ChooseClassQuestionsUiView::InitialPopUpPatternType, descriptionImageTextureWidth, descriptionImageTextureHeight, textureManager, renderer);
	const UiElementInstanceID descriptionImageElementInstID = uiManager.createImage(descriptionImageElementInitInfo, descriptionImageTextureID, state.suggestedClassPopUpContextInstID, renderer);
	const Rect descriptionImageRect = uiManager.getTransformGlobalRect(descriptionImageElementInstID);

	const int buttonImageTextureWidth = descriptionImageTextureWidth;
	const int buttonImageTextureHeight = 40; // Parchment minimum height
	const UiTextureID buttonImageTextureID = uiManager.getOrAddTexture(ChooseClassQuestionsUiView::InitialPopUpPatternType, buttonImageTextureWidth, buttonImageTextureHeight, textureManager, renderer);

	UiElementInitInfo yesTextBoxElementInitInfo;
	yesTextBoxElementInitInfo.name = "ChooseClassQuestionsSuggestedClassPopUpYesTextBox";
	yesTextBoxElementInitInfo.position = descriptionImageRect.getCenter() + Int2(0, (descriptionImageTextureHeight / 2) + (buttonImageTextureHeight / 2) + 1);
	yesTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	yesTextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo yesTextBoxInitInfo;
	yesTextBoxInitInfo.text = exeData.travel.diseaseRiskOfDeathYes;
	yesTextBoxInitInfo.fontName = ChooseClassQuestionsUiView::SuggestedClassPopUpFontName;
	yesTextBoxInitInfo.defaultColor = ChooseClassQuestionsUiView::SuggestedClassPopUpTextColor;
	yesTextBoxInitInfo.alignment = ChooseClassQuestionsUiView::SuggestedClassPopUpTextAlignment;
	yesTextBoxInitInfo.lineSpacing = ChooseClassQuestionsUiView::SuggestedClassPopUpLineSpacing;
	const UiElementInstanceID yesTextBoxElementInstID = uiManager.createTextBox(yesTextBoxElementInitInfo, yesTextBoxInitInfo, state.suggestedClassPopUpContextInstID, renderer);
	const Rect yesTextBoxRect = uiManager.getTransformGlobalRect(yesTextBoxElementInstID);

	UiElementInitInfo yesImageElementInitInfo;
	yesImageElementInitInfo.name = "ChooseClassQuestionsSuggestedClassPopUpYesImage";
	yesImageElementInitInfo.position = yesTextBoxRect.getCenter();
	yesImageElementInitInfo.pivotType = UiPivotType::Middle;
	yesImageElementInitInfo.drawOrder = 0;
	uiManager.createImage(yesImageElementInitInfo, buttonImageTextureID, state.suggestedClassPopUpContextInstID, renderer);

	UiElementInitInfo yesButtonElementInitInfo;
	yesButtonElementInitInfo.name = "ChooseClassQuestionsSuggestedClassPopUpYesButton";
	yesButtonElementInitInfo.position = yesTextBoxRect.getCenter();
	yesButtonElementInitInfo.pivotType = UiPivotType::Middle;
	yesButtonElementInitInfo.drawOrder = 2;

	UiButtonInitInfo yesButtonInitInfo;
	yesButtonInitInfo.callback = ChooseClassQuestionsUI::onSuggestedClassPopUpYesButtonSelected;
	yesButtonInitInfo.contentElementName = yesImageElementInitInfo.name;
	uiManager.createButton(yesButtonElementInitInfo, yesButtonInitInfo, state.suggestedClassPopUpContextInstID);

	UiElementInitInfo noTextBoxElementInitInfo;
	noTextBoxElementInitInfo.name = "ChooseClassQuestionsSuggestedClassPopUpNoTextBox";
	noTextBoxElementInitInfo.position = yesTextBoxRect.getCenter() + Int2(0, buttonImageTextureHeight);
	noTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	noTextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo noTextBoxInitInfo;
	noTextBoxInitInfo.text = exeData.travel.diseaseRiskOfDeathNo;
	noTextBoxInitInfo.fontName = ChooseClassQuestionsUiView::SuggestedClassPopUpFontName;
	noTextBoxInitInfo.defaultColor = ChooseClassQuestionsUiView::SuggestedClassPopUpTextColor;
	noTextBoxInitInfo.alignment = ChooseClassQuestionsUiView::SuggestedClassPopUpTextAlignment;
	noTextBoxInitInfo.lineSpacing = ChooseClassQuestionsUiView::SuggestedClassPopUpLineSpacing;
	const UiElementInstanceID noTextBoxElementInstID = uiManager.createTextBox(noTextBoxElementInitInfo, noTextBoxInitInfo, state.suggestedClassPopUpContextInstID, renderer);
	const Rect noTextBoxRect = uiManager.getTransformGlobalRect(noTextBoxElementInstID);

	UiElementInitInfo noImageElementInitInfo;
	noImageElementInitInfo.name = "ChooseClassQuestionsSuggestedClassPopUpNoImage";
	noImageElementInitInfo.position = noTextBoxRect.getCenter();
	noImageElementInitInfo.pivotType = UiPivotType::Middle;
	noImageElementInitInfo.drawOrder = 0;
	uiManager.createImage(noImageElementInitInfo, buttonImageTextureID, state.suggestedClassPopUpContextInstID, renderer);

	UiElementInitInfo noButtonElementInitInfo;
	noButtonElementInitInfo.name = "ChooseClassQuestionsSuggestedClassPopUpNoButton";
	noButtonElementInitInfo.position = noTextBoxRect.getCenter();
	noButtonElementInitInfo.pivotType = UiPivotType::Middle;
	noButtonElementInitInfo.drawOrder = 2;

	UiButtonInitInfo noButtonInitInfo;
	noButtonInitInfo.callback = ChooseClassQuestionsUI::onSuggestedClassPopUpNoButtonSelected;
	noButtonInitInfo.contentElementName = noImageElementInitInfo.name;
	uiManager.createButton(noButtonElementInitInfo, noButtonInitInfo, state.suggestedClassPopUpContextInstID);

	uiManager.addInputActionListener(InputActionName::Back, ChooseClassQuestionsUI::onSuggestedClassPopUpBackInputAction, ContextName_SuggestedClassPopUp, inputManager);

	uiManager.setContextEnabled(state.suggestedClassPopUpContextInstID, true);
}

void ChooseClassQuestionsUI::onMouseScrollChanged(Game &game, MouseWheelScrollType type, const Int2 &position)
{
	if (type == MouseWheelScrollType::Up)
	{
		ChooseClassQuestionsUI::onScrollUpButtonSelected(MouseButtonType::Left);
	}
	else if (type == MouseWheelScrollType::Down)
	{
		ChooseClassQuestionsUI::onScrollDownButtonSelected(MouseButtonType::Left);
	}
}

// @todo replace with mouse button down
void ChooseClassQuestionsUI::onScrollDownButtonSelected(MouseButtonType mouseButtonType)
{
	ChooseClassQuestionsUiState &state = ChooseClassQuestionsUI::state;
	if (state.vfxClassCategoryID >= 0)
	{
		// Currently animating.
		return;
	}

	state.currentScrollAmount = std::min<int>(state.currentScrollAmount + SCROLL_DELTA_PER_SECOND, 400); // @todo text box height minus viewable height? 
}

void ChooseClassQuestionsUI::onScrollUpButtonSelected(MouseButtonType mouseButtonType)
{
	ChooseClassQuestionsUiState &state = ChooseClassQuestionsUI::state;
	if (state.vfxClassCategoryID >= 0)
	{
		// Currently animating.
		return;
	}

	state.currentScrollAmount = std::max<int>(state.currentScrollAmount - SCROLL_DELTA_PER_SECOND, 0);
}

void ChooseClassQuestionsUI::onBackInputAction(const InputActionCallbackValues &values)
{
	ChooseClassQuestionsUiController::onBackToChooseClassCreationInputAction(values);
}

void ChooseClassQuestionsUI::onAInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		ChooseClassQuestionsUI::onAnswerChosen(0);
	}
}

void ChooseClassQuestionsUI::onBInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		ChooseClassQuestionsUI::onAnswerChosen(1);
	}
}

void ChooseClassQuestionsUI::onCInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		ChooseClassQuestionsUI::onAnswerChosen(2);
	}
}

void ChooseClassQuestionsUI::onInitialPopUpBackButtonSelected(MouseButtonType mouseButtonType)
{
	ChooseClassQuestionsUiState &state = ChooseClassQuestionsUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	ChooseClassQuestionsUI::setQuestionsPresentationVisible(true);
}

void ChooseClassQuestionsUI::onInitialPopUpBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		ChooseClassQuestionsUI::onInitialPopUpBackButtonSelected(MouseButtonType::Left);
	}
}

void ChooseClassQuestionsUI::onSuggestedClassPopUpYesButtonSelected(MouseButtonType mouseButtonType)
{
	ChooseClassQuestionsUiState &state = ChooseClassQuestionsUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
	game.setNextContext(ChooseNameUI::ContextName);

	DebugAssert(state.suggestedCharClassDefID >= 0);
	game.charCreationState->classDefID = state.suggestedCharClassDefID;
}

void ChooseClassQuestionsUI::onSuggestedClassPopUpNoButtonSelected(MouseButtonType mouseButtonType)
{
	ChooseClassQuestionsUiState &state = ChooseClassQuestionsUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
	game.setNextContext(ChooseClassUI::ContextName);
}

void ChooseClassQuestionsUI::onSuggestedClassPopUpBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		ChooseClassQuestionsUI::onSuggestedClassPopUpNoButtonSelected(MouseButtonType::Left);
	}
}
