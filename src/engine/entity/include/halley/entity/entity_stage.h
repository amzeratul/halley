#pragma once
#include "halley/core/stage/stage.h"

namespace Halley
{
	class System;
	class EntityRef;
	class EntityFactory;

	class EntityStage : public Stage
	{
	public:
		using CreateSystemFunction = std::function<std::unique_ptr<System>(String)>;
		using CreateComponentFunction = std::function<void(EntityFactory& factory, const String& componentName, EntityRef& entity, const ConfigNode& componentData)>;
		std::unique_ptr<World> createWorld(const String& configName, CreateSystemFunction createFunction, CreateComponentFunction createComponent);
	};
}
