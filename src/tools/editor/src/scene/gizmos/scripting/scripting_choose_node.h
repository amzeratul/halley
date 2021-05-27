#pragma once

#include "src/scene/choose_asset_window.h"

namespace Halley {
	class ScriptingChooseNode final : public ChooseAssetWindow {
	public:
		ScriptingChooseNode(UIFactory& factory, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes, const Callback& callback);
	};
}
