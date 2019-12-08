#pragma once

namespace Halley {
	class ProjectProperties;

	class Toolbar : public UIWidget
	{
	public:
		Toolbar(UIFactory& factory, const ProjectProperties& projectProperties);
	};
}
