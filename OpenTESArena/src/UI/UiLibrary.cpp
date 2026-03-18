#include <algorithm>

#include "TextAlignment.h"
#include "UiContext.h"
#include "UiLibrary.h"
#include "UiPivotType.h"
#include "UiRenderSpace.h"
#include "../Assets/TextureUtils.h"
#include "../Interface/AutomapUiState.h"
#include "../Interface/CharacterUiState.h"
#include "../Interface/CharacterEquipmentUiState.h"
#include "../Interface/ChooseAttributesUiState.h"
#include "../Interface/ChooseClassUiState.h"
#include "../Interface/ChooseClassCreationUiState.h"
#include "../Interface/ChooseGenderUiState.h"
#include "../Interface/ChooseNameUiState.h"
#include "../Interface/ChooseRaceUiState.h"
#include "../Interface/CinematicUiState.h"
#include "../Interface/GameWorldUiState.h"
#include "../Interface/LoadSaveUiState.h"
#include "../Interface/LogbookUiState.h"
#include "../Interface/MainMenuUiState.h"
#include "../Interface/MainQuestSplashUiState.h"
#include "../Interface/OptionsUiState.h"
#include "../Interface/PauseMenuUiState.h"
#include "../Interface/ProvinceMapUiState.h"
#include "../Interface/WorldMapUiState.h"

#include "components/debug/Debug.h"
#include "components/utilities/Directory.h"
#include "components/utilities/KeyValueFile.h"
#include "components/utilities/String.h"
#include "components/utilities/StringView.h"

namespace
{
	const std::string ContextSectionName = "Context";
	const std::string Keyword_ContextName = "ContextName";

	const std::string Keyword_EntryType = "Type";
	const std::string Keyword_EntryType_Image = "Image";
	const std::string Keyword_EntryType_TextBox = "TextBox";
	const std::string Keyword_EntryType_ListBox = "ListBox";
	const std::string Keyword_EntryType_Button = "Button";
	const std::string Keyword_EntryType_InputListener = "InputListener";
	const std::string ValidEntryTypeKeys[] = { Keyword_EntryType_Image, Keyword_EntryType_TextBox, Keyword_EntryType_ListBox, Keyword_EntryType_Button, Keyword_EntryType_InputListener };

	const std::string Keyword_ElementPosition = "Position";
	const std::string Keyword_ElementSize = "Size";
	const std::string Keyword_ElementPivot = "Pivot";
	const std::string Keyword_ElementClipRect = "ClipRect";
	const std::string Keyword_ElementDrawOrder = "DrawOrder";
	const std::string Keyword_ElementRenderSpace = "RenderSpace";
	const std::string ValidElementKeys[] = { Keyword_ElementPosition, Keyword_ElementSize, Keyword_ElementPivot, Keyword_ElementClipRect, Keyword_ElementDrawOrder, Keyword_ElementRenderSpace };

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

	const std::string Keyword_ListBoxTextureSize = "TextureSize";
	const std::string Keyword_ListBoxItemPixelSpacing = "ItemSpacing";
	const std::string Keyword_ListBoxFontName = "FontName";
	const std::string Keyword_ListBoxDefaultTextColor = "TextColor";
	const std::string Keyword_ListBoxMouseButtons = "MouseButtons";
	const std::string Keyword_ListBoxScrollDeltaScale = "ScrollDeltaScale";
	const std::string ValidListBoxKeys[] = { Keyword_ListBoxTextureSize, Keyword_ListBoxItemPixelSpacing, Keyword_ListBoxFontName, Keyword_ListBoxDefaultTextColor, Keyword_ListBoxMouseButtons, Keyword_ListBoxScrollDeltaScale };

	const std::string Keyword_ButtonMouseButtons = "MouseButtons";
	const std::string Keyword_ButtonCallback = "Callback";
	const std::string Keyword_ButtonContentElementName = "ContentElementName";
	const std::string ValidButtonKeys[] = { Keyword_ButtonMouseButtons, Keyword_ButtonCallback, Keyword_ButtonContentElementName };

	const std::string Keyword_InputListenerInputActionName = "InputActionName";
	const std::string Keyword_InputListenerCallback = "Callback";
	const std::string ValidInputListenerKeys[] = { Keyword_InputListenerInputActionName, Keyword_InputListenerCallback };

