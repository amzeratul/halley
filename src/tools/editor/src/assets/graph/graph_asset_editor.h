#pragma once

#include "../asset_editor.h"
#include "halley/graph/base_graph.h"
#include "halley/tools/dll/project_dll.h"

namespace Halley {
	class GraphEditor;

	class GraphAssetEditor : public AssetEditor, IProjectDLLListener {
	public:
		GraphAssetEditor(UIFactory& factory, Resources& gameResources, ProjectWindow& projectWindow, AssetType assetType);
		~GraphAssetEditor() override;

        void onResourceLoaded() override;
		void refreshAssets() override;
		
		void save() override;
		bool isModified() override;

		void onProjectDLLStatusChange(ProjectDLL::Status status) override;

    protected:
		std::shared_ptr<const Resource> loadResource(const Path& assetPath, const String& assetId, AssetType assetType) override;
		virtual std::shared_ptr<BaseGraph> makeGraph() = 0;
		virtual std::shared_ptr<GraphEditor> makeGraphEditor(std::shared_ptr<BaseGraph> graph) = 0;
		
    	ProjectWindow& projectWindow;

    private:
		bool dllListenerAdded = false;

		std::shared_ptr<GraphEditor> graphEditor;
	};

	class ScriptGraphAssetEditor : public GraphAssetEditor {
	public:
		ScriptGraphAssetEditor(UIFactory& factory, Resources& gameResoures, ProjectWindow& projectWindow);

	protected:
		std::shared_ptr<BaseGraph> makeGraph() override;
		std::shared_ptr<GraphEditor> makeGraphEditor(std::shared_ptr<BaseGraph> graph) override;
	};
}
