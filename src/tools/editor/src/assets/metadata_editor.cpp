#include "metadata_editor.h"
#include "halley/tools/project/project.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_validator.h"
#include "halley/ui/widgets/ui_checkbox.h"
#include "halley/ui/widgets/ui_label.h"
#include "halley/ui/widgets/ui_textinput.h"
using namespace Halley;

MetadataEditor::MetadataEditor(UIFactory& factory)
	: UIWidget("metadataEditor", {}, UISizer(UISizerType::Grid, 1, 2))
	, factory(factory)
{
}

void MetadataEditor::setResource(Project& p, AssetType type, const Path& path, Metadata effectiveMeta)
{
	assetType = type;
	filePath = path;
	project = &p;
	metadata = project->readMetadataFromDisk(filePath);
	effectiveMetadata = std::move(effectiveMeta);

	makeUI();
}

void MetadataEditor::onMetadataChanged()
{
	changed = true;
	getWidget("applyChanges")->setEnabled(true);
}

void MetadataEditor::saveMetadata()
{
	if (changed) {
		project->writeMetadataToDisk(filePath, metadata);
		changed = false;
		getWidget("applyChanges")->setEnabled(false);
	}
}

void MetadataEditor::makeUI()
{
	clear();
	add(factory.makeUI("ui/halley/metadata_editor"));
	fields = getWidget("fields");
	getWidget("applyChanges")->setEnabled(false);

	setHandle(UIEventType::ButtonClicked, "applyChanges", [=](const UIEvent& event)
	{
		saveMetadata();
	});

	switch (assetType) {
	case AssetType::Sprite:
	case AssetType::Animation:
		addInt2Field("Pivot", "pivotX", "pivotY", Vector2i());
		addInt4Field("Slices", "slice_left", "slice_right", "slice_bottom", "slice_top", Vector4i());
		addInt2Field("Tile", "tileWidth", "tileHeight", Vector2i());
		addBoolField("Trim", "trim", true);
		addStringField("Atlas", "atlas", "");
		addStringField("Palette", "palette", "");
		addStringField("Material", "material", "Halley/Sprite");
		addBoolField("Filtering", "filtering", false);
		addBoolField("Minimap", "minimap", false);
		addStringField("Format", "format", "rgba");
		addStringField("Address Mode", "addressMode", "clamp");
		addBoolField("Group Separated", "group_separated", false);
		break;

	case AssetType::AudioClip:
		addBoolField("Streaming", "streaming", false);
		addIntField("Loop Point", "loopPoint", 0);
		break;

	case AssetType::Font:
		addFloatField("Font Size", "fontSize", 0);
		addStringField("Font Name", "fontName", "");
		addInt2Field("Image Size", "width", "height", Vector2i(512, 512));
		addIntField("Supersample", "supersample", 4);
		addInt2Field("Range", "rangeStart", "rangeEnd", Vector2i(0, 255));
		addIntField("Ascender Adj.", "ascenderAdjustment", 0);
		addIntField("Line Spacing", "lineSpacing", 0);
		addFloatField("Radius", "radius", 8.0f);
		addStringField("Fallback", "fallback", "");
		addFloatField("Fallback Scale", "replacementScale", 1.0f);
		addStringField("Extra characters", "extraCharacters", "");
		break;
	}
}

void MetadataEditor::addIntField(const String& name, const String& key, int defaultValue)
{
	makeLabel(name);
	makeIntField(fields->getSizer(), key, defaultValue);
}

void MetadataEditor::addInt2Field(const String& name, const String& x, const String& y, Vector2i defaultValue)
{
	makeLabel(name);
	auto sizer = std::make_shared<UISizer>();
	fields->add(sizer, 1);
	makeIntField(*sizer, x, defaultValue.x);
	makeIntField(*sizer, y, defaultValue.y);
}

void MetadataEditor::addInt4Field(const String& name, const String& x, const String& y, const String& z, const String& w, Vector4i defaultValue)
{
	makeLabel(name);
	auto sizer = std::make_shared<UISizer>();
	fields->add(sizer, 1);
	makeIntField(*sizer, x, defaultValue.x);
	makeIntField(*sizer, y, defaultValue.y);
	makeIntField(*sizer, z, defaultValue.z);
	makeIntField(*sizer, w, defaultValue.w);
}

void MetadataEditor::addFloatField(const String& name, const String& key, float defaultValue)
{
	makeLabel(name);
	makeFloatField(fields->getSizer(), key, defaultValue);
}

void MetadataEditor::addBoolField(const String& name, const String& key, bool defaultValue)
{
	makeLabel(name);
	makeBoolField(fields->getSizer(), key, defaultValue);
}

void MetadataEditor::addStringField(const String& name, const String& key, const String& defaultValue)
{
	makeLabel(name);
	makeStringField(fields->getSizer(), key, defaultValue);
}

void MetadataEditor::makeLabel(const String& name)
{
	fields->add(std::make_shared<UILabel>("", factory.getStyle("label").getTextRenderer("label"), LocalisedString::fromUserString(name)), 0, Vector4f(0, 0, 10, 0), UISizerAlignFlags::CentreVertical);
}

template <typename T>
void updateMetadata(Metadata& metadata, const String& key, MetadataEditor& editor, const T& value, const T& defaultValue)
{
	if (value == defaultValue) {
		if (metadata.erase(key)) {
			editor.onMetadataChanged();
		}
	} else {
		if (metadata.set(key, value)) {
			editor.onMetadataChanged();
		}
	}
}

void MetadataEditor::makeIntField(UISizer& sizer, const String& key, int defaultValue)
{
	const auto result = std::make_shared<UITextInput>(key, factory.getStyle("inputThin"));
	result->setMinSize(Vector2f(40, 22));
	result->setValidator(std::make_shared<UINumericValidator>(true, false));
	sizer.add(result, 1);
	bindData(key, metadata.getInt(key, defaultValue), [=] (int value)
	{
		updateMetadata(metadata, key, *this, value, defaultValue);
	});
}

void MetadataEditor::makeFloatField(UISizer& sizer, const String& key, float defaultValue)
{
	const auto result = std::make_shared<UITextInput>(key, factory.getStyle("inputThin"));
	result->setMinSize(Vector2f(40, 22));
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
	result->setMinSize(Vector2f(22, 22));
	sizer.add(result, 1, {}, UISizerAlignFlags::Left | UISizerAlignFlags::CentreVertical);
	bindData(key, metadata.getBool(key, defaultValue), [=] (bool value)
	{
		updateMetadata(metadata, key, *this, value, defaultValue);
	});
}

void MetadataEditor::makeStringField(UISizer& sizer, const String& key, const String& defaultValue)
{
	const auto result = std::make_shared<UITextInput>(key, factory.getStyle("inputThin"));
	result->setMinSize(Vector2f(40, 22));
	sizer.add(result, 1);

	auto effectiveDefault = effectiveMetadata.getString(":" + key, defaultValue);
	auto value = metadata.getString(key, effectiveDefault);

	bindData(key, value, [=] (const String& value)
	{
		updateMetadata(metadata, key, *this, value, defaultValue);
	});
}
