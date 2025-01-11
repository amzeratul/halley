#pragma once
#include "halley/stage/stage.h"
#include "create_functions.h"

namespace Halley
{
	class InspectorClient;

	class EntityStage : public Stage
	{
	public:
		void init() override;
		void onVariableUpdate(Time dt) override;

		std::unique_ptr<World> createWorld(const String& configName, const std::optional<String>& systemTag = std::nullopt);

		virtual Vector<World*> getWorlds();

	private:
		std::shared_ptr<InspectorClient> inspectorClient;
	};
}
