#include <numeric>

#include "CharacterCreationUiMVC.h"
#include "ChooseClassQuestionsUiState.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Math/RandomUtils.h"
#include "../Stats/CharacterClassLibrary.h"

namespace
{
	static constexpr const char ElementName_QuestionsTextBox[] = "ChooseClassQuestionsTextBox";

	constexpr double SCROLL_DELTA_PER_SECOND = 5.0;
}

ChooseClassQuestionsUiState::ChooseClassQuestionsUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	this->initialPopUpContextInstID = -1;
	this->decidedClassContextInstID = -1;
	std::fill(std::begin(this->questionIndices), std::end(this->questionIndices), -1);
	this->questionIndexCount = 0;
	this->currentQuestion = -1;
	this->currentScrollAmount = 0.0;
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

	const CharacterQuestion &firstQuestion = questions[questionIndices[0]];
	ChooseClassQuestionsUI::setQuestionsTextBoxContent(firstQuestion);

	state.currentScrollAmount = 0.0;

	uiManager.addMouseScrollChangedListener(ChooseClassQuestionsUI::onMouseScrollChanged, contextDef.name.c_str(), inputManager);

	game.setCursorOverride(std::nullopt);

	inputManager.setInputActionMapActive(InputActionMapName::CharacterCreation, true);
}

void ChooseClassQuestionsUI::destroy()
{
	ChooseClassQuestionsUiState &state = ChooseClassQuestionsUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;

	if (state.contextInstID >= 0)
	{
		uiManager.freeContext(state.contextInstID, game.inputManager, game.renderer);
		state.contextInstID = -1;
	}

	if (state.initialPopUpContextInstID >= 0)
	{
		uiManager.freeContext(state.initialPopUpContextInstID, game.inputManager, game.renderer);
		state.initialPopUpContextInstID = -1;
	}

	if (state.decidedClassContextInstID >= 0)
	{
		uiManager.freeContext(state.decidedClassContextInstID, game.inputManager, game.renderer);
		state.decidedClassContextInstID = -1;
	}

	InputManager &inputManager = game.inputManager;
	inputManager.setInputActionMapActive(InputActionMapName::CharacterCreation, false);
}

void ChooseClassQuestionsUI::update(double dt)
{
	ChooseClassQuestionsUiState &state = ChooseClassQuestionsUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;

	const UiElementInstanceID questionsTextBoxElementInstID = uiManager.getElementByName(ElementName_QuestionsTextBox);
	const int currentScrollPixelAmount = static_cast<int>(state.currentScrollAmount);
	const Int2 questionsTextBoxPosition(ArenaRenderUtils::SCREEN_WIDTH / 2, 140 - currentScrollPixelAmount);
	uiManager.setTransformPosition(questionsTextBoxElementInstID, questionsTextBoxPosition);

	// @todo animate vfx
}

void ChooseClassQuestionsUI::onAnswerChosen(int optionIndex)
{
	ChooseClassQuestionsUiState &state = ChooseClassQuestionsUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;

	Span<const CharacterQuestion> questions = TextAssetLibrary::getInstance().questionTxt;
	const CharacterQuestion &currentQuestion = questions[state.currentQuestion];

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

	DebugLogFormat("Selected category ID %d.", choice->categoryID);

	// @todo increment categoryID count

	if (state.currentQuestion < (ChooseClassQuestionsUiState::MAX_QUESTIONS - 1))
	{
		state.currentQuestion++;
		const int questionsIndex = state.questionIndices[state.currentQuestion];
		const CharacterQuestion &newlySelectedQuestion = questions[questionsIndex];

		const UiElementInstanceID questionsTextBoxElementInstID = uiManager.getElementByName(ElementName_QuestionsTextBox);
		ChooseClassQuestionsUI::setQuestionsTextBoxContent(newlySelectedQuestion);
		state.currentScrollAmount = 0.0;
	}
	else
	{
		// Show decided class popup
		DebugLogFormat("You would be most successful as a TBD.");

		// @todo probably need some ExeData info for classes -> A/B/C counts
	}
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

	const UiElementInstanceID questionsTextBoxElementInstID = uiManager.getElementByName(ElementName_QuestionsTextBox);
	uiManager.setTextBoxText(questionsTextBoxElementInstID, text.c_str());
}

void ChooseClassQuestionsUI::showInitialPopUp()
{
	// @todo
}

void ChooseClassQuestionsUI::showDecidedClassPopUp()
{
	// @todo
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
	state.currentScrollAmount = std::min<int>(state.currentScrollAmount + SCROLL_DELTA_PER_SECOND, 400); // @todo text box height minus viewable height? 
}

void ChooseClassQuestionsUI::onScrollUpButtonSelected(MouseButtonType mouseButtonType)
{
	ChooseClassQuestionsUiState &state = ChooseClassQuestionsUI::state;
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
