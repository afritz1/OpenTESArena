#ifndef CHOOSE_ATTRIBUTES_PANEL_H
#define CHOOSE_ATTRIBUTES_PANEL_H

#include "Panel.h"
#include "../Game/Game.h"
#include "../Stats/PrimaryAttribute.h"
#include "../UI/Button.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"
#include "../UI/TextRenderUtils.h"

class Random;

// For choosing character creation attributes and the portrait. I think it should be used for level-up
// purposes, since distributing points is basically identical to setting your character's original attributes.
// Maybe there could be a "LevelUpPanel" for that instead.
class ChooseAttributesPanel : public Panel
{
private:
	TextBox nameTextBox, raceTextBox, classTextBox;
	TextBox attributeTextBoxes[PrimaryAttributes::COUNT];
	TextBox experienceTextBox, levelTextBox;
	TextBox bonusPointsTextBox;
	Button<Game&, bool*> doneButton;
	Button<Game&, bool> portraitButton;
	Buffer<ScopedUiTextureRef> headTextureRefs;
	ScopedUiTextureRef bodyTextureRef, shirtTextureRef, pantsTextureRef, statsBgTextureRef, cursorTextureRef;
	ScopedUiTextureRef upDownTextureRef, bonusPointsTextureRef;
	Button<> upDownButtons[PrimaryAttributes::COUNT * 2];
	bool attributesAreSaved; // Whether attributes have been saved and the player portrait can now be changed.
	int bonusPoints;
	int selectedAttributeIndex;
	int lastSelectedAttributeIndex;
	int fixedArrowX;
	int arrowDrawCallIndex;

	int calculateInitialBonusPoints(Random &random) const;
	void redrawAttributeArrows();
public:
	ChooseAttributesPanel(Game &game);
	~ChooseAttributesPanel() override = default;

	bool init();

	void tick(double dt) override;
};

#endif
