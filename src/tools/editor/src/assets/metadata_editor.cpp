#include "metadata_editor.h"
#include "halley/tools/project/project.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_validator.h"
#include "halley/ui/widgets/ui_checkbox.h"
#include "halley/ui/widgets/ui_label.h"
#include "halley/ui/widgets/ui_textinput.h"
#include "src/ui/select_asset_widget.h"
#include "src/ui/project_window.h"
using namespace Halley;

MetadataEditor::MetadataEditor(UIFactory& factory, ProjectWindow& projectWindow)
	: UIWidget("metadataEditor", {}, UISizer(UISizerType::Grid, 1, 2))
	, factory(factory)
	, projectWindow(projectWindow)
{
}

bool MetadataEditor::hasEditorForType(AssetType type)
{
	switch (type) {
	case AssetType::Sprite:
	case AssetType::Animation:
	case AssetType::AudioClip:
	case AssetType::Font:
	case AssetType::Mesh:
		return true;
	default:
		return false;
	}
}

void MetadataEditor::setResource(Project& p, AssetType type, const Path& path, Metadata effectiveMeta)
{
	assetType = type;
	filePath = path;
	project = &p;
	metadata = project->readMetadataFromDisk(filePath);
	origMetadata = metadata;
	effectiveMetadata = std::move(effectiveMeta);

	makeUI();
}

void MetadataEditor::onMetadataChanged()
{
	changed = true;// metadata != origMetadata;
	getWidget("applyChanges")->setEnabled(changed);
}

void MetadataEditor::saveMetadata()
{
	if (changed) {
		project->writeMetadataToDisk(filePath, metadata);
		origMetadata = metadata;
		changed = false;
		getWidget("applyChanges")->setEnabled(false);
	}
}

bool MetadataEditor::isModified() const
{
	return changed;
}

