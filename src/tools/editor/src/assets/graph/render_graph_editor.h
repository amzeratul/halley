#pragma once

#include "graph_editor.h"

namespace Halley {
	class RenderGraphEditor : public GraphEditor {
	public:
		RenderGraphEditor(UIFactory& factory, Resources& gameResources, Project& project, AssetType type);

		void reload() override;

	protected:
		std::shared_ptr<const Resource> loadResource(const String& assetId) override;
	};
}
