#pragma once

#include "project.h"

namespace Halley
{
	class ProjectLoader
	{
	public:
		ProjectLoader(const HalleyStatics& statics, const Path& halleyPath);

		std::unique_ptr<Project> loadProject(const Path& path) const;
		void setPlatforms(std::vector<String> platforms);

	private:
		const HalleyStatics& statics;
		Path halleyPath;
		std::vector<String> curPlatforms;
		std::vector<HalleyPluginPtr> plugins;

		HalleyPluginPtr loadPlugin(const Path& path);
	};
}
