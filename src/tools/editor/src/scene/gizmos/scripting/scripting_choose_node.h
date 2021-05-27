#pragma once

#include "src/scene/choose_asset_window.h"

namespace Halley {
	class ScriptingChooseNode final : public ChooseAssetWindow {
	public:
		ScriptingChooseNode(UIFactory& factory, Resources& resources, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes, const Callback& callback);

	protected:
		std::shared_ptr<UISizer> makeItemSizer(Sprite icon, std::shared_ptr<UILabel> label) override;
		Sprite makeIcon(const String& id) override;
		void sortItems(std::vector<std::pair<String, String>>& items) override;
	
	private:
		Resources& resources;
		std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes;
	};
}
