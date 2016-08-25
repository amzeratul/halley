#pragma once

#include "prec.h"
#include <boost/filesystem.hpp>

namespace Halley
{
	namespace Stages {
		enum Type
		{
			Root
		};
	}

	class Project;
	class Preferences;

	class HalleyEditor final : public Game
	{
	public:
		HalleyEditor();
		~HalleyEditor();

		Project& loadProject(boost::filesystem::path path);
		Project& createProject(boost::filesystem::path path);

		bool hasProjectLoaded() const;
		Project& getProject() const;

	protected:
		int initPlugins(IPluginRegistry &registry) override;
		void initResourceLocator(String dataPath, ResourceLocator& locator) override;
		void init(HalleyAPI* api, const Environment& environment, const Vector<String>& args) override;

		std::unique_ptr<Stage> makeStage(StageID id) override;

		StageID getInitialStage() const override;
		String getName() const override;
		String getDataPath() const override;
		bool isDevBuild() const override;

	private:
		std::unique_ptr<Project> project;
		std::unique_ptr<Preferences> preferences;
		boost::filesystem::path sharedAssetsPath;
	};
}