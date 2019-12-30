#pragma once

#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class ProjectProperties;

	class Toolbar : public UIWidget
	{
	public:
		Toolbar(UIFactory& factory, const ProjectProperties& projectProperties);
	};
}
