#pragma once
#include "halley/api/halley_api_internal.h"

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
		bool isDevMode() override;
		void addProfilerCallback(IProfileCallback* callback) override;
		void removeProfilerCallback(IProfileCallback* callback) override;
		Future<std::unique_ptr<RenderSnapshot>> requestRenderSnapshot() override;
		DevConClient* getDevConClient() const override;

	private:
		CoreAPI& parent;
	};
}
