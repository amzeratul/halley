#pragma once

#include "halley/tools/dll/project_dll.h"
#include "src/assets/asset_editor.h"

namespace Halley {
	class RenderGraphEditor : public AssetEditor {
	public:
		RenderGraphEditor(UIFactory& factory, Resources& gameResources, Project& project, AssetType type);

	protected:
		void onResourceLoaded() override;
		std::shared_ptr<const Resource> loadResource(const Path& assetPath, const String& assetId, AssetType assetType) override;

	private:
		std::shared_ptr<const RenderGraphDefinition> renderGraph;
	};
}
