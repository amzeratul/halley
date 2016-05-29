#pragma once
#include "stage.h"

namespace Halley
{
	class System;

	class EntityStage : public Stage
	{
	public:
		std::unique_ptr<World> createWorld(String configName, std::function<std::unique_ptr<System>(String)> createFunction);
	};
}
