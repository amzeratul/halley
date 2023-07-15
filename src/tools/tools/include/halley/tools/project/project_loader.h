#pragma once

#include "project.h"

namespace Halley
{
	class Preferences;

	class ProjectLoader
	{
	public:
		ProjectLoader(const HalleyStatics& statics, Path halleyPath, Vector<String> disabledPlatforms = {});

		void setDisabledPlatforms(Vector<String> platforms);
		void setImporterOption(std::string_view key, ConfigNode value);
		
		std::unique_ptr<Project> loadProject(const Path& path) const;
		void setupPlugins(Project& project) const;
		const Vector<String>& getKnownPlatforms() const;

	private:
		const HalleyStatics& statics;
		Path halleyPath;
		Vector<String> disabledPlatforms;
		ConfigNode importerOptions;

		Vector<String> knownPlatforms;
		Vector<HalleyPluginPtr> plugins;

		void loadPlugins();
		HalleyPluginPtr loadPlugin(const Path& path) const;
		Vector<HalleyPluginPtr> getPlugins(Vector<String> platforms) const;
	};
}
