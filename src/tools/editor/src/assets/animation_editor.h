#pragma once

#include "halley/core/graphics/sprite/animation.h"
#include "halley/core/resources/resources.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"

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

