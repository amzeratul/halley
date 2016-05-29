#pragma once

#include "halley/core/stage/stage.h"

namespace Halley
{
	class IPluginRegistry;
	class ResourceLocator;
	class Stage;

	class Game
	{
	public:
		virtual ~Game() = default;

		virtual int initPlugins(IPluginRegistry &registry) = 0;
		virtual void initResourceLocator(String, ResourceLocator&) {}

		virtual String getName() const = 0;
		virtual String getDataPath() const = 0;
		virtual bool isDevBuild() const = 0;

		virtual void init(HalleyAPI*) {}
		virtual void deInit() {}

		virtual std::unique_ptr<Stage> makeStage(StageID id) = 0;
		virtual StageID getInitialStage() const = 0;
	};
}
