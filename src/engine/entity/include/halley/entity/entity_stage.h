#pragma once
#include "halley/core/stage/stage.h"
#include "create_functions.h"

namespace Halley
{
	class EntityStage : public Stage
	{
	public:
		std::unique_ptr<World> createWorld(const String& configName);
	};
}
