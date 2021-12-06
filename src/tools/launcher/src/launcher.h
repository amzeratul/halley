#pragma once

#include <halley.hpp>

namespace Halley
{
	class HalleyLauncher final : public Game
	{
	public:
		HalleyLauncher();
		~HalleyLauncher();

	protected:
		void init(const Environment& environment, const Vector<String>& args) override;
		int initPlugins(IPluginRegistry &registry) override;
		Resources::Options initResourceLocator(const Path& gamePath, const Path& assetsPath, const Path& unpackedAssetsPath, ResourceLocator& locator) override;
		std::unique_ptr<Stage> startGame() override;

		String getName() const override;
		String getDataPath() const override;
		bool isDevMode() const override;
		bool shouldCreateSeparateConsole() const override;
	};
}