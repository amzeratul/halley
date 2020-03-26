#pragma once
#include "halley/core/api/halley_api_internal.h"

namespace Halley {
	class CoreAPIWrapper : public CoreAPIInternal {
	public:
		CoreAPIWrapper(CoreAPI& parent);
		
		void registerPlugin(std::unique_ptr<Plugin> plugin) override;
		Vector<Plugin*> getPlugins(PluginType type) override;
		void quit(int exitCode) override;
		void setStage(StageID stage) override;
		void setStage(std::unique_ptr<Stage> stage) override;
		void initStage(Stage& stage) override;
		Stage& getCurrentStage() override;
		HalleyStatics& getStatics() override;
		const Environment& getEnvironment() override;
		int64_t getTime(CoreAPITimer timer, TimeLine tl, StopwatchAveraging::Mode mode) const override;

	private:
		CoreAPI& parent;
	};
}
