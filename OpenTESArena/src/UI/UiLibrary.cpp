#include <algorithm>

#include "TextAlignment.h"
#include "UiContext.h"
#include "UiLibrary.h"
#include "UiPivotType.h"
#include "UiRenderSpace.h"

#include "components/debug/Debug.h"
#include "components/utilities/Directory.h"
#include "components/utilities/KeyValueFile.h"
#include "components/utilities/String.h"
#include "components/utilities/StringView.h"

namespace
{
	const std::string ContextSectionName = "Context";
	const std::string Keyword_ContextType = "ContextType";

	const std::string Keyword_EntryType = "Type";
	const std::string Keyword_EntryType_Image = "Image";
	const std::string Keyword_EntryType_TextBox = "TextBox";
	const std::string Keyword_EntryType_Button = "Button";
	const std::string Keyword_EntryType_InputListener = "InputListener";
	const std::string ValidEntryTypeKeys[] = { Keyword_EntryType_Image, Keyword_EntryType_TextBox, Keyword_EntryType_Button, Keyword_EntryType_InputListener };

	const std::string Keyword_ElementPosition = "Position";
	const std::string Keyword_ElementSize = "Size";
	const std::string Keyword_ElementPivot = "Pivot";
	const std::string Keyword_ElementDrawOrder = "DrawOrder";
	const std::string Keyword_ElementRenderSpace = "RenderSpace";
	const std::string ValidElementKeys[] = { Keyword_ElementPosition, Keyword_ElementSize, Keyword_ElementPivot, Keyword_ElementDrawOrder, Keyword_ElementRenderSpace };

	const std::string Keyword_ImageTexture = "Texture";
	const std::string Keyword_ImagePalette = "Palette";
	const std::string ValidImageKeys[] = { Keyword_ImageTexture, Keyword_ImagePalette };

	const std::string Keyword_TextBoxWorstCaseText = "WorstCaseText";
	const std::string Keyword_TextBoxText = "Text";
	const std::string Keyword_TextBoxFontName = "FontName";
	const std::string Keyword_TextBoxDefaultColor = "TextColor";
	const std::string Keyword_TextBoxAlignment = "TextAlignment";
	const std::string Keyword_TextBoxShadowInfo = "Shadow";
	const std::string Keyword_TextBoxLineSpacing = "LineSpacing";
	const std::string ValidTextBoxKeys[] = { Keyword_TextBoxWorstCaseText, Keyword_TextBoxText, Keyword_TextBoxFontName, Keyword_TextBoxDefaultColor, Keyword_TextBoxAlignment, Keyword_TextBoxShadowInfo, Keyword_TextBoxLineSpacing };

	const std::string Keyword_ButtonMouseButtons = "MouseButtons";
	const std::string Keyword_ButtonCallback = "Callback";
	const std::string ValidButtonKeys[] = { Keyword_ButtonMouseButtons, Keyword_ButtonCallback };

	const std::string Keyword_InputListenerCallback = "Callback";
	const std::string ValidInputListenerKeys[] = { Keyword_InputListenerCallback };

	enum class UiParseEntryType
	{
		None,
		Image,
		TextBox,
		Button,
		InputListener
	};

	struct UiParseState
	{
		UiParseEntryType entryType;
		UiImageDefinition imageDef;
		UiTextBoxDefinition textBoxDef;
		UiButtonDefinition buttonDef;
		UiInputListenerDefinition inputListenerDef;

		UiParseState()
		{
			this->clear();
		}

		void clear()
		{
			this->entryType = UiParseEntryType::None;
			this->imageDef.clear();
			this->textBoxDef.clear();
			this->buttonDef.clear();
			this->inputListenerDef.clear();
		}
	};

	constexpr std::pair<const char*, UiContextType> ContextTypeMappings[] =
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

	constexpr std::pair<const char*, UiPivotType> PivotTypeMappings[] =
	{
		{ "TopLeft", UiPivotType::TopLeft },
		{ "Top", UiPivotType::Top },
		{ "TopRight", UiPivotType::TopRight },
		{ "MiddleLeft", UiPivotType::MiddleLeft },
		{ "Middle", UiPivotType::Middle },
		{ "MiddleRight", UiPivotType::MiddleRight },
		{ "BottomLeft", UiPivotType::BottomLeft },
		{ "Bottom", UiPivotType::Bottom },
		{ "BottomRight", UiPivotType::BottomRight }
	};

