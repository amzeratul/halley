#pragma once

#include "asset_browser_tabs.h"
#include "halley/data_structures/config_node.h"
#include "halley/text/halleystring.h"

namespace Halley {
	class IAssetFileHandler {
	public:
		struct MenuEntry {
			String id;
			String text;
			String tooltip;
			String icon;
		};

		virtual ~IAssetFileHandler() = default;

		virtual bool canCreateNew() const = 0;
		virtual bool canDuplicate() const = 0;
		virtual AssetType getAssetType() const = 0;
		virtual std::string_view getRootAssetDirectory() const = 0;
		virtual std::string_view getName() const = 0;
		virtual std::string_view getFileExtension() const = 0;
		virtual String makeDefaultFile() const = 0;
		virtual String duplicateAsset(const ConfigNode& node) const = 0;
		virtual Vector<MenuEntry> getEmptySpaceContextMenuEntries() const = 0;
		virtual Vector<MenuEntry> getContextMenuEntries() const = 0;
		virtual void onContextMenu(const String& actionId, UIRoot& ui, EditorUIFactory& factory, const String& assetId, Project& project) const = 0;
	};

	class AssetFileHandler {
	public:
		AssetFileHandler();

		const IAssetFileHandler* tryGetHandlerFor(const String& assetType) const;
		const IAssetFileHandler* tryGetHandlerFor(const Path& path) const;

	private:
		void populate();
		void clear();
		void addHandler(std::unique_ptr<IAssetFileHandler> handler);

		HashMap<String, std::unique_ptr<IAssetFileHandler>> handlers;
	};

	class AssetFileHandlerBase : public IAssetFileHandler {
	public:
		AssetFileHandlerBase(AssetType type, String rootAssetDir, String name, String fileExtension);

		bool canCreateNew() const override { return true; }
		bool canDuplicate() const override { return true; }
		AssetType getAssetType() const override;
		std::string_view getRootAssetDirectory() const override;
		std::string_view getName() const override;
		std::string_view getFileExtension() const override;
		Vector<MenuEntry> getEmptySpaceContextMenuEntries() const override;
		Vector<MenuEntry> getContextMenuEntries() const override;
		void onContextMenu(const String& actionId, UIRoot& ui, EditorUIFactory& factory, const String& assetId, Project& project) const override;

	private:
		AssetType type;
		String rootAssetDir;
		String name;
		String fileExtension;
	};

	class AssetFileHandlerPrefab : public AssetFileHandlerBase {
	public:
		AssetFileHandlerPrefab();
		String makeDefaultFile() const override;
		String duplicateAsset(const ConfigNode& node) const override;
	};

	class AssetFileHandlerScene : public AssetFileHandlerBase {
	public:
		AssetFileHandlerScene();
		String makeDefaultFile() const override;
		String duplicateAsset(const ConfigNode& node) const override;
	};

	class AssetFileHandlerAudioObject : public AssetFileHandlerBase {
	public:
		AssetFileHandlerAudioObject();
		String makeDefaultFile() const override;
		String duplicateAsset(const ConfigNode& node) const override;
	};

	class AssetFileHandlerAudioEvent : public AssetFileHandlerBase {
	public:
		AssetFileHandlerAudioEvent();
		String makeDefaultFile() const override;
		String duplicateAsset(const ConfigNode& node) const override;
		Vector<MenuEntry> getEmptySpaceContextMenuEntries() const override;
		void onContextMenu(const String& actionId, UIRoot& ui, EditorUIFactory& factory, const String& assetId, Project& project) const override;

	private:
		void convertLegacyEvents(Project& project) const;
	};

	class AssetFileHandlerUI : public AssetFileHandlerBase {
	public:
		AssetFileHandlerUI();
		String makeDefaultFile() const override;
		String duplicateAsset(const ConfigNode& node) const override;
	};

	class AssetFileHandlerCometScript : public AssetFileHandlerBase {
	public:
		AssetFileHandlerCometScript();
		String makeDefaultFile() const override;
		String duplicateAsset(const ConfigNode& node) const override;
	};
}
