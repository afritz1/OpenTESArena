#include <algorithm>

#include "UiContext.h"
#include "UiLibrary.h"

#include "components/debug/Debug.h"
#include "components/utilities/Directory.h"
#include "components/utilities/KeyValueFile.h"
#include "components/utilities/String.h"

namespace
{
	const std::string ContextSectionName = "Context";
	const std::string Keyword_ContextType = "ContextType";

	const std::string Keyword_EntryType = "Type";
	const std::string Keyword_EntryType_Image = "Image";
	const std::string Keyword_EntryType_TextBox = "TextBox";
	const std::string Keyword_EntryType_Button = "Button";
	const std::string Keyword_EntryType_InputListener = "InputListener";

	const std::string Keyword_ElementPosition = "Position";
	const std::string Keyword_ElementSize = "Size";
	const std::string Keyword_ElementPivot = "Pivot";
	const std::string Keyword_ElementDrawOrder = "DrawOrder";
	const std::string Keyword_ElementRenderSpace = "RenderSpace";

	const std::string Keyword_ImageTexture = "Texture";

	const std::string Keyword_TextBoxWorstCaseText = "WorstCaseText";
	const std::string Keyword_TextBoxText = "Text";
	const std::string Keyword_TextBoxFontName = "FontName";
	const std::string Keyword_TextBoxDefaultColor = "TextColor";
	const std::string Keyword_TextBoxAlignment = "Alignment";
	const std::string Keyword_TextBoxShadowInfo = "Shadow";
	const std::string Keyword_TextBoxLineSpacing = "LineSpacing";

	const std::string Keyword_ButtonMouseButtons = "MouseButtons";
	const std::string Keyword_ButtonCallback = "Callback";

	bool TryGetContextTypeMapping(const std::string &typeString, UiContextType *outContextType)
	{
		constexpr std::pair<const char*, UiContextType> ContextTypeNameMappings[] =
		{
			{ "Global", UiContextType::Global },
			{ "Automap", UiContextType::Automap },
			{ "CharacterCreation", UiContextType::CharacterCreation },
			{ "CharacterSheet", UiContextType::CharacterSheet },
			{ "Cinematic", UiContextType::Cinematic },
			{ "GameWorld", UiContextType::GameWorld },
			{ "Image", UiContextType::Image },
			{ "ImageSequence", UiContextType::ImageSequence },
			{ "LoadSave", UiContextType::LoadSave },
			{ "Logbook", UiContextType::Logbook },
			{ "Loot", UiContextType::Loot },
			{ "MainMenu", UiContextType::MainMenu },
			{ "MainQuestSplash", UiContextType::MainQuestSplash },
			{ "MessageBox", UiContextType::MessageBox },
			{ "Options", UiContextType::Options },
			{ "PauseMenu", UiContextType::PauseMenu },
			{ "ProvinceMap", UiContextType::ProvinceMap },
			{ "TextCinematic", UiContextType::TextCinematic },
			{ "WorldMap", UiContextType::WorldMap }
		};

		const auto mappingsBegin = std::begin(ContextTypeNameMappings);
		const auto mappingsEnd = std::end(ContextTypeNameMappings);
		const auto iter = std::find_if(mappingsBegin, mappingsEnd,
			[&typeString](const std::pair<const char*, UiContextType> &pair)
		{
			return pair.first == typeString;
		});

		if (iter == mappingsEnd)
		{
			return false;
		}

		*outContextType = iter->second;
		return true;
	}
}

UiContextDefinition::UiContextDefinition()
{
	this->type = static_cast<UiContextType>(-1);
}

bool UiLibrary::init(const char *folderPath)
{
	if (String::isNullOrEmpty(folderPath))
	{
		DebugLogError("Missing UI assets directory path.");
		return false;
	}

	const std::vector<std::string> filenames = Directory::getFilesWithExtension(folderPath, ".txt");
	if (filenames.empty())
	{
		DebugLogErrorFormat("Couldn't find UI assets in \"%s\".", folderPath);
		return false;
	}

	for (const std::string &filename : filenames)
	{
		// @todo store parsing state per filename
		// @todo get context type from UI txt file
		// @todo parse UI txt file into UI definitions

		KeyValueFile keyValueFile;
		if (!keyValueFile.init(filename.c_str()))
		{
			DebugLogErrorFormat("Couldn't read \"%s\" as key value file for UI library.", filename.c_str());
			continue;
		}

		const bool isContextSectionDefined = keyValueFile.findSection(ContextSectionName) != nullptr;
		if (!isContextSectionDefined)
		{
			DebugLogErrorFormat("Missing %s section from \"%s\".", ContextSectionName.c_str(), filename.c_str());
			continue;
		}

		UiContextDefinition contextDef;
		for (int sectionIndex = 0; sectionIndex < keyValueFile.getSectionCount(); sectionIndex++)
		{
			const KeyValueFileSection &section = keyValueFile.getSection(sectionIndex);
			const std::string &entryName = section.getName();

			for (int pairIndex = 0; pairIndex < section.getPairCount(); pairIndex++)
			{
				const std::pair<std::string, std::string> &pair = section.getPair(pairIndex);
				const std::string &key = pair.first;
				const std::string &value = pair.second;

				if (entryName == ContextSectionName)
				{
					if (key == Keyword_ContextType)
					{
						if (!TryGetContextTypeMapping(value, &contextDef.type))
						{
							DebugLogErrorFormat("Couldn't get context type from entry \"%s\".", entryName.c_str());
						}
					}
				}
				else if (key == Keyword_EntryType)
				{
					if (value == Keyword_EntryType_Image)
					{
						// @todo try check for all Image fields
						printf("%s is Image\n", entryName.c_str());
					}
					else if (value == Keyword_EntryType_TextBox)
					{
						// @todo try check for all TextBox fields
						printf("%s is TextBox\n", entryName.c_str());
					}
					else if (value == Keyword_EntryType_Button)
					{
						// @todo try check for all Button fields
						printf("%s is Button\n", entryName.c_str());
					}
					else if (value == Keyword_EntryType_InputListener)
					{
						// @todo try check for all InputListener fields
						printf("%s is InputListener\n", entryName.c_str());
					}
					else
					{
						DebugLogWarningFormat("Unrecognized type \"%s\" for entry \"%s\".", value.c_str(), entryName.c_str());
					}
				}

				// @todo if Texture has two commas then assume generated texture

			}
		}

		this->contextDefs.emplace_back(std::move(contextDef));
	}

	return true;
}

const UiContextDefinition &UiLibrary::getDefinition(UiContextType contextType) const
{
	for (const UiContextDefinition &def : this->contextDefs)
	{
		if (def.type == contextType)
		{
			return def;
		}
	}

	DebugCrashFormat("Couldn't find context definition for type %d.", contextType);
	return UiContextDefinition();
}
