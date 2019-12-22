#pragma once

namespace Halley {
	class Project;

	class AnimationEditor : public UIWidget {
    public:
        AnimationEditor(UIFactory& factory, Resources& resources, Project& project, const String& animationId);

	private:
		UIFactory& factory;
		Project& project;
		std::shared_ptr<const Animation> animation;

		void setupWindow();
	};
}

