#include "DialogueFunctions.h"
#include "DialogueManager.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Game/Game.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

void DialogueManager::init(Game &game)
{
	this->game = &game;
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

	for (const std::pair<const char*, DialogueFunction> &functionMapping : DialogueFunctions::FunctionMappings)
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