	constexpr std::pair<const char*, UiRenderSpace> RenderSpaceMappings[] =
	{
		{ "Native", UiRenderSpace::Native },
		{ "Classic", UiRenderSpace::Classic }
	};

	constexpr std::pair<const char*, TextAlignment> TextAlignmentMappings[] =
	{
		{ "TopLeft", TextAlignment::TopLeft },
		{ "TopCenter", TextAlignment::TopCenter },
		{ "TopRight", TextAlignment::TopRight },
		{ "MiddleLeft", TextAlignment::MiddleLeft },
		{ "MiddleCenter", TextAlignment::MiddleCenter },
		{ "MiddleRight", TextAlignment::MiddleRight },
		{ "BottomLeft", TextAlignment::BottomLeft },
		{ "BottomCenter", TextAlignment::BottomCenter },
		{ "BottomRight",  TextAlignment::BottomRight }
	};

	bool TryGetContextTypeMapping(const std::string &str, UiContextType *outContextType)
	{
		const auto mappingsBegin = std::begin(ContextTypeMappings);
		const auto mappingsEnd = std::end(ContextTypeMappings);
		const auto iter = std::find_if(mappingsBegin, mappingsEnd,
			[&str](const std::pair<const char*, UiContextType> &pair)
		{
			return pair.first == str;
		});

		if (iter == mappingsEnd)
		{
			return false;
		}

		*outContextType = iter->second;
		return true;
	}

	bool TryGetPivotTypeMapping(const std::string &str, UiPivotType *outPivotType)
	{
		const auto mappingsBegin = std::begin(PivotTypeMappings);
		const auto mappingsEnd = std::end(PivotTypeMappings);
		const auto iter = std::find_if(mappingsBegin, mappingsEnd,
			[&str](const std::pair<const char*, UiPivotType> &pair)
		{
			return pair.first == str;
		});

		if (iter == mappingsEnd)
		{
			return false;
		}

		*outPivotType = iter->second;
		return true;
	}

	bool TryGetRenderSpaceMapping(const std::string &str, UiRenderSpace *outRenderSpace)
	{
		const auto mappingsBegin = std::begin(RenderSpaceMappings);
		const auto mappingsEnd = std::end(RenderSpaceMappings);
		const auto iter = std::find_if(mappingsBegin, mappingsEnd,
			[&str](const std::pair<const char*, UiRenderSpace> &pair)
		{
			return pair.first == str;
		});

		if (iter == mappingsEnd)
		{
			return false;
		}

		*outRenderSpace = iter->second;
		return true;
	}

	bool TryGetTextAlignmentMapping(const std::string &str, TextAlignment *outAlignment)
	{
		const auto mappingsBegin = std::begin(TextAlignmentMappings);
		const auto mappingsEnd = std::end(TextAlignmentMappings);
		const auto iter = std::find_if(mappingsBegin, mappingsEnd,
			[&str](const std::pair<const char*, TextAlignment> &pair)
		{
			return pair.first == str;
		});

		if (iter == mappingsEnd)
		{
			return false;
		}

		*outAlignment = iter->second;
		return true;
	}

	bool IsKeyValidForElementEntry(const std::string &key)
	{
		return std::find(std::begin(ValidElementKeys), std::end(ValidElementKeys), key) != std::end(ValidElementKeys);
	}

	bool IsKeyValidForImageEntry(const std::string &key)
	{
		return std::find(std::begin(ValidImageKeys), std::end(ValidImageKeys), key) != std::end(ValidImageKeys);
	}

	bool IsKeyValidForTextBoxEntry(const std::string &key)
	{
		return std::find(std::begin(ValidTextBoxKeys), std::end(ValidTextBoxKeys), key) != std::end(ValidTextBoxKeys);
	}

	bool IsKeyValidForButtonEntry(const std::string &key)
	{
		return std::find(std::begin(ValidButtonKeys), std::end(ValidButtonKeys), key) != std::end(ValidButtonKeys);
	}

	bool IsKeyValidForInputListenerEntry(const std::string &key)
	{
		return std::find(std::begin(ValidInputListenerKeys), std::end(ValidInputListenerKeys), key) != std::end(ValidInputListenerKeys);
	}

	bool TryParseInteger(const std::string &str, int *outValue)
	{
		try
		{
			size_t index = 0;
			*outValue = std::stoi(str, &index);
			return index == str.size();
		}
		catch (std::exception)
		{
			DebugLogErrorFormat("Couldn't parse \"%s\" as an integer.", str.c_str());
			return false;
		}
	}

