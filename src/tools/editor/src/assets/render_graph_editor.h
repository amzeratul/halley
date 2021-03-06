#pragma once

#include "graph_editor.h"

namespace Halley {
	class RenderGraphEditor : public GraphEditor {
	public:
		RenderGraphEditor(UIFactory& factory, Resources& resources, Project& project, AssetType type);

	protected:
		std::shared_ptr<const Resource> loadResource(const String& assetId) override;
	};
}
