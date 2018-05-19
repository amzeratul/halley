#pragma once

#include "project.h"

namespace Halley
{
	class ProjectLoader
	{
	public:
		ProjectLoader(const HalleyStatics& statics, const Path& halleyPath);

		std::unique_ptr<Project> loadProject(const Path& path) const;
		void setPlatform(const String& platform);

	private:
		const HalleyStatics& statics;
		Path halleyPath;
		String curPlatform;
		std::vector<HalleyPluginPtr> plugins;

		HalleyPluginPtr loadPlugin(const Path& path);
	};
}
