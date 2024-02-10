#include "font_editor.h"

using namespace Halley;

FontEditor::FontEditor(UIFactory& factory, Resources& gameResources, AssetType type, Project& project, ProjectWindow& projectWindow)
	: AssetEditor(factory, gameResources, project, type)
{
	factory.loadUI(*this, "halley/font_editor");
}

void FontEditor::onMakeUI()
{
}

void FontEditor::refresh()
{
}

void FontEditor::reload()
{
	updatePreviews();
}

void FontEditor::refreshAssets()
{
}

void FontEditor::onAddedToRoot(UIRoot& root)
{
}

void FontEditor::onRemovedFromRoot(UIRoot& root)
{
}

bool FontEditor::onKeyPress(KeyboardKeyPress key)
{
	return false;
}

void FontEditor::update(Time t, bool moved)
{
}

std::shared_ptr<const Resource> FontEditor::loadResource(const String& assetId)
{
	font = gameResources.get<Font>(assetId);
	return font;
}

void FontEditor::updatePreviews()
{
	const auto text = LocalisedString::fromHardcodedString("The Quick Brown Fox Jumps Over the Lazy Dog");
	const auto loremIpsum = LocalisedString::fromHardcodedString("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
	
	const auto style = TextRenderer(font)
		.setColour(Colour4f(1, 1, 1, 1));

	const auto preview0 = getWidgetAs<UILabel>("preview0");
	const auto preview1 = getWidgetAs<UILabel>("preview1");
	const auto preview2 = getWidgetAs<UILabel>("preview2");

	preview0->setTextRenderer(style.clone().setSize(42));
	preview1->setTextRenderer(style.clone().setSize(20));
	preview2->setTextRenderer(style.clone().setSize(10));

	preview0->setText(text);
	preview1->setText(text);
	preview2->setText(loremIpsum);

	layout();
}
