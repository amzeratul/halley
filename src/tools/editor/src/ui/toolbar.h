#pragma once

#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class ProjectWindow;
	class Project;
	class ProjectProperties;

	enum class EditorTabs {
		Assets,
		ECS,
		Remotes,
		Properties,
		Settings
	};

	template <>
	struct EnumNames<EditorTabs> {
		constexpr std::array<const char*, 5> operator()() const {
			return{{
				"assets",
				"ecs",
				"remotes",
				"properties",
				"settings"
			}};
		}
	};

	class Toolbar : public UIWidget
	{
	public:
		Toolbar(UIFactory& factory, ProjectWindow& projectWindow, Project& project);

		const std::shared_ptr<UIList>& getList() const;

	private:
		UIFactory& factory;
		ProjectWindow& projectWindow;
		Project& project;
		std::shared_ptr<UIList> list;

		void makeUI();
	};
}
