#include "font_editor.h"

#include "halley/tools/project/project.h"

using namespace Halley;

FontEditor::FontEditor(UIFactory& factory, Resources& gameResources, AssetType type, Project& project, ProjectWindow& projectWindow)
	: AssetEditor(factory, gameResources, project, type)
{
	factory.loadUI(*this, "halley/font_editor");
}

void FontEditor::onMakeUI()
{
	bindData("customPreviewText", "", [=] (String previewText)
	{
		updatePreviewText(LocalisedString::fromUserString(std::move(previewText)));
	});
}

void FontEditor::refresh()
{
}

void FontEditor::onResourceLoaded()
{
	updateFont();
}

void FontEditor::refreshAssets()
{
	AssetEditor::refreshAssets();
	updateFont();
}

std::shared_ptr<const Resource> FontEditor::loadResource(const Path& assetPath, const String& assetId, AssetType assetType)
{
	loadMetadata();
	font = gameResources.get<Font>(assetId);
	return font;
}

void FontEditor::updateFont()
{
	if (!resource) {
		return;
	}
	font = std::dynamic_pointer_cast<const Font>(resource);
	if (!font) {
		Logger::logError("Unable to convert resource into font in FontEditor");
		return;
	}

	updatePreviews();
}

void FontEditor::updatePreviews()
{
	const auto style = TextRenderer(font)
		.setColour(Colour4f(1, 1, 1, 1));

	const auto preview0 = getWidgetAs<UILabel>("preview0");
	const auto preview1 = getWidgetAs<UILabel>("preview1");
	const auto preview2 = getWidgetAs<UILabel>("preview2");
	const auto previewRegular = getWidgetAs<UILabel>("previewRegular");
	const auto previewOutline = getWidgetAs<UILabel>("previewOutline");
	const auto previewShadow = getWidgetAs<UILabel>("previewShadow");
	const auto previewRegularSmall = getWidgetAs<UILabel>("previewRegularSmall");
	const auto previewOutlineSmall = getWidgetAs<UILabel>("previewOutlineSmall");
	const auto previewShadowSmall = getWidgetAs<UILabel>("previewShadowSmall");

	preview0->setTextRenderer(style.clone().setSize(42));
	preview1->setTextRenderer(style.clone().setSize(20));
	preview2->setTextRenderer(style.clone().setSize(10));
	previewRegular->setTextRenderer(style.clone().setSize(64));
	previewOutline->setTextRenderer(style.clone().setSize(64).setOutline(1000.0f, Colour4f(0, 0, 0)));
	previewShadow->setTextRenderer(style.clone().setSize(64).setShadow(3.0f, 1000.0f, Colour4f(0, 0, 0)));
	previewRegularSmall->setTextRenderer(style.clone().setSize(20));
	previewOutlineSmall->setTextRenderer(style.clone().setSize(20).setOutline(1000.0f, Colour4f(0, 0, 0)));
	previewShadowSmall->setTextRenderer(style.clone().setSize(20).setShadow(2.0f, 1000.0f, Colour4f(0, 0, 0)));

	updatePreviewText();
}

void FontEditor::updatePreviewText()
{
	const auto quickBrownFox = LocalisedString::fromHardcodedString("The Quick Brown Fox Jumps Over the Lazy Dog");
	const auto loremIpsumShort = LocalisedString::fromHardcodedString("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.");
	const auto loremIpsum = LocalisedString::fromHardcodedString("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
	const auto regular = LocalisedString::fromHardcodedString("Regular");
	const auto outline = LocalisedString::fromHardcodedString("Outline");
	const auto shadow = LocalisedString::fromHardcodedString("Shadow");

	const auto preview0 = getWidgetAs<UILabel>("preview0");
	const auto preview1 = getWidgetAs<UILabel>("preview1");
	const auto preview2 = getWidgetAs<UILabel>("preview2");
	const auto previewRegular = getWidgetAs<UILabel>("previewRegular");
	const auto previewOutline = getWidgetAs<UILabel>("previewOutline");
	const auto previewShadow = getWidgetAs<UILabel>("previewShadow");
	const auto previewRegularSmall = getWidgetAs<UILabel>("previewRegularSmall");
	const auto previewOutlineSmall = getWidgetAs<UILabel>("previewOutlineSmall");
	const auto previewShadowSmall = getWidgetAs<UILabel>("previewShadowSmall");

	const bool hasPreview = !previewText.getString().isEmpty();
	preview0->setText(hasPreview ? previewText : loremIpsumShort);
	preview1->setText(hasPreview ? previewText : loremIpsum);
	preview2->setText(hasPreview ? previewText : loremIpsum);
	previewRegular->setText(hasPreview ? previewText : regular);
	previewOutline->setText(hasPreview ? previewText : outline);
	previewShadow->setText(hasPreview ? previewText : shadow);
	previewRegularSmall->setText(hasPreview ? previewText : regular);
	previewOutlineSmall->setText(hasPreview ? previewText : outline);
	previewShadowSmall->setText(hasPreview ? previewText : shadow);

	layout();
}

void FontEditor::updatePreviewText(LocalisedString text)
{
	previewText = std::move(text);
	updatePreviewText();
}

void FontEditor::loadMetadata()
{
	metaPath = project.getAssetsSrcPath() / assetPath;
	metadata = project.getImportAssetsDatabase().getMetadata(assetType, assetId).value_or(Metadata());
	//metadata = project.readMetadataFromDisk(metaPath);
}

void FontEditor::saveMetadata()
{
	project.writeMetadataToDisk(metaPath, metadata);
}