	enum class UiParseEntryType
	{
		None,
		Image,
		TextBox,
		ListBox,
		Button,
		InputListener
	};

	struct UiParseState
	{
		UiParseEntryType entryType;
		UiImageDefinition imageDef;
		UiTextBoxDefinition textBoxDef;
		UiListBoxDefinition listBoxDef;
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
			this->listBoxDef.clear();
			this->buttonDef.clear();
			this->inputListenerDef.clear();
		}
	};

#define DEFINE_TEXT_MAPPING(typeName, value) { #value, typeName::value }

	constexpr std::pair<const char*, UiPivotType> PivotTypeMappings[] =
	{
		DEFINE_TEXT_MAPPING(UiPivotType, TopLeft),
		DEFINE_TEXT_MAPPING(UiPivotType, Top),
		DEFINE_TEXT_MAPPING(UiPivotType, TopRight),
		DEFINE_TEXT_MAPPING(UiPivotType, MiddleLeft),
		DEFINE_TEXT_MAPPING(UiPivotType, Middle),
		DEFINE_TEXT_MAPPING(UiPivotType, MiddleRight),
		DEFINE_TEXT_MAPPING(UiPivotType, BottomLeft),
		DEFINE_TEXT_MAPPING(UiPivotType, Bottom),
		DEFINE_TEXT_MAPPING(UiPivotType, BottomRight)
	};

	constexpr std::pair<const char*, UiRenderSpace> RenderSpaceMappings[] =
	{
		DEFINE_TEXT_MAPPING(UiRenderSpace, Native),
		DEFINE_TEXT_MAPPING(UiRenderSpace, Classic)
	};

	constexpr std::pair<const char*, UiTexturePatternType> TexturePatternTypeMappings[] =
	{
		DEFINE_TEXT_MAPPING(UiTexturePatternType, Parchment),
		DEFINE_TEXT_MAPPING(UiTexturePatternType, Dark),
		DEFINE_TEXT_MAPPING(UiTexturePatternType, Custom1)
	};

	constexpr std::pair<const char*, TextAlignment> TextAlignmentMappings[] =
	{
		DEFINE_TEXT_MAPPING(TextAlignment, TopLeft),
		DEFINE_TEXT_MAPPING(TextAlignment, TopCenter),
		DEFINE_TEXT_MAPPING(TextAlignment, TopRight),
		DEFINE_TEXT_MAPPING(TextAlignment, MiddleLeft),
		DEFINE_TEXT_MAPPING(TextAlignment, MiddleCenter),
		DEFINE_TEXT_MAPPING(TextAlignment, MiddleRight),
		DEFINE_TEXT_MAPPING(TextAlignment, BottomLeft),
		DEFINE_TEXT_MAPPING(TextAlignment, BottomCenter),
		DEFINE_TEXT_MAPPING(TextAlignment, BottomRight)
	};

	constexpr std::pair<const char*, MouseButtonTypeFlags> MouseButtonTypeFlagsMappings[] =
	{
		{ "Left", MouseButtonTypeFlags(MouseButtonType::Left) },
		{ "Right", MouseButtonTypeFlags(MouseButtonType::Right) },
		{ "Both", MouseButtonTypeFlags(MouseButtonType::Left | MouseButtonType::Right) }
	};

