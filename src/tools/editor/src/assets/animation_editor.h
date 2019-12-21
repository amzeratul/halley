#pragma once

namespace Halley {
	class Project;

	class AnimationEditor : public UIWidget {
    public:
        AnimationEditor(UIFactory& factory, Resources& resources, const String& animationId);

	private:
		UIFactory& factory;
		std::shared_ptr<const Animation> animation;

		void setupWindow();
	};
}

