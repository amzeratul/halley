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

void MetadataEditor::addInt(const String& name, const String& key, int defaultValue)
{
	makeLabel(name);
	makeIntField(getSizer(), key, defaultValue);
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

void MetadataEditor::addFloat(const String& name, const String& key, float defaultValue)
{
	makeLabel(name);
	makeFloatField(getSizer(), key, defaultValue);
}

void MetadataEditor::addBool(const String& name, const String& key, bool defaultValue)
{
	makeLabel(name);
	makeBoolField(getSizer(), key, defaultValue);	
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

template <typename T>
void updateMetadata(Metadata& metadata, const String& key, MetadataEditor& editor, const T& value, const T& defaultValue)
{
	if (value == defaultValue) {
		if (metadata.erase(key)) {
			editor.saveMetadata();
		}
	} else {
		if (metadata.set(key, value)) {
			editor.saveMetadata();
		}
	}
}

void MetadataEditor::makeIntField(UISizer& sizer, const String& key, int defaultValue)
{
	const auto result = std::make_shared<UITextInput>(factory.getKeyboard(), key, factory.getStyle("input"));
	result->setMinSize(Vector2f(40, 30));
	result->setValidator(std::make_shared<UINumericValidator>(true, false));
	sizer.add(result, 1);
	bindData(key, metadata.getInt(key, defaultValue), [=] (int value)
	{
		updateMetadata(metadata, key, *this, value, defaultValue);
	});
}

void MetadataEditor::makeFloatField(UISizer& sizer, const String& key, float defaultValue)
{
	const auto result = std::make_shared<UITextInput>(factory.getKeyboard(), key, factory.getStyle("input"));
	result->setMinSize(Vector2f(40, 30));
	result->setValidator(std::make_shared<UINumericValidator>(true, true));
	sizer.add(result, 1);
	bindData(key, metadata.getFloat(key, defaultValue), [=] (float value)
	{
		updateMetadata(metadata, key, *this, value, defaultValue);
	});
}

void MetadataEditor::makeBoolField(UISizer& sizer, const String& key, bool defaultValue)
{
	const auto result = std::make_shared<UICheckbox>(key, factory.getStyle("checkbox"));
	result->setMinSize(Vector2f(30, 30));
	sizer.add(result, 1);
	bindData(key, metadata.getBool(key, defaultValue), [=] (bool value)
	{
		updateMetadata(metadata, key, *this, value, defaultValue);
	});
}

void MetadataEditor::makeStringField(UISizer& sizer, const String& key, const String& defaultValue)
{
	const auto result = std::make_shared<UITextInput>(factory.getKeyboard(), key, factory.getStyle("input"));
	result->setMinSize(Vector2f(40, 30));
	sizer.add(result, 1);
	bindData(key, metadata.getString(key, defaultValue), [=] (const String& value)
	{
		updateMetadata(metadata, key, *this, value, defaultValue);
	});
}

void MetadataEditor::saveMetadata()
{
	project->setMetaData(assetType, assetId, metadata);
}