	bool TryParseElementLine(const std::string &key, const std::string &value, UiElementDefinition *outElementDef)
	{
		if (!IsKeyValidForElementEntry(key))
		{
			return false;
		}

		if (key == Keyword_ElementPosition)
		{
			std::string positionTokens[2];
			if (!String::splitExpected<2>(value, ',', positionTokens))
			{
				DebugLogErrorFormat("Couldn't split position value \"%s\" into X,Y.", value.c_str());
				return false;
			}

			int positionX = 0;
			int positionY = 0;

			bool success = true;
			success &= TryParseInteger(positionTokens[0], &positionX);
			success &= TryParseInteger(positionTokens[1], &positionY);
			if (!success)
			{
				DebugLogErrorFormat("Couldn't parse position value \"%s\".", value.c_str());
				return false;
			}

			outElementDef->position.x = positionX;
			outElementDef->position.y = positionY;
		}
		else if (key == Keyword_ElementSize)
		{
			std::string sizeTokens[2];
			if (!String::splitExpected<2>(value, ',', sizeTokens))
			{
				DebugLogErrorFormat("Couldn't split size value \"%s\" into X,Y.", value.c_str());
				return false;
			}

			int sizeX = 0;
			int sizeY = 0;

			bool success = true;
			success &= TryParseInteger(sizeTokens[0], &sizeX);
			success &= TryParseInteger(sizeTokens[1], &sizeY);
			if (!success)
			{
				DebugLogErrorFormat("Couldn't parse size value \"%s\".", value.c_str());
				return false;
			}

			outElementDef->sizeType = UiTransformSizeType::Manual;
			outElementDef->size.x = sizeX;
			outElementDef->size.y = sizeY;
		}
		else if (key == Keyword_ElementPivot)
		{
			UiPivotType pivotType = static_cast<UiPivotType>(-1);
			if (!TryGetPivotTypeMapping(value, &pivotType))
			{
				DebugLogErrorFormat("Couldn't parse pivot value \"%s\".", value.c_str());
				return false;
			}

			outElementDef->pivotType = pivotType;
		}
		else if (key == Keyword_ElementDrawOrder)
		{
			int drawOrder = 0;
			if (!TryParseInteger(value, &drawOrder))
			{
				DebugLogErrorFormat("Couldn't parse draw order value \"%s\".", value.c_str());
				return false;
			}

			outElementDef->drawOrder = drawOrder;
		}
		else if (key == Keyword_ElementRenderSpace)
		{
			UiRenderSpace renderSpace = static_cast<UiRenderSpace>(-1);
			if (!TryGetRenderSpaceMapping(value, &renderSpace))
			{
				DebugLogErrorFormat("Couldn't parse render space value \"%s\".", value.c_str());
				return false;
			}

			outElementDef->renderSpace = renderSpace;
		}
		else
		{
			DebugLogErrorFormat("Unrecognized element key \"%s\".", key.c_str());
			return false;
		}

		return true;
	}

	bool TryParseImageLine(const std::string &key, const std::string &value, UiImageDefinition *outImageDef)
	{
		if (TryParseElementLine(key, value, &outImageDef->element))
		{
			return true;
		}

		if (!IsKeyValidForImageEntry(key))
		{
			return false;
		}

		if (key == Keyword_ImageTexture)
		{
			// @todo generate with TextureUtils, use UiTexturePatternType enum
			const bool isCustomGeneratedTexture = value.find(",") != std::string::npos;
			if (isCustomGeneratedTexture)
			{
				DebugLogWarningFormat("Custom generated texture \"%s\" not supported yet.", value.c_str());
			}

			outImageDef->texture = TextureAsset(std::string(value));

			if (!isCustomGeneratedTexture && outImageDef->palette.filename.empty())
			{
				outImageDef->palette = outImageDef->texture;
			}
		}
		else if (key == Keyword_ImagePalette)
		{
			outImageDef->palette = TextureAsset(std::string(value));
		}
		else
		{
			DebugLogErrorFormat("Unrecognized image key \"%s\".", key.c_str());
			return false;
		}

		return true;
	}

