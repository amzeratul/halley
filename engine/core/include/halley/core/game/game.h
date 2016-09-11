#pragma once

#include "halley/core/stage/stage.h"

namespace Halley
{
	class IPluginRegistry;
	class ResourceLocator;
	class Stage;
	class Environment;

	class Game
	{
	public:
		virtual ~Game() = default;

		virtual void init(const Environment&, const Vector<String>& /*args*/) {}
		virtual int initPlugins(IPluginRegistry &registry) = 0;
		virtual void initResourceLocator(String, ResourceLocator&) {}

		virtual String getName() const = 0;
		virtual String getDataPath() const = 0;
		virtual bool isDevBuild() const = 0;
		virtual bool shouldCreateSeparateConsole() const { return isDevBuild(); }

		virtual std::unique_ptr<Stage> startGame(HalleyAPI*) = 0;
		virtual void endGame() {}

		virtual std::unique_ptr<Stage> makeStage(StageID id) { return std::unique_ptr<Stage>(); }
	};
}
