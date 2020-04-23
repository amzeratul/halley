#pragma once

#include "project.h"

namespace Halley
{
	class ProjectLoader
	{
	public:
		ProjectLoader(const HalleyStatics& statics, const Path& halleyPath);

		std::unique_ptr<Project> loadProject(const Path& path) const;
		std::vector<HalleyPluginPtr> getPlugins(std::vector<String> platforms) const;

	private:
		const HalleyStatics& statics;
		Path halleyPath;

		std::vector<String> knownPlatforms;
		std::vector<HalleyPluginPtr> plugins;

		void loadPlugins();
		HalleyPluginPtr loadPlugin(const Path& path) const;
	};
}
