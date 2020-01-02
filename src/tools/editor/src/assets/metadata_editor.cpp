#include "metadata_editor.h"
#include "halley/tools/project/project.h"
using namespace Halley;

MetadataEditor::MetadataEditor(UIFactory& factory)
	: UIWidget("metadataEditor", {}, UISizer(UISizerType::Grid, 1, 2))
	, factory(factory)
{
}

void MetadataEditor::setResource(Project& p, AssetType type, const String& name)
{
	assetType = type;
	assetId = name;
	project = &p;
	metadata = project->getMetadata(assetType, assetId).get_value_or(Metadata());

	makeUI();
}

void MetadataEditor::makeUI()
{
	clear();
	switch (assetType) {
	case AssetType::Sprite:
	case AssetType::Animation:
		addInt2Field("Pivot", "pivotX", "pivotY", Vector2i());
		addInt4Field("Slices", "slice_left", "slice_right", "slice_bottom", "slice_top", Vector4i());
		addInt2Field("Tile", "tileWidth", "tileHeight", Vector2i());
		addStringField("Palette", "palette", "");
		addStringField("Material", "material", "Halley/Sprite");
		addStringField("Atlas", "atlas", "");
	}
}

void MetadataEditor::addInt2Field(const String& name, const String& x, const String& y, Vector2i defaultValue)
{
	makeLabel(name);
	auto sizer = std::make_shared<UISizer>();
	add(sizer, 1);
	makeIntField(*sizer, x, defaultValue.x);
	makeIntField(*sizer, y, defaultValue.y);
}

void MetadataEditor::addInt4Field(const String& name, const String& x, const String& y, const String& z, const String& w, Vector4i defaultValue)
{
	makeLabel(name);
	auto sizer = std::make_shared<UISizer>();
	add(sizer, 1);
	makeIntField(*sizer, x, defaultValue.x);
	makeIntField(*sizer, y, defaultValue.y);
	makeIntField(*sizer, z, defaultValue.z);
	makeIntField(*sizer, w, defaultValue.w);
}

void MetadataEditor::addStringField(const String& name, const String& key, const String& defaultValue)
{
	makeLabel(name);
	makeStringField(getSizer(), key, defaultValue);
}

void MetadataEditor::makeLabel(const String& name)
{
	add(std::make_shared<UILabel>("", factory.getStyle("label").getTextRenderer("label"), LocalisedString::fromUserString(name)), 0, Vector4f(0, 0, 10, 0), UISizerAlignFlags::CentreVertical);
}

void MetadataEditor::makeIntField(UISizer& sizer, const String& key, int defaultValue)
{
	const auto result = std::make_shared<UITextInput>(factory.getKeyboard(), key, factory.getStyle("input"));
	result->setMinSize(Vector2f(40, 30));
	result->setValidator(std::make_shared<UINumericValidator>(true, false));
	sizer.add(result, 1);
	bindData(key, metadata.getInt(key, defaultValue), [=] (int value)
	{
		if (value == defaultValue) {
			metadata.erase(key);
		} else {
			if (metadata.set(key, value)) {
				saveMetadata();
			}
		}
	});
}

void MetadataEditor::makeStringField(UISizer& sizer, const String& key, const String& defaultValue)
{
	const auto result = std::make_shared<UITextInput>(factory.getKeyboard(), key, factory.getStyle("input"));
	result->setMinSize(Vector2f(40, 30));
	sizer.add(result, 1);
	bindData(key, metadata.getString(key, defaultValue), [=] (const String& value)
	{
		if (value == defaultValue) {
			metadata.erase(key);
		} else {
			if (metadata.set(key, value)) {
				saveMetadata();
			}
		}
	});
}

void MetadataEditor::saveMetadata()
{
	project->setMetaData(assetType, assetId, metadata);
}
