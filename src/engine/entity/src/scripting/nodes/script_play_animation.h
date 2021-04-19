#pragma once
#include "scripting/script_environment.h"

namespace Halley {
	class ScriptPlayAnimation final : public ScriptNodeTypeBase<void> {
	public:
		String getName() override { return "playAnimation"; }
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) override;
	};
}
