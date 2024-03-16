#pragma once

#include "../asset_editor.h"
#include "halley/tools/dll/project_dll.h"

namespace Halley {
	class ScriptGraphEditor;

	class ScriptGraphAssetEditor : public AssetEditor, IProjectDLLListener {
	public:
		ScriptGraphAssetEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow);
		~ScriptGraphAssetEditor() override;

        void onResourceLoaded() override;
		void refreshAssets() override;
		
		void save() override;
		bool isModified() override;

		void onProjectDLLStatusChange(ProjectDLL::Status status) override;

    protected:
		std::shared_ptr<const Resource> loadResource(const Path& assetPath, const String& assetId, AssetType assetType) override;

    private:
    	ProjectWindow& projectWindow;
		Resources& gameResources;
		bool dllListenerAdded = false;

		std::shared_ptr<ScriptGraphEditor> graphEditor;
	};
}
