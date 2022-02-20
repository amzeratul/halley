#pragma once
#include "halley/ui/ui_widget.h"
#include "halley/file/path.h"

namespace Halley
{
	class Project;
	enum class AssetType;
	class UIFactory;

	class MetadataEditor : public UIWidget
	{
	public:
		MetadataEditor(UIFactory& factory);

		void setResource(Project& project, AssetType type, const Path& filePath, Metadata effectiveMetadata);
		void onMetadataChanged();
		void saveMetadata();
		bool isModified() const;

		void setPivot(Vector2i pos);

		String getMetaValue(const String& key) const;

	private:
		UIFactory& factory;
		Metadata metadata;
		Metadata origMetadata;
		Metadata effectiveMetadata;
		Project* project = nullptr;
		AssetType assetType = AssetType::BinaryFile;
		Path filePath;
		bool changed = false;

		std::shared_ptr<UIWidget> fields;

		void makeUI();

		void addIntField(const String& name, const String& key, int defaultValue);
		void addInt2Field(const String& name, const String& x, const String& y, Vector2i defaultValue);
		void addInt4Field(const String& name, const String& x, const String& y, const String& z, const String& w, Vector4i defaultValue);
		void addFloatField(const String& name, const String& key, float defaultValue);
		void addBoolField(const String& name, const String& key, bool defaultValue);
		void addStringField(const String& name, const String& key, const String& defaultValue);
		void addAssetTypeField(const String& name, const String& key, AssetType type, const String& defaultValue);
		void addDropdownField(const String& name, const String& key, Vector<String> values, const String& defaultValue);

		template <typename T>
		void addEnumField(const String& name, const String& key, const String& defaultValue)
		{
			const auto names = EnumNames<T>()();
			Vector<String> values;
			for (const auto& name: names) {
				values.push_back(name);
			}
			addDropdownField(name, key, std::move(values), defaultValue);
		}

		void makeLabel(const String& name);
		void makeIntField(UISizer& sizer, const String& key, int defaultValue);
		void makeFloatField(UISizer& sizer, const String& key, float defaultValue);
		void makeBoolField(UISizer& sizer, const String& key, bool defaultValue);
		void makeStringField(UISizer& sizer, const String& key, const String& defaultValue);
		void makeDropdownField(UISizer& sizer, const String& key, Vector<String> values, const String& defaultValue);
		void makeAssetTypeField(UISizer& sizer, const String& key, AssetType type, const String& defaultValue);
	};
}
