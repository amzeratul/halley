#pragma once

#include "stage_id.h"
#include "../resources/resources.h"
#include "../api/halley_api.h"

namespace Halley
{
	class HalleyAPI;
	class Painter;

	class Stage
	{
	public:
		virtual ~Stage() {}

		virtual void onFixedUpdate(Time) {}
		virtual void onVariableUpdate(Time) {}
		virtual void onRender(Painter&) const {}

		virtual void init() {}
		virtual void deInit() {}

	protected:
		explicit Stage(String name = "unnamed");

		HalleyAPI& getAPI() { return *api; }
		const HalleyAPI& getAPI() const { return *api; }

		template <typename T>
		std::shared_ptr<T> getResource(String name) const {
			return getAPI().core->getResources().get<T>(name);
		}

	private:
		friend class CoreRunner;

		void doInit(HalleyAPI* api);
		void doDeInit();

		String name;
		HalleyAPI* api;
	};

}
