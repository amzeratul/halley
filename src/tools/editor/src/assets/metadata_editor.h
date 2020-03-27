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

	private:
		UIFactory& factory;
		Metadata metadata;
		Metadata effectiveMetadata;
		Project* project = nullptr;
		AssetType assetType;
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

		void makeLabel(const String& name);
		void makeIntField(UISizer& sizer, const String& key, int defaultValue);
		void makeFloatField(UISizer& sizer, const String& key, float defaultValue);
		void makeBoolField(UISizer& sizer, const String& key, bool defaultValue);
		void makeStringField(UISizer& sizer, const String& key, const String& defaultValue);
	};
}
