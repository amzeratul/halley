#pragma once
#include "halley/plugin/plugin.h"

namespace Halley {
	class DummyVideoPlugin : public Plugin {
	public:
		PluginType getType() override;
		String getName() override;
		HalleyAPIInternal* createAPI(SystemAPI*) override;
		int getPriority() const override;
	};

	class DummyAudioPlugin : public Plugin {
	public:
		PluginType getType() override;
		String getName() override;
		HalleyAPIInternal* createAPI(SystemAPI*) override;
		int getPriority() const override;
	};

	class DummyInputPlugin : public Plugin {
	public:
		PluginType getType() override;
		String getName() override;
		HalleyAPIInternal* createAPI(SystemAPI*) override;
		int getPriority() const override;
	};

	class DummySystemPlugin : public Plugin {
	public:
		PluginType getType() override;
		String getName() override;
		HalleyAPIInternal* createAPI(SystemAPI*) override;
		int getPriority() const override;
	};
}
