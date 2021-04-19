#pragma once
#include "scripting/script_environment.h"

namespace Halley {
	class ScriptWaitData : public IScriptStateData {
	public:
		Time timeLeft = 0;
	};
	
	class ScriptWait final : public ScriptNodeTypeBase<ScriptWaitData> {
	public:
		String getName() override { return "wait"; }
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptWaitData& curData) override;
		void doInitData(ScriptWaitData& data, const ScriptGraphNode& node) override;
	};
}
