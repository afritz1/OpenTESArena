#include "UiCommand.h"
#include "UiManager.h"

#include "components/debug/Debug.h"

bool UiManager::init(const char *folderPath, TextureManager &textureManager, Renderer &renderer)
{
	// @todo eventually load UI asset txt files from folderPath

	// @todo preload some global things like cursor images

	return true;
}

void UiManager::shutdown(Renderer &renderer)
{
	this->beginScopeCallbackLists.clear();
	this->updateScopeCallbackLists.clear();
	this->endScopeCallbackLists.clear();
	this->renderElementsCache.clear();
}

void UiManager::addBeginScopeCallback(UiScope scope, const UiScopeCallback &callback)
{
	auto iter = this->beginScopeCallbackLists.find(scope);
	if (iter == this->beginScopeCallbackLists.end())
	{
		iter = this->beginScopeCallbackLists.emplace(scope, std::vector<UiScopeCallback>()).first;
	}

	std::vector<UiScopeCallback> &callbacks = iter->second;
	callbacks.emplace_back(callback);
}

void UiManager::addUpdateScopeCallback(UiScope scope, const UiScopeUpdateCallback &callback)
{
	auto iter = this->updateScopeCallbackLists.find(scope);
	if (iter == this->updateScopeCallbackLists.end())
	{
		iter = this->updateScopeCallbackLists.emplace(scope, std::vector<UiScopeUpdateCallback>()).first;
	}

	std::vector<UiScopeUpdateCallback> &callbacks = iter->second;
	callbacks.emplace_back(callback);
}

void UiManager::addEndScopeCallback(UiScope scope, const UiScopeCallback &callback)
{
	auto iter = this->endScopeCallbackLists.find(scope);
	if (iter == this->endScopeCallbackLists.end())
	{
		iter = this->endScopeCallbackLists.emplace(scope, std::vector<UiScopeCallback>()).first;
	}

	std::vector<UiScopeCallback> &callbacks = iter->second;
	callbacks.emplace_back(callback);
}

void UiManager::clearScopeCallbacks(UiScope scope)
{
	this->beginScopeCallbackLists.erase(scope);
	this->updateScopeCallbackLists.erase(scope);
	this->endScopeCallbackLists.erase(scope);
}

void UiManager::beginScope(UiScope scope, Game &game)
{
	const auto activeIter = std::find(this->activeScopes.begin(), this->activeScopes.end(), scope);
	if (activeIter != this->activeScopes.end())
	{
		DebugLogErrorFormat("UI scope %d already active.", scope);
		return;
	}

	this->activeScopes.emplace_back(scope);

	const auto beginIter = this->beginScopeCallbackLists.find(scope);
	if (beginIter != this->beginScopeCallbackLists.end())
	{
		for (const UiScopeCallback &callback : beginIter->second)
		{
			callback(game);
		}
	}
}

void UiManager::endScope(UiScope scope, Game &game)
{
	const auto activeIter = std::find(this->activeScopes.begin(), this->activeScopes.end(), scope);
	if (activeIter == this->activeScopes.end())
	{
		DebugLogErrorFormat("Expected UI scope %d to be active.", scope);
		return;
	}

	const auto endIter = this->endScopeCallbackLists.find(scope);
	if (endIter != this->endScopeCallbackLists.end())
	{
		for (const UiScopeCallback &callback : endIter->second)
		{
			callback(game);
		}
	}

	this->activeScopes.erase(activeIter);
}

void UiManager::populateCommandList(UiCommandList &commandList)
{
	commandList.addElements(this->renderElementsCache);
}

void UiManager::update(double dt, Game &game)
{
	for (const UiScope scope : this->activeScopes)
	{
		const auto updateIter = this->updateScopeCallbackLists.find(scope);
		if (updateIter != this->updateScopeCallbackLists.end())
		{
			for (const UiScopeUpdateCallback &callback : updateIter->second)
			{
				callback(dt, game);
			}
		}
	}
}