void MetadataEditor::makeUI()
{
	clear();
	add(factory.makeUI("halley/metadata_editor"));
	fields = getWidget("fields");
	getWidget("applyChanges")->setEnabled(false);

	setHandle(UIEventType::ButtonClicked, "applyChanges", [=](const UIEvent& event)
	{
		saveMetadata();
	});

	const bool isAseprite = filePath.getExtension() == ".ase" || filePath.getExtension() == ".aseprite";

	switch (assetType) {
	case AssetType::Sprite:
	case AssetType::Animation:
		addInt2Field("Pivot", "pivotX", "pivotY", Vector2i());
		addInt4Field("Slices", "slice_left", "slice_top", "slice_right", "slice_bottom", Vector4i());
		if (isAseprite) {
			addBoolField("By Group", "group_separated", false);
		}
		addAssetTypeField("Material", "material", AssetType::MaterialDefinition, MaterialDefinition::defaultMaterial);
		addStringField("Atlas", "atlas", "");
		addStringField("Fallback Sequence", "fallbackSequence", "");
		addAssetTypeField("Palette", "palette", AssetType::Texture, "");
		addBoolField("Palette Top\nLine Only", "paletteTopLineOnly", false);
		addBoolField("Filtering", "filtering", false);
		addBoolField("Minimap", "minimap", false);
		addBoolField("Power of Two", "powerOfTwo", true);
		addEnumField<TextureFormat>("Format", "format", "rgba");
		addEnumField<TextureAddressMode>("Address", "addressMode", "clamp");
		addInt2Field("Tile Split", "tileWidth", "tileHeight", Vector2i());
		addBoolField("Trim", "trim", true);
		addIntField("Padding", "padding", 0);
		break;

	case AssetType::AudioClip:
		addBoolField("Streaming", "streaming", false);
		addIntField("Loop Point", "loopPoint", 0);
		break;

	case AssetType::Font:
		addFloatField("Font Size", "fontSize", 0);
		addStringField("Font Name", "fontName", "");
		addInt2Field("Image Size", "width", "height", Vector2i(512, 512));
		addInt2Field("Range", "rangeStart", "rangeEnd", Vector2i(0, 255));
		addIntField("Ascender Adj.", "ascenderAdjustment", 0);
		addIntField("Line Spacing", "lineSpacing", 0);
		addFloatField("Radius", "radius", 8.0f);
		addStringField("Fallback", "fallback", "");
		addFloatField("Fallback Scale", "replacementScale", 1.0f);
		addStringField("Extra characters", "extraCharacters", "");
		addBoolField("Floor Glyph Pos.", "floorGlyphPosition", false);
		break;
	case AssetType::Mesh:
		addAssetTypeField("Material", "material", AssetType::MaterialDefinition, MaterialDefinition::defaultMeshMaterial);
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

void MetadataEditor::addAssetTypeField(const String& name, const String& key, AssetType type, const String& defaultValue)
{
	makeLabel(name);
	makeAssetTypeField(fields->getSizer(), key, type, defaultValue);
}

void MetadataEditor::addDropdownField(const String& name, const String& key, Vector<String> values, const String& defaultValue)
{
	makeLabel(name);
	makeDropdownField(fields->getSizer(), key, std::move(values), defaultValue);
}

void MetadataEditor::makeLabel(const String& name)
{
	fields->add(std::make_shared<UILabel>("", factory.getStyle("label"), LocalisedString::fromUserString(name)), 0, Vector4f(0, 0, 10, 0), UISizerAlignFlags::CentreVertical);
}

template <typename T>
static void updateMetadata(Metadata& metadata, const String& key, MetadataEditor& editor, const T& value, const T& defaultValue)
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

void MetadataEditor::setPivot(Vector2i pos)
{
	const auto px = tryGetWidgetAs<UITextInput>("pivotX");
	const auto py = tryGetWidgetAs<UITextInput>("pivotY");
	if (px && py) {
		px->setText(toString(pos.x));
		py->setText(toString(pos.y));
	}
}

String MetadataEditor::getString(const String& key) const
{
	return metadata.getString(key, effectiveMetadata.getString(key, ""));
}

void MetadataEditor::setValue(const String& key, ConfigNode value)
{
	metadata.set(key, std::move(value));
	onMetadataChanged();
}

ConfigNode MetadataEditor::getValue(const String& key) const
{
	return metadata.getValue(key);
}

void MetadataEditor::makeIntField(UISizer& sizer, const String& key, int defaultValue)
{
	const auto effectiveDefault = effectiveMetadata.getInt(":" + key, defaultValue);
	const auto value = metadata.getInt(key, effectiveDefault);

	const auto result = std::make_shared<UISpinControl2>(key, factory.getStyle("spinControl"), float(value), false);
	result->setMinSize(Vector2f(40, 22));
	result->setGhostText(LocalisedString::fromNumber(effectiveDefault));
	sizer.add(result, 1);

	bindData(key, value, [=] (int value)
	{
		updateMetadata(metadata, key, *this, toString(value), toString(defaultValue));
	});
}

void MetadataEditor::makeFloatField(UISizer& sizer, const String& key, float defaultValue)
{
	const auto effectiveDefault = effectiveMetadata.getFloat(":" + key, defaultValue);
	const auto value = metadata.getFloat(key, effectiveDefault);

	const auto result = std::make_shared<UISpinControl2>(key, factory.getStyle("spinControl"), value, true);
	result->setMinSize(Vector2f(40, 22));
	result->setGhostText(LocalisedString::fromUserString(toString(effectiveDefault)));
	sizer.add(result, 1);

	bindData(key, value, [=] (float value)
	{
		updateMetadata(metadata, key, *this, toString(value), toString(defaultValue));
	});
}

void MetadataEditor::makeBoolField(UISizer& sizer, const String& key, bool defaultValue)
{
	const auto result = std::make_shared<UICheckbox>(key, factory.getStyle("checkbox"));
	result->setMinSize(Vector2f(22, 22));
	sizer.add(result, 1, {}, UISizerAlignFlags::Left | UISizerAlignFlags::CentreVertical);

	const auto effectiveDefault = effectiveMetadata.getBool(":" + key, defaultValue);
	
	bindData(key, metadata.getBool(key, effectiveDefault), [=] (bool value)
	{
		updateMetadata(metadata, key, *this, value, defaultValue);
	});
}

void MetadataEditor::makeStringField(UISizer& sizer, const String& key, const String& defaultValue)
{
	const auto result = std::make_shared<UITextInput>(key, factory.getStyle("inputThin"));
	result->setMinSize(Vector2f(40, 22));
	sizer.add(result, 1);

	const auto effectiveDefault = effectiveMetadata.getString(":" + key, defaultValue);
	const auto value = metadata.getString(key, "");
	result->setGhostText(LocalisedString::fromUserString(effectiveDefault));

	bindData(key, value, [=] (const String& value)
	{
		updateMetadata(metadata, key, *this, value, defaultValue);
	});
}

void MetadataEditor::makeDropdownField(UISizer& sizer, const String& key, Vector<String> values, const String& defaultValue)
{
	const auto result = std::make_shared<UIDropdown>(key, factory.getStyle("dropdown"));
	result->setOptions(std::move(values));
	sizer.add(result, 1);

	const auto effectiveDefault = effectiveMetadata.getString(":" + key, defaultValue);
	const auto value = metadata.getString(key, effectiveDefault);

	bindData(key, value, [=] (const String& value)
	{
		updateMetadata(metadata, key, *this, value, defaultValue);
	});
}

void MetadataEditor::makeAssetTypeField(UISizer& sizer, const String& key, AssetType type, const String& defaultValue)
{
	const auto result = std::make_shared<SelectAssetWidget>(key, factory, type, project->getGameResources(), projectWindow);
	result->setAllowEmpty("");
	result->setMinSize(Vector2f(40, 22));
	sizer.add(result, 1);

	const auto effectiveDefault = effectiveMetadata.getString(":" + key, defaultValue);
	const auto value = metadata.getString(key, effectiveDefault);

	bindData(key, value, [=] (const String& value)
	{
		updateMetadata(metadata, key, *this, value, defaultValue);
	});
}
