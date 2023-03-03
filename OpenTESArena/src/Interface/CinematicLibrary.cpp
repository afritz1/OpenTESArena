#include "CinematicLibrary.h"
#include "../Assets/ArenaTextureName.h"

#include "components/debug/Debug.h"

namespace
{
	const Color ColorGood(105, 174, 207);
	const Color ColorBad(251, 207, 8);
}

void CinematicLibrary::init()
{
	const std::string &animFilenameDreamGood = ArenaTextureSequenceName::Silmane;
	const std::string &animFilenameDreamBad = ArenaTextureSequenceName::Jagar;

	// Main quest intro.
	TextCinematicDefinition textCinematicDef;
	textCinematicDef.initMainQuest(1400, std::string(animFilenameDreamGood), ColorGood, 0);
	this->textDefs.push_back(std::move(textCinematicDef));

	// Death (good).
	textCinematicDef.initDeath(1402, std::string(animFilenameDreamGood), ColorGood,
		TextCinematicDefinition::DeathDefinition::Type::Good);
	this->textDefs.push_back(std::move(textCinematicDef));

	// Death (bad).
	textCinematicDef.initDeath(1403, std::string(animFilenameDreamBad), ColorBad,
		TextCinematicDefinition::DeathDefinition::Type::Bad);
	this->textDefs.push_back(std::move(textCinematicDef));
}

int CinematicLibrary::getTextDefinitionCount() const
{
	return static_cast<int>(this->textDefs.size());
}

const TextCinematicDefinition &CinematicLibrary::getTextDefinition(int index) const
{
	DebugAssertIndex(this->textDefs, index);
	return this->textDefs[index];
}

bool CinematicLibrary::findTextDefinitionIndexIf(const TextPredicate &predicate, int *outIndex) const
{
	for (int i = 0; i < static_cast<int>(this->textDefs.size()); i++)
	{
		const TextCinematicDefinition &def = this->textDefs[i];
		if (predicate(def))
		{
			*outIndex = i;
			return true;
		}
	}

	return false;
}