#define DEFINE_CALLBACK_TUPLE(namespaceName) { namespaceName::NamespaceString, namespaceName::ButtonCallbacks, namespaceName::InputActionCallbacks }

	const std::tuple<const char*, Span<const std::pair<const char*, UiButtonDefinitionCallback>>, Span<const std::pair<const char*, UiInputListenerDefinitionCallback>>> ContextNamespaceCallbacks[] =
	{
		DEFINE_CALLBACK_TUPLE(AutomapUI),
		DEFINE_CALLBACK_TUPLE(CharacterUI),
		DEFINE_CALLBACK_TUPLE(CharacterEquipmentUI),
		DEFINE_CALLBACK_TUPLE(ChooseAttributesUI),
		DEFINE_CALLBACK_TUPLE(ChooseClassUI),
		DEFINE_CALLBACK_TUPLE(ChooseClassCreationUI),
		DEFINE_CALLBACK_TUPLE(ChooseGenderUI),
		DEFINE_CALLBACK_TUPLE(ChooseNameUI),
		DEFINE_CALLBACK_TUPLE(ChooseRaceUI),
		DEFINE_CALLBACK_TUPLE(CinematicUI),
		DEFINE_CALLBACK_TUPLE(GameWorldUI),
		DEFINE_CALLBACK_TUPLE(LoadSaveUI),
		DEFINE_CALLBACK_TUPLE(LogbookUI),
		DEFINE_CALLBACK_TUPLE(MainMenuUI),
		DEFINE_CALLBACK_TUPLE(MainQuestSplashUI),
		DEFINE_CALLBACK_TUPLE(OptionsUI),
		DEFINE_CALLBACK_TUPLE(PauseMenuUI),
		DEFINE_CALLBACK_TUPLE(ProvinceMapUI),
		DEFINE_CALLBACK_TUPLE(WorldMapUI)
	};

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

	bool TryGetTexturePatternTypeMapping(const std::string &str, UiTexturePatternType *outPatternType)
	{
		const auto mappingsBegin = std::begin(TexturePatternTypeMappings);
		const auto mappingsEnd = std::end(TexturePatternTypeMappings);
		const auto iter = std::find_if(mappingsBegin, mappingsEnd,
			[&str](const std::pair<const char*, UiTexturePatternType> &pair)
		{
			return pair.first == str;
		});

		if (iter == mappingsEnd)
		{
			return false;
		}

		*outPatternType = iter->second;
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

	bool TryGetMouseButtonTypeFlags(const std::string &str, MouseButtonTypeFlags *outFlags)
	{
		const auto mappingsBegin = std::begin(MouseButtonTypeFlagsMappings);
		const auto mappingsEnd = std::end(MouseButtonTypeFlagsMappings);
		const auto iter = std::find_if(mappingsBegin, mappingsEnd,
			[&str](const std::pair<const char*, MouseButtonTypeFlags> &pair)
		{
			return pair.first == str;
		});

		if (iter == mappingsEnd)
		{
			return false;
		}

		*outFlags = iter->second;
		return true;
	}

	bool TryGetButtonCallback(const std::string &namespaceStr, const std::string &functionName, UiButtonDefinitionCallback *outCallback)
	{
		const auto mappingsBegin = std::begin(ContextNamespaceCallbacks);
		const auto mappingsEnd = std::end(ContextNamespaceCallbacks);
		const auto namespaceIter = std::find_if(mappingsBegin, mappingsEnd,
			[&namespaceStr](const auto &tuple)
		{
			return std::get<0>(tuple) == namespaceStr;
		});

		if (namespaceIter == mappingsEnd)
		{
			return false;
		}

		Span<const std::pair<const char*, UiButtonDefinitionCallback>> callbackPairs = std::get<1>(*namespaceIter);
		const auto functionIter = std::find_if(callbackPairs.begin(), callbackPairs.end(),
			[&functionName](const std::pair<const char*, UiButtonDefinitionCallback> &pair)
		{
			return pair.first == functionName;
		});

		if (functionIter == callbackPairs.end())
		{
			return false;
		}

		*outCallback = functionIter->second;
		return true;
	}

	bool TryGetInputActionCallback(const std::string &namespaceStr, const std::string &functionName, UiInputListenerDefinitionCallback *outCallback)
	{
		const auto mappingsBegin = std::begin(ContextNamespaceCallbacks);
		const auto mappingsEnd = std::end(ContextNamespaceCallbacks);
		const auto namespaceIter = std::find_if(mappingsBegin, mappingsEnd,
			[&namespaceStr](const auto &tuple)
		{
			return std::get<0>(tuple) == namespaceStr;
		});

		if (namespaceIter == mappingsEnd)
		{
			return false;
		}

		Span<const std::pair<const char*, UiInputListenerDefinitionCallback>> callbackPairs = std::get<2>(*namespaceIter);
		const auto functionIter = std::find_if(callbackPairs.begin(), callbackPairs.end(),
			[&functionName](const std::pair<const char*, UiInputListenerDefinitionCallback> &pair)
		{
			return pair.first == functionName;
		});

		if (functionIter == callbackPairs.end())
		{
			return false;
		}

		*outCallback = functionIter->second;
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

	bool IsKeyValidForListBoxEntry(const std::string &key)
	{
		return std::find(std::begin(ValidListBoxKeys), std::end(ValidListBoxKeys), key) != std::end(ValidListBoxKeys);
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

	bool TryParseDouble(const std::string &str, double *outValue)
	{
		try
		{
			size_t index = 0;
			*outValue = std::stod(str, &index);
			return index == str.size();
		}
		catch (std::exception)
		{
			DebugLogErrorFormat("Couldn't parse \"%s\" as a double.", str.c_str());
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
		else if (key == Keyword_ElementClipRect)
		{
			std::string rectTokens[4];
			if (!String::splitExpected<4>(value, ',', rectTokens))
			{
				DebugLogErrorFormat("Couldn't split clip rect value \"%s\" into X,Y,width,height.", value.c_str());
				return false;
			}

			int rectX = 0;
			int rectY = 0;
			int rectWidth = 0;
			int rectHeight = 0;

			bool success = true;
			success &= TryParseInteger(rectTokens[0], &rectX);
			success &= TryParseInteger(rectTokens[1], &rectY);
			success &= TryParseInteger(rectTokens[2], &rectWidth);
			success &= TryParseInteger(rectTokens[3], &rectHeight);
			if (!success)
			{
				DebugLogErrorFormat("Couldn't parse clip rect value \"%s\".", value.c_str());
				return false;
			}

			outElementDef->clipRect.x = rectX;
			outElementDef->clipRect.y = rectY;
			outElementDef->clipRect.width = rectWidth;
			outElementDef->clipRect.height = rectHeight;			
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
			// @todo support TextureAsset index optional argument (check for 1 comma)
			const bool isGeneratedTexture = std::count(value.begin(), value.end(), ',') == 2;

			if (isGeneratedTexture)
			{
				std::string textureTokens[3];
				if (!String::splitExpected<3>(value, ',', textureTokens))
				{
					DebugLogErrorFormat("Couldn't split generated texture value \"%s\" into PatternType,Width,Height.", value.c_str());
					return false;
				}

				UiTexturePatternType patternType = static_cast<UiTexturePatternType>(-1);
				if (!TryGetTexturePatternTypeMapping(textureTokens[0], &patternType))
				{
					DebugLogErrorFormat("Couldn't parse generated texture pattern type value \"%s\".", value.c_str());
					return false;
				}

				int generatedWidth = 0;
				int generatedHeight = 0;

				bool success = true;
				success &= TryParseInteger(textureTokens[1], &generatedWidth);
				success &= TryParseInteger(textureTokens[2], &generatedHeight);
				if (!success)
				{
					DebugLogErrorFormat("Couldn't parse generated texture width/height values \"%s\".", value.c_str());
					return false;
				}

				outImageDef->type = UiImageDefinitionType::Generated;
				outImageDef->patternType = patternType;
				outImageDef->generatedWidth = generatedWidth;
				outImageDef->generatedHeight = generatedHeight;
			}
			else
			{
				outImageDef->type = UiImageDefinitionType::Asset;
				outImageDef->texture = TextureAsset(std::string(value));

				if (outImageDef->palette.filename.empty())
				{
					outImageDef->palette = outImageDef->texture;
				}
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

		auto unescapeText = [](const std::string &text)
		{
			std::string resultText;

			for (size_t i = 0; i < text.length(); i++)
			{
				const char c = text[i];
				const bool isNewline = (c == '\\') && (i < (text.length() - 1)) && (text[i + 1] == 'n');
				if (isNewline)
				{
					resultText.push_back('\n');
				}
				else
				{
					resultText.push_back(c);
				}
			}

			return resultText;
		};

		if (key == Keyword_TextBoxWorstCaseText)
		{
			outTextBoxDef->worstCaseText = unescapeText(value);
		}
		else if (key == Keyword_TextBoxText)
		{
			outTextBoxDef->text = unescapeText(value);
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
			std::string shadowTokens[6];
			if (!String::splitExpected<6>(value, ',', shadowTokens))
			{
				DebugLogErrorFormat("Couldn't split shadow value \"%s\" into offsetX,offsetY,r,g,b,a.", value.c_str());
				return false;
			}

			int offsetX = 0;
			int offsetY = 0;
			int r = 0;
			int g = 0;
			int b = 0;
			int a = 0;

			bool success = true;
			success &= TryParseInteger(shadowTokens[0], &offsetX);
			success &= TryParseInteger(shadowTokens[1], &offsetY);
			success &= TryParseInteger(shadowTokens[2], &r);
			success &= TryParseInteger(shadowTokens[3], &g);
			success &= TryParseInteger(shadowTokens[4], &b);
			success &= TryParseInteger(shadowTokens[5], &a);
			if (!success)
			{
				DebugLogErrorFormat("Couldn't parse shadow value \"%s\".", value.c_str());
				return false;
			}

			TextRenderShadowInfo shadowInfo;
			shadowInfo.offsetX = offsetX;
			shadowInfo.offsetY = offsetY;
			shadowInfo.color = Color(r, g, b, a);

			outTextBoxDef->shadowInfo = shadowInfo;
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

	bool TryParseListBoxLine(const std::string &key, const std::string &value, UiListBoxDefinition *outListBoxDef)
	{
		if (TryParseElementLine(key, value, &outListBoxDef->element))
		{
			return true;
		}

		if (!IsKeyValidForListBoxEntry(key))
		{
			return false;
		}

		if (key == Keyword_ListBoxTextureSize)
		{
			std::string sizeTokens[2];
			if (!String::splitExpected<2>(value, ',', sizeTokens))
			{
				DebugLogErrorFormat("Couldn't split texture size value \"%s\" into width,height.", value.c_str());
				return false;
			}

			int textureWidth = 0;
			int textureHeight = 0;

			bool success = true;
			success &= TryParseInteger(sizeTokens[0], &textureWidth);
			success &= TryParseInteger(sizeTokens[1], &textureHeight);
			if (!success)
			{
				DebugLogErrorFormat("Couldn't parse texture size value \"%s\".", value.c_str());
				return false;
			}

			outListBoxDef->textureWidth = textureWidth;
			outListBoxDef->textureHeight = textureHeight;
		}
		else if (key == Keyword_ListBoxItemPixelSpacing)
		{
			int itemPixelSpacing = 0;
			if (!TryParseInteger(value, &itemPixelSpacing))
			{
				DebugLogErrorFormat("Couldn't parse item pixel spacing value \"%s\".", value.c_str());
				return false;
			}

			outListBoxDef->itemPixelSpacing = itemPixelSpacing;
		}
		else if (key == Keyword_ListBoxFontName)
		{
			outListBoxDef->fontName = value;
		}
		else if (key == Keyword_ListBoxDefaultTextColor)
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

			outListBoxDef->defaultTextColor = Color(r, g, b, a);
		}
		else if (key == Keyword_ListBoxMouseButtons)
		{
			MouseButtonTypeFlags mouseButtonTypeFlags;
			if (!TryGetMouseButtonTypeFlags(value, &mouseButtonTypeFlags))
			{
				DebugLogErrorFormat("Couldn't parse mouse button type flags value \"%s\".", value.c_str());
				return false;
			}

			outListBoxDef->buttonFlags = mouseButtonTypeFlags;
		}
		else if (key == Keyword_ListBoxScrollDeltaScale)
		{
			double scrollDeltaScale = 0.0;
			if (!TryParseDouble(value, &scrollDeltaScale))
			{
				DebugLogErrorFormat("Couldn't parse scroll delta scale value \"%s\".", value.c_str());
				return false;
			}

			outListBoxDef->scrollDeltaScale = scrollDeltaScale;
		}
		else
		{
			DebugLogErrorFormat("Unrecognized list box key \"%s\".", key.c_str());
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
			MouseButtonTypeFlags mouseButtonTypeFlags;
			if (!TryGetMouseButtonTypeFlags(value, &mouseButtonTypeFlags))
			{
				DebugLogErrorFormat("Couldn't parse mouse button type flags value \"%s\".", value.c_str());
				return false;
			}

			outButtonDef->buttonFlags = mouseButtonTypeFlags;
		}
		else if (key == Keyword_ButtonCallback)
		{
			std::string functionTokens[2];
			if (!String::splitExpected<2>(value, '.', functionTokens))
			{
				DebugLogErrorFormat("Couldn't split button callback value \"%s\" into Namespace,Function.", value.c_str());
				return false;
			}

			UiButtonDefinitionCallback callback;
			if (!TryGetButtonCallback(functionTokens[0], functionTokens[1], &callback))
			{
				DebugLogErrorFormat("Couldn't parse button callback value \"%s\".", value.c_str());
				return false;
			}

			outButtonDef->callback = callback;
		}
		else if (key == Keyword_ButtonContentElementName)
		{
			outButtonDef->contentElementName = value;
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

		if (key == Keyword_InputListenerInputActionName)
		{
			outInputListenerDef->inputActionName = value;
		}
		else if (key == Keyword_InputListenerCallback)
		{
			std::string functionTokens[2];
			if (!String::splitExpected<2>(value, '.', functionTokens))
			{
				DebugLogErrorFormat("Couldn't split input action callback value \"%s\" into Namespace,Function.", value.c_str());
				return false;
			}

			UiInputListenerDefinitionCallback callback;
			if (!TryGetInputActionCallback(functionTokens[0], functionTokens[1], &callback))
			{
				DebugLogErrorFormat("Couldn't parse input action callback value \"%s\".", value.c_str());
				return false;
			}

			outInputListenerDef->callback = callback;
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
	this->clipRect = Rect();
	this->drawOrder = 0;
	this->renderSpace = UiRenderSpace::Classic;
}

UiImageDefinition::UiImageDefinition()
{
	this->type = static_cast<UiImageDefinitionType>(-1);
	this->generatedWidth = -1;
	this->generatedHeight = -1;
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

UiListBoxDefinition::UiListBoxDefinition()
{
	this->textureWidth = 0;
	this->textureHeight = 0;
	this->itemPixelSpacing = 0;
	this->buttonFlags = MouseButtonTypeFlags(MouseButtonType::Left);
	this->scrollDeltaScale = 1.0;
}

void UiListBoxDefinition::clear()
{
	this->element.clear();
	this->textureWidth = 0;
	this->textureHeight = 0;
	this->itemPixelSpacing = 0;
	this->fontName.clear();
	this->defaultTextColor = Colors::Black;
	this->buttonFlags = MouseButtonTypeFlags(MouseButtonType::Left);
	this->scrollDeltaScale = 1.0;
}

UiButtonDefinition::UiButtonDefinition()
{
	this->buttonFlags = MouseButtonTypeFlags(MouseButtonType::Left);
	this->callback = nullptr;
}

void UiButtonDefinition::clear()
{
	this->element.clear();
	this->buttonFlags = MouseButtonTypeFlags(MouseButtonType::Left);
	this->callback = nullptr;
	this->contentElementName.clear();
}

UiInputListenerDefinition::UiInputListenerDefinition()
{
	this->callback = nullptr;
}

void UiInputListenerDefinition::clear()
{
	this->name.clear();
	this->callback = nullptr;
}

UiContextDefinition::UiContextDefinition()
{
	this->clear();
}

void UiContextDefinition::clear()
{
	this->name.clear();
	this->imageDefs.clear();
	this->textBoxDefs.clear();
	this->listBoxDefs.clear();
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

				if (key == Keyword_ContextName)
				{
					if (entryName != ContextSectionName)
					{
						DebugLogErrorFormat("Context names must only be under the \"%s\" entry.", ContextSectionName.c_str());
						continue;
					}

					if (value.empty())
					{
						DebugLogErrorFormat("Couldn't get context name from entry \"%s\".", entryName.c_str());
						continue;
					}

					contextDef.name = value;
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
					else if (value == Keyword_EntryType_ListBox)
					{
						parseState.entryType = UiParseEntryType::ListBox;
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
					case UiParseEntryType::ListBox:
						if (!TryParseListBoxLine(key, value, &parseState.listBoxDef))
						{
							DebugLogErrorFormat("Couldn't parse \"%s\" and \"%s\" for list box entry \"%s\".", key.c_str(), value.c_str(), entryName.c_str());
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
			case UiParseEntryType::ListBox:
				parseState.listBoxDef.element.name = entryName;
				contextDef.listBoxDefs.emplace_back(parseState.listBoxDef);
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

const UiContextDefinition &UiLibrary::getDefinition(const char *contextName) const
{
	for (const UiContextDefinition &def : this->contextDefs)
	{
		if (StringView::equals(def.name, contextName))
		{
			return def;
		}
	}

	DebugCrashFormat("Couldn't find context definition for type %s.", contextName);
	return UiContextDefinition();
}
