#include <cstring>

#include "DialogueManager.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Game/Game.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

void DialogueManager::init(Game &game)
{
	this->game = &game;

	const Span<const std::pair<const char*, DialogueFunction>> sourceMappings = DialogueFunctions::FunctionMappings;
	this->sortedFunctionMappings.init(sourceMappings.getCount());
	std::copy(sourceMappings.begin(), sourceMappings.end(), this->sortedFunctionMappings.begin());
	std::sort(this->sortedFunctionMappings.begin(), this->sortedFunctionMappings.end(),
		[](const std::pair<const char*, DialogueFunction> &a, const std::pair<const char*, DialogueFunction> &b)
	{
		const size_t aLength = std::strlen(a.first);
		const size_t bLength = std::strlen(b.first);
		if (aLength != bLength)
		{
			return aLength > bLength;
		}

		return std::strcmp(a.first, b.first) < 0;
	});
}

void DialogueManager::beginDialogue(EntityInstanceID entityInstID)
{
	this->entityInstID = entityInstID;
}

void DialogueManager::endDialogue()
{
	this->entityInstID = -1;
}

const EntityInstance &DialogueManager::getEntityInstance() const
{
	const EntityChunkManager &entityChunkManager = this->game->sceneManager.entityChunkManager;
	return entityChunkManager.entities.get(this->entityInstID);
}

ArenaNpcPersonalityType DialogueManager::getEntityPersonalityType() const
{
	const EntityInstance &entityInst = this->getEntityInstance();
	const EntityChunkManager &entityChunkManager = this->game->sceneManager.entityChunkManager;
	const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityInst.defID);

	if (entityDef.type == EntityDefinitionType::Citizen)
	{
		return ArenaNpcPersonalityType::Citizen;
	}
	else if (entityDef.type == EntityDefinitionType::StaticNPC)
	{
		const StaticNpcEntityDefinition &staticNpcEntityDef = entityDef.staticNpc;
		DebugAssert(staticNpcEntityDef.type == StaticNpcEntityDefinitionType::General);
		return staticNpcEntityDef.general.type;
	}
	else
	{
		DebugUnhandledReturnMsg(ArenaNpcPersonalityType, std::to_string(static_cast<int>(entityDef.type)));
	}
}

bool DialogueManager::hasEntityBeenIntroduced() const
{
	const EntityInstance &entityInst = this->getEntityInstance();
	// @todo retrieve from entity instance
	return false;
}

int DialogueManager::getEntityOccupationIndex() const
{
	const EntityInstance &entityInst = this->getEntityInstance();
	// @todo store in entity instance
	ArenaRandom tempRandom(entityInst.instanceID); // Hacky randomization for now
	return tempRandom.next(100);
}

const std::string &DialogueManager::getTemplateDatEntryValueAtIndex(int entryKey, int index) const
{
	const ArenaTemplateDat &templateDat = TextAssetLibrary::getInstance().templateDat;
	const ArenaTemplateDatEntry &entry = templateDat.getEntry(entryKey);
	const Span<const std::string> entryValues = entry.values;
	return entryValues[index];
}

const std::string &DialogueManager::getRandomTemplateDatEntryValue(int entryKey) const
{
	const ArenaTemplateDat &templateDat = TextAssetLibrary::getInstance().templateDat;
	const ArenaTemplateDatEntry &entry = templateDat.getEntry(entryKey);
	const Span<const std::string> entryValues = entry.values;
	const int entryValuesRandomIndex = this->game->random.next(static_cast<int>(entry.values.size()));
	return entryValues[entryValuesRandomIndex];
}

std::string DialogueManager::getSubstitutedText(const char *text, int maxCharsPerLine) const
{
	DebugAssert(this->entityInstID >= 0);
	DebugAssert(maxCharsPerLine > 0);

	// @todo optimize these string allocations
	std::string newText = text;

	for (const std::pair<const char*, DialogueFunction> &functionMapping : this->sortedFunctionMappings)
	{
		const char *substitutionToken = functionMapping.first;
		const size_t substitutionTokenLength = std::strlen(substitutionToken);
		const DialogueFunction &substitutionFunction = functionMapping.second;
		
		size_t tokenIndex = newText.find(substitutionToken);
		while (tokenIndex != std::string::npos)
		{
			const std::string replacementString = substitutionFunction(*this->game);
			newText.replace(tokenIndex, substitutionTokenLength, replacementString);
			tokenIndex = newText.find(substitutionToken, tokenIndex + replacementString.length());
		}
	}

	return String::distributeNewlines(newText, maxCharsPerLine);
}
