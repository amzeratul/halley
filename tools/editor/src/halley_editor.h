#pragma once

#include "prec.h"
#include "halley/file/path.h"

namespace Halley
{
	class Project;
	class Preferences;

	class HalleyEditor final : public Game
	{
	public:
		HalleyEditor();
		~HalleyEditor();

		Project& loadProject(const HalleyStatics& statics, const String& platform, Path path);
		Project& createProject(Path path);

		bool hasProjectLoaded() const;
		Project& getProject() const;
		bool isHeadless() const { return headless; }

	protected:
		void init(const Environment& environment, const Vector<String>& args) override;
		int initPlugins(IPluginRegistry &registry) override;
		void initResourceLocator(Path dataPath, ResourceLocator& locator) override;
		std::unique_ptr<Stage> startGame(HalleyAPI* api) override;

		String getName() const override;
		String getDataPath() const override;
		bool isDevBuild() const override;
		bool shouldCreateSeparateConsole() const override;

	private:
		std::unique_ptr<Project> project;
		std::unique_ptr<Preferences> preferences;
		Path rootPath;

		bool headless = true;
		String platform;
		String projectPath;
		bool gotProjectPath = false;

		void parseArguments(const std::vector<String>& args);
	};
}