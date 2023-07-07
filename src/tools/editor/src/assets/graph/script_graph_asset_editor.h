#pragma once

#include "../asset_editor.h"
#include "halley/tools/dll/project_dll.h"

namespace Halley {
	class ScriptGraphEditor;

	class ScriptGraphAssetEditor : public AssetEditor, IProjectDLLListener {
	public:
		ScriptGraphAssetEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow);
		~ScriptGraphAssetEditor() override;

        void reload() override;
		void refreshAssets() override;
		
		void save() override;
		bool isModified() override;

		void onProjectDLLStatusChange(ProjectDLL::Status status) override;

    protected:
    	void update(Time t, bool moved) override;
		std::shared_ptr<const Resource> loadResource(const String& assetId) override;

    private:
    	ProjectWindow& projectWindow;
		Resources& gameResources;
		bool dllListenerAdded = false;
		bool pendingLoad = false;

		std::shared_ptr<ScriptGraphEditor> graphEditor;

		void open();
    };
}