	bool TryParseTextBoxLine(const std::string &key, const std::string &value, UiTextBoxDefinition *outTextBoxDef)
	{
		if (TryParseElementLine(key, value, &outTextBoxDef->element))
		{
			return true;
		}

		if (!IsKeyValidForTextBoxEntry(key))
		{
			return false;
		}

		if (key == Keyword_TextBoxWorstCaseText)
		{
			outTextBoxDef->worstCaseText = value;
		}
		else if (key == Keyword_TextBoxText)
		{
			outTextBoxDef->text = value;
		}
		else if (key == Keyword_TextBoxFontName)
		{
			outTextBoxDef->fontName = value;
		}
		else if (key == Keyword_TextBoxDefaultColor)
		{
			std::string colorTokens[4];
			if (!String::splitExpected<4>(value, ',', colorTokens))
			{
				DebugLogErrorFormat("Couldn't split color value \"%s\" into r,g,b,a.", value.c_str());
				return false;
			}

			int r = 0;
			int g = 0;
			int b = 0;
			int a = 0;

			bool success = true;
			success &= TryParseInteger(colorTokens[0], &r);
			success &= TryParseInteger(colorTokens[1], &g);
			success &= TryParseInteger(colorTokens[2], &b);
			success &= TryParseInteger(colorTokens[3], &a);
			if (!success)
			{
				DebugLogErrorFormat("Couldn't parse color value \"%s\".", value.c_str());
				return false;
			}

			outTextBoxDef->defaultColor = Color(r, g, b, a);
		}
		else if (key == Keyword_TextBoxAlignment)
		{
			TextAlignment alignment = static_cast<TextAlignment>(-1);
			if (!TryGetTextAlignmentMapping(value, &alignment))
			{
				DebugLogErrorFormat("Couldn't parse render space value \"%s\".", value.c_str());
				return false;
			}

			outTextBoxDef->alignment = alignment;
		}
		else if (key == Keyword_TextBoxShadowInfo)
		{
			TextRenderShadowInfo shadowInfo;
			// @todo offsetX, offsetY, r, g, b, a
			DebugNotImplemented();
		}
		else if (key == Keyword_TextBoxLineSpacing)
		{
			int lineSpacing = 0;
			if (!TryParseInteger(value, &lineSpacing))
			{
				DebugLogErrorFormat("Couldn't parse line spacing value \"%s\".", value.c_str());
				return false;
			}

			outTextBoxDef->lineSpacing = lineSpacing;
		}
		else
		{
			DebugLogErrorFormat("Unrecognized text box key \"%s\".", key.c_str());
			return false;
		}

		return true;
	}

	bool TryParseButtonLine(const std::string &key, const std::string &value, UiButtonDefinition *outButtonDef)
	{
		if (TryParseElementLine(key, value, &outButtonDef->element))
		{
			return true;
		}

		if (!IsKeyValidForButtonEntry(key))
		{
			return false;
		}

		if (key == Keyword_ButtonMouseButtons)
		{
			DebugNotImplemented();
			// @todo button flags? is there an existing enum?
		}
		else if (key == Keyword_ButtonCallback)
		{
			outButtonDef->callback = value;
		}
		else
		{
			DebugLogErrorFormat("Unrecognized button key \"%s\".", key.c_str());
			return false;
		}

		return true;
	}

	bool TryParseInputListenerLine(const std::string &key, const std::string &value, UiInputListenerDefinition *outInputListenerDef)
	{
		if (!IsKeyValidForInputListenerEntry(key))
		{
			return false;
		}

		if (key == Keyword_InputListenerCallback)
		{
			outInputListenerDef->callback = value;
		}
		else
		{
			DebugLogErrorFormat("Unrecognized input listener key \"%s\".", key.c_str());
			return false;
		}

		return true;
	}
}

UiElementDefinition::UiElementDefinition()
{
	this->clear();
}

void UiElementDefinition::clear()
{
	this->name.clear();
	this->sizeType = UiTransformSizeType::Content;
	this->pivotType = UiPivotType::TopLeft;
	this->drawOrder = 0;
	this->renderSpace = UiRenderSpace::Classic;
}

void UiImageDefinition::clear()
{
	this->element.clear();
	this->texture = TextureAsset();
	this->palette = TextureAsset();
}

void UiTextBoxDefinition::clear()
{
	this->element.clear();
	this->worstCaseText.clear();
	this->text.clear();
	this->fontName.clear();
	this->defaultColor = Colors::Black;
	this->alignment = TextAlignment::TopLeft;
	this->shadowInfo = std::nullopt;
	this->lineSpacing = 0;
}

void UiButtonDefinition::clear()
{
	this->element.clear();
	//this->callback = nullptr;
	this->callback.clear();
}

void UiInputListenerDefinition::clear()
{
	this->name.clear();
	//this->callback = nullptr;
	this->callback.clear();
}

UiContextDefinition::UiContextDefinition()
{
	this->clear();
}

