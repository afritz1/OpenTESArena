#include "SDL.h"

#include "TextSubPanel.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"
#include "../Math/Rect.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../UI/CursorAlignment.h"
#include "../UI/CursorData.h"
#include "../UI/FontLibrary.h"
#include "../UI/TextBox.h"

TextSubPanel::TextSubPanel(Game &game)
	: Panel(game) { }

bool TextSubPanel::init(const TextBox::InitInfo &textBoxInitInfo, const std::string_view &text,
	const OnClosedFunction &onClosed, Texture &&texture, const Int2 &textureCenter)
{
	auto &game = this->getGame();

	if (!this->textBox.init(textBoxInitInfo, text, game.getRenderer()))
	{
		DebugLogError("Couldn't init sub-panel text box.");
		return false;
	}

	this->closeButton = Button<Game&>(
		0,
		0,
		ArenaRenderUtils::SCREEN_WIDTH,
		ArenaRenderUtils::SCREEN_HEIGHT,
		onClosed);

	this->addButtonProxy(MouseButtonType::Left, this->closeButton.getRect(),
		[this, &game]() { this->closeButton.click(game); });
	this->addButtonProxy(MouseButtonType::Right, this->closeButton.getRect(),
		[this, &game]() { this->closeButton.click(game); });

	this->addInputActionListener(InputActionName::Back,
		[this](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			this->closeButton.click(values.game);
		}
	});
	
	this->texture = std::move(texture);
	this->textureCenter = textureCenter;

	return true;
}

bool TextSubPanel::init(const TextBox::InitInfo &textBoxInitInfo, const std::string_view &text,
	const OnClosedFunction &onClosed)
{
	return this->init(textBoxInitInfo, text, onClosed, Texture(), Int2());
}

std::optional<CursorData> TextSubPanel::getCurrentCursor() const
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &textureManager = game.getTextureManager();
	
	const std::string &paletteFilename = ArenaPaletteName::Default;
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteFilename.c_str());
	if (!paletteID.has_value())
	{
		DebugLogWarning("Couldn't get palette ID for \"" + paletteFilename + "\".");
		return std::nullopt;
	}

	const std::string &textureFilename = ArenaTextureName::SwordCursor;
	const std::optional<TextureBuilderID> textureBuilderID =
		textureManager.tryGetTextureBuilderID(textureFilename.c_str());
	if (!textureBuilderID.has_value())
	{
		DebugLogWarning("Couldn't get texture ID for \"" + textureFilename + "\".");
		return std::nullopt;
	}

	return CursorData(*textureBuilderID, *paletteID, CursorAlignment::TopLeft);
}

void TextSubPanel::render(Renderer &renderer)
{
	// Draw background if it exists.
	if (this->texture.get() != nullptr)
	{
		const Rect textureRect(
			this->textureCenter.x - (this->texture.getWidth() / 2),
			this->textureCenter.y - (this->texture.getHeight() / 2),
			this->texture.getWidth(),
			this->texture.getHeight());
		const Rect nativeTextureRect = renderer.originalToNative(textureRect);

		renderer.draw(this->texture,
			nativeTextureRect.getLeft(),
			nativeTextureRect.getTop(),
			nativeTextureRect.getWidth(),
			nativeTextureRect.getHeight());
	}

	// Draw text.
	const Rect nativeTextBoxRect = renderer.originalToNative(this->textBox.getRect());
	renderer.draw(this->textBox.getTexture(),
		nativeTextBoxRect.getLeft(),
		nativeTextBoxRect.getTop(),
		nativeTextBoxRect.getWidth(),
		nativeTextBoxRect.getHeight());
}