void UiContextDefinition::clear()
{
	this->type = static_cast<UiContextType>(-1);
	this->imageDefs.clear();
	this->textBoxDefs.clear();
	this->buttonDefs.clear();
	this->inputListenerDefs.clear();
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

		UiParseState parseState;
		UiContextDefinition contextDef;
		for (int sectionIndex = 0; sectionIndex < keyValueFile.getSectionCount(); sectionIndex++)
		{
			const KeyValueFileSection &section = keyValueFile.getSection(sectionIndex);
			const std::string &entryName = section.getName();

			parseState.clear();

			for (int pairIndex = 0; pairIndex < section.getPairCount(); pairIndex++)
			{
				const std::pair<std::string, std::string> &pair = section.getPair(pairIndex);
				const std::string &key = pair.first;
				const std::string &value = pair.second;

				if (key == Keyword_ContextType)
				{
					if (entryName != ContextSectionName)
					{
						DebugLogErrorFormat("Context types must only be under the \"%s\" entry.", ContextSectionName.c_str());
						continue;
					}

					UiContextType contextType;
					if (!TryGetContextTypeMapping(value, &contextType))
					{
						DebugLogErrorFormat("Couldn't get context type from entry \"%s\".", entryName.c_str());
						continue;
					}

					contextDef.type = contextType;
				}
				else if (key == Keyword_EntryType)
				{
					if (value == Keyword_EntryType_Image)
					{
						parseState.entryType = UiParseEntryType::Image;
					}
					else if (value == Keyword_EntryType_TextBox)
					{
						parseState.entryType = UiParseEntryType::TextBox;
					}
					else if (value == Keyword_EntryType_Button)
					{
						parseState.entryType = UiParseEntryType::Button;
					}
					else if (value == Keyword_EntryType_InputListener)
					{
						parseState.entryType = UiParseEntryType::InputListener;
					}
					else
					{
						DebugLogWarningFormat("Unrecognized type \"%s\" for entry \"%s\".", value.c_str(), entryName.c_str());
					}
				}
				else
				{
					switch (parseState.entryType)
					{
					case UiParseEntryType::None:
						DebugLogErrorFormat("No active parse entry type yet for entry \"%s\".", entryName.c_str());
						break;
					case UiParseEntryType::Image:
						if (!TryParseImageLine(key, value, &parseState.imageDef))
						{
							DebugLogErrorFormat("Couldn't parse \"%s\" and \"%s\" for image entry \"%s\".", key.c_str(), value.c_str(), entryName.c_str());
						}
						break;
					case UiParseEntryType::TextBox:
						if (!TryParseTextBoxLine(key, value, &parseState.textBoxDef))
						{
							DebugLogErrorFormat("Couldn't parse \"%s\" and \"%s\" for text box entry \"%s\".", key.c_str(), value.c_str(), entryName.c_str());
						}
						break;
					case UiParseEntryType::Button:
						if (!TryParseButtonLine(key, value, &parseState.buttonDef))
						{
							DebugLogErrorFormat("Couldn't parse \"%s\" and \"%s\" for button entry \"%s\".", key.c_str(), value.c_str(), entryName.c_str());
						}
						break;
					case UiParseEntryType::InputListener:
						if (!TryParseInputListenerLine(key, value, &parseState.inputListenerDef))
						{
							DebugLogErrorFormat("Couldn't parse \"%s\" and \"%s\" for input listener entry \"%s\".", key.c_str(), value.c_str(), entryName.c_str());
						}
						break;
					default:
						DebugNotImplementedMsg(std::to_string(static_cast<int>(parseState.entryType)));
						break;
					}
				}
			}

			switch (parseState.entryType)
			{
			case UiParseEntryType::None:
				break;
			case UiParseEntryType::Image:
				parseState.imageDef.element.name = entryName;
				contextDef.imageDefs.emplace_back(parseState.imageDef);
				break;
			case UiParseEntryType::TextBox:
				parseState.textBoxDef.element.name = entryName;
				contextDef.textBoxDefs.emplace_back(parseState.textBoxDef);
				break;
			case UiParseEntryType::Button:
				parseState.buttonDef.element.name = entryName;
				contextDef.buttonDefs.emplace_back(parseState.buttonDef);
				break;
			case UiParseEntryType::InputListener:
				parseState.inputListenerDef.name = entryName;
				contextDef.inputListenerDefs.emplace_back(parseState.inputListenerDef);
				break;
			default:
				DebugNotImplementedMsg(std::to_string(static_cast<int>(parseState.entryType)));
				break;
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
